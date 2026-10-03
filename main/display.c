/* Minimal monochrome/full-refresh port of FreeInk SDK, MIT. See docs/PORTING.md.
 * Frame format: landscape 800x480, row-major MSB first, 1=white. No custom LUTs. */
#include "board.h"
#include "pins.h"
#include <string.h>
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_attr.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "display";
static spi_device_handle_t spi;
static ink_panel_t panel;
static bool sleeping, ready;
static bool baseline;
static DMA_ATTR uint8_t transfer[4000];
#define TRY(call) ESP_RETURN_ON_ERROR((call), TAG, "%s", #call)

static void delay_ms(unsigned ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }
static void out(int pin, int level)
{
    gpio_hold_dis(pin);
    gpio_set_level(pin, level);
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
}

static void reset_panel(unsigned low_ms, unsigned settle_ms)
{
    out(PIN_EPD_RESET, 1); delay_ms(10);
    gpio_set_level(PIN_EPD_RESET, 0); delay_ms(low_ms);
    gpio_set_level(PIN_EPD_RESET, 1); delay_ms(settle_ms);
}

static void probe_write(uint8_t b)
{
    for (unsigned i = 0; i < 8; ++i) {
        gpio_set_level(PIN_EPD_MOSI, (b & 0x80) != 0);
        esp_rom_delay_us(1);
        gpio_set_level(PIN_EPD_CLK, 1); esp_rom_delay_us(1);
        gpio_set_level(PIN_EPD_CLK, 0); b <<= 1;
    }
}

static void probe_read(uint8_t command, uint8_t *data, size_t size)
{
    gpio_set_direction(PIN_EPD_MOSI, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_EPD_DC, 0); gpio_set_level(PIN_EPD_CS, 0);
    esp_rom_delay_us(1); probe_write(command);
    gpio_set_level(PIN_EPD_DC, 1);
    gpio_set_direction(PIN_EPD_MOSI, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIN_EPD_MOSI, GPIO_PULLUP_ONLY);
    for (size_t n = 0; n < size; ++n) {
        uint8_t b = 0;
        for (unsigned bit = 0; bit < 8; ++bit) {
            esp_rom_delay_us(1);
            b = (uint8_t)((b << 1) | gpio_get_level(PIN_EPD_MOSI));
            gpio_set_level(PIN_EPD_CLK, 1); esp_rom_delay_us(1);
            gpio_set_level(PIN_EPD_CLK, 0);
        }
        data[n] = b;
    }
    gpio_set_level(PIN_EPD_CS, 1);
    gpio_set_pull_mode(PIN_EPD_MOSI, GPIO_FLOATING);
    gpio_set_direction(PIN_EPD_MOSI, GPIO_MODE_OUTPUT);
}

static bool uniform(const uint8_t *p, size_t n)
{
    for (size_t i = 1; i < n; ++i) if (p[i] != p[0]) return false;
    return true;
}
static bool driven(uint8_t flag) { return flag != 0xff && flag != 0 && (flag & 1); }

static esp_err_t detect(void)
{
    out(PIN_EPD_CS, 1); out(PIN_EPD_CLK, 0); out(PIN_EPD_DC, 0); out(PIN_EPD_MOSI, 0);
    TRY(gpio_set_direction(PIN_EPD_BUSY, GPIO_MODE_INPUT));
    uint8_t ver[2][5], flag[2];
    reset_panel(1, 30);
    probe_read(0x71, &flag[0], 1); probe_read(0x70, ver[0], 5);
    bool first = driven(flag[0]) && !uniform(ver[0], 5);
    delay_ms(2); reset_panel(first ? 50 : 1, 30);
    probe_read(0x71, &flag[1], 1); probe_read(0x70, ver[1], 5);
    bool second = driven(flag[1]) && !uniform(ver[1], 5);
    bool agree = memcmp(ver[0], ver[1], 5) == 0;
    bool uc = first && second && agree;
    if (!uc && driven(flag[0]) && agree && uniform(ver[0], 5) && ver[0][0] == 0xff) {
        uint8_t mtp[49], repeat[49];
        probe_read(0xa2, mtp, sizeof(mtp)); /* First byte is a dummy. */
        if (mtp[1] == 0xa5) uc = true;
        else if (!uniform(mtp + 1, 48)) {
            probe_read(0xa2, repeat, sizeof(repeat));
            uc = memcmp(mtp + 1, repeat + 1, 48) == 0;
        }
    }
    ESP_LOGI(TAG, "VER %02x %02x %02x %02x %02x FLG %02x/%02x agreement=%d",
             ver[0][0], ver[0][1], ver[0][2], ver[0][3], ver[0][4], flag[0], flag[1], agree);
    if (!uc && (!agree || first || second)) return ESP_ERR_INVALID_RESPONSE;
    uint8_t id = ver[0][2];
    panel = !uc ? INK_SSD1677 :
        (id == 2 || id == 3 || id == 0x67 || id == 0x68 || id == 0x69) ? INK_UC8279 : INK_UC8179;
    ESP_LOGI(TAG, "selected %s%s", panel == INK_SSD1677 ? "SSD1677" :
             panel == INK_UC8179 ? "UC8179" : "UC8279", uc ? "" : " (assumed: no UC signature)");
    return ESP_OK;
}

/* All SPI data, including PSRAM frame rows, passes through one internal DMA buffer. */
static esp_err_t bytes(const uint8_t *data, size_t length)
{
    while (length) {
        size_t n = length < sizeof(transfer) ? length : sizeof(transfer);
        memcpy(transfer, data, n);
        spi_transaction_t t = {.length = n * 8, .tx_buffer = transfer};
        esp_err_t e = spi_device_polling_transmit(spi, &t);
        if (e != ESP_OK) return e;
        data += n; length -= n;
    }
    return ESP_OK;
}
static esp_err_t command(uint8_t c, const uint8_t *data, size_t n)
{
    gpio_set_level(PIN_EPD_CS, 0); gpio_set_level(PIN_EPD_DC, 0);
    esp_err_t e = bytes(&c, 1);
    gpio_set_level(PIN_EPD_DC, 1);
    if (e == ESP_OK && n) e = bytes(data, n);
    gpio_set_level(PIN_EPD_CS, 1);
    return e;
}
#define CMD(c, ...) do { const uint8_t d[] = {__VA_ARGS__}; TRY(command(c, d, sizeof(d))); } while (0)

static esp_err_t wait_idle(bool must_start)
{
    int busy = panel == INK_SSD1677 ? 1 : 0;
    int64_t start = esp_timer_get_time();
    if (must_start) {
        while (gpio_get_level(PIN_EPD_BUSY) != busy && esp_timer_get_time() - start < 100000)
            delay_ms(1);
        if (gpio_get_level(PIN_EPD_BUSY) != busy) return ESP_ERR_INVALID_RESPONSE;
    } else delay_ms(10); /* Avoid sampling before a command has asserted BUSY. */
    while (gpio_get_level(PIN_EPD_BUSY) == busy) {
        if (esp_timer_get_time() - start > 30000000) return ESP_ERR_TIMEOUT;
        delay_ms(5);
    }
    return ESP_OK;
}

static esp_err_t ssd_area(void)
{
    CMD(0x11, 0x01);
    CMD(0x44, 0, 0, 0x1f, 0x03); /* x=0..799 */
    CMD(0x45, 0xdf, 0x01, 0, 0); /* y=479..0 */
    CMD(0x4e, 0, 0); CMD(0x4f, 0xdf, 0x01);
    return ESP_OK;
}

static esp_err_t initialize_controller(void)
{
    reset_panel(10, panel == INK_SSD1677 ? 10 : 60);
    if (panel == INK_SSD1677) {
        TRY(command(0x12, NULL, 0)); delay_ms(10); TRY(wait_idle(false));
        CMD(0x18, 0x80); CMD(0x0c, 0xae, 0xc7, 0xc3, 0xc0, 0x80);
        CMD(0x01, 0xdf, 0x01, 0x02); CMD(0x3c, 0x80);
        TRY(ssd_area()); CMD(0x46, 0xf7); TRY(wait_idle(false));
        CMD(0x47, 0xf7); TRY(wait_idle(false));
    } else {
        if (panel == INK_UC8179) { CMD(0x00, 0x3f, 0x0a); }
        else { CMD(0x00, 0x37, 0x4d); }
        CMD(0x61, 0x03, 0x20, 0x02, 0x58); /* Both UC panels address 800x600. */
        CMD(0x65, 0, 0, 0, 0); CMD(0x03, 0x20);
        if (panel == INK_UC8179) { CMD(0x06, 0x25, 0x25, 0x3c, 0x25); }
        else { CMD(0x30, 0x0e); }
        CMD(0xe1, 0x02);
        if (panel == INK_UC8179) { CMD(0xe3, 0x22); }
    }
    sleeping = false;
    baseline = false;
    return ESP_OK;
}

esp_err_t ink_display_init(void)
{
    TRY(detect());
    spi_bus_config_t bus = {
        .mosi_io_num = PIN_EPD_MOSI, .miso_io_num = -1, .sclk_io_num = PIN_EPD_CLK,
        .quadwp_io_num = -1, .quadhd_io_num = -1, .max_transfer_sz = sizeof(transfer),
    };
    TRY(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO));
    spi_device_interface_config_t device = {.clock_speed_hz = 10000000, .mode = 0,
        .spics_io_num = -1, .queue_size = 1};
    TRY(spi_bus_add_device(SPI2_HOST, &device, &spi));
    TRY(initialize_controller());
    ready = true;
    return ESP_OK;
}

ink_panel_t ink_display_panel(void) { return panel; }

static esp_err_t plane(uint8_t c, const uint8_t *frame)
{
    TRY(command(c, NULL, 0));
    uint8_t white[PANEL_STRIDE]; memset(white, 0xff, sizeof(white));
    gpio_set_level(PIN_EPD_CS, 0); gpio_set_level(PIN_EPD_DC, 1);
    esp_err_t e = ESP_OK;
    unsigned rows = panel == INK_SSD1677 ? 480 : 600;
    for (unsigned row = 0; row < rows && e == ESP_OK; ++row) {
        const uint8_t *src = white;
        if (frame) {
            if (panel == INK_SSD1677) src = frame + row * PANEL_STRIDE;
            else if (panel == INK_UC8179 && row < 480) src = frame + (479 - row) * PANEL_STRIDE;
            else if (panel == INK_UC8279 && row >= 120) src = frame + (row - 120) * PANEL_STRIDE;
        }
        e = bytes(src, PANEL_STRIDE);
    }
    gpio_set_level(PIN_EPD_CS, 1);
    return e;
}

esp_err_t ink_display_update(const uint8_t frame[PANEL_BYTES], bool full)
{
    if (!ready) return ESP_ERR_INVALID_STATE;
    if (sleeping) TRY(initialize_controller());
    bool fast = baseline && !full;
    int64_t start = esp_timer_get_time();
    if (panel == INK_SSD1677) {
        TRY(ssd_area()); TRY(plane(0x24, frame));
        if (!fast) TRY(plane(0x26, frame));
        uint8_t ctrl1=fast?0x00:0x40, ctrl2=fast?0xfc:0xf7;
        TRY(command(0x21,&ctrl1,1)); CMD(0x3c, 0xc0); TRY(command(0x22,&ctrl2,1));
        TRY(command(0x20, NULL, 0)); TRY(wait_idle(true));
        TRY(ssd_area()); TRY(plane(0x24, frame)); TRY(plane(0x26, frame));
        /* Stock partial leaves rails on; power down after syncing OLD. */
        if (fast) { CMD(0x3c,0x80); CMD(0x22,0x03); TRY(command(0x20,NULL,0)); delay_ms(200); TRY(wait_idle(false)); }
    } else {
        TRY(plane(0x13, frame)); if (!fast) TRY(plane(0x10, NULL));
        if (panel == INK_UC8179) { CMD(0x50, 0x29, 0x07); }
        else { uint8_t cdi=fast?0xd7:0x97; TRY(command(0x50,&cdi,1)); }
        CMD(0xe0, 0x02); uint8_t temp=fast?0x5a:0x1e; TRY(command(0xe5,&temp,1));
        if (fast) { CMD(0x03,0x20); CMD(0xe1,0x02); }
        if (panel == INK_UC8179) { CMD(0x00, 0x1f, 0x0a); }
        TRY(command(0x04, NULL, 0)); TRY(wait_idle(false));
        /* UC8279 reloads MTP on PON, so its PSR must be written AFTER PON. */
        if (fast) {
            TRY(command(0x91,NULL,0));
            if (panel == INK_UC8279) { CMD(0x90,0,0,0x03,0x1f,0,0x78,0x02,0x57,0x01); }
        }
        if (panel == INK_UC8279) { CMD(0x00, 0x17, 0x4d); }
        TRY(command(0x12, NULL, 0)); TRY(wait_idle(true));
        if (fast) TRY(command(0x92,NULL,0));
        if (panel == INK_UC8179) { CMD(0x50, 0xa9, 0x07); }
        TRY(plane(0x10, frame));
        TRY(command(0x02, NULL, 0)); TRY(wait_idle(false));
    }
    baseline = true;
    ESP_LOGI(TAG, "%s refresh %lld ms", fast?"partial":"full", (long long)((esp_timer_get_time() - start) / 1000));
    return ESP_OK;
}
esp_err_t ink_display_frame(const uint8_t frame[PANEL_BYTES])
{ return ink_display_update(frame,true); }

esp_err_t ink_display_sleep(void)
{
    if (!ready || sleeping) return ESP_OK;
    if (panel == INK_SSD1677) {
        CMD(0x3c, 0x80); CMD(0x22, 0x03); TRY(command(0x20, NULL, 0));
        TRY(wait_idle(false)); CMD(0x10, 0x03);
    } else {
        /* Also power off if a previous refresh failed before its cleanup. */
        TRY(command(0x02, NULL, 0)); TRY(wait_idle(false)); CMD(0x07, 0xa5);
    }
    sleeping = true;
    return ESP_OK;
}
