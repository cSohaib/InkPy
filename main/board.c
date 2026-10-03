/* X4 Pro sequencing adapted from FreeInk SDK (MIT); see docs/PORTING.md. */
#include "board.h"
#include "pins.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/ledc.h"
#include "driver/sdmmc_host.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "sdmmc_cmd.h"

static const char *TAG = "board";
static i2c_master_bus_handle_t i2c;
static SemaphoreHandle_t i2c_lock;
static i2c_master_dev_handle_t touch, rtc, gauge;
static sdmmc_card_t *card;
static unsigned saved_brightness = 20, saved_warmth = 50;
static bool saved_light;

static void pause_ms(unsigned ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }

static esp_err_t output(int pin, int level)
{
    ESP_RETURN_ON_ERROR(gpio_hold_dis(pin), TAG, "release GPIO%d", pin);
    ESP_RETURN_ON_ERROR(gpio_set_level(pin, level), TAG, "GPIO%d level", pin);
    return gpio_set_direction(pin, GPIO_MODE_OUTPUT);
}

static esp_err_t add_device(uint8_t address, i2c_master_dev_handle_t *device)
{
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7, .device_address = address,
        .scl_speed_hz = 400000,
    };
    return i2c_master_bus_add_device(i2c, &cfg, device);
}

esp_err_t ink_light_set(unsigned brightness, unsigned warmth, bool on)
{
    if (brightness > 100 || warmth > 100) return ESP_ERR_INVALID_ARG;
    unsigned total = on ? (brightness * 1023 + 50) / 100 : 0;
    unsigned warm = (total * warmth + 50) / 100;
    ESP_RETURN_ON_ERROR(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, total - warm), TAG, "cool");
    ESP_RETURN_ON_ERROR(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0), TAG, "cool update");
    ESP_RETURN_ON_ERROR(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, warm), TAG, "warm");
    ESP_RETURN_ON_ERROR(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1), TAG, "warm update");
    saved_brightness = brightness;
    saved_warmth = warmth;
    saved_light = on;
    return ESP_OK;
}

/* Sleep switches light off without changing the remembered user state. */
esp_err_t ink_light_sleep(bool sleeping)
{
    unsigned b = saved_brightness, w = saved_warmth;
    bool on = saved_light;
    esp_err_t e = ink_light_set(b, w, sleeping ? false : on);
    saved_light = on;
    return e;
}

static esp_err_t touch_enable(bool enabled)
{
    if (touch) {
        ESP_RETURN_ON_ERROR(i2c_master_bus_rm_device(touch), TAG, "remove touch handle");
        touch = NULL;
    }
    if (!enabled) {
        /* Leave signal pins floating before cutting power, avoiding back-power. */
        ESP_RETURN_ON_ERROR(gpio_set_direction(PIN_TOUCH_IRQ, GPIO_MODE_INPUT), TAG, "IRQ input");
        ESP_RETURN_ON_ERROR(gpio_set_direction(PIN_TOUCH_RESET, GPIO_MODE_INPUT), TAG, "reset input");
        return output(PIN_TOUCH_ENABLE, 1);
    }
    ESP_RETURN_ON_ERROR(output(PIN_TOUCH_ENABLE, 0), TAG, "touch power");
    pause_ms(50);
    for (int select = 0; select <= 1; ++select) {
        ESP_RETURN_ON_ERROR(output(PIN_TOUCH_RESET, 0), TAG, "touch reset");
        ESP_RETURN_ON_ERROR(output(PIN_TOUCH_IRQ, select), TAG, "touch address select");
        pause_ms(10);
        gpio_set_level(PIN_TOUCH_RESET, 1);
        pause_ms(10);
        pause_ms(50);
        gpio_set_direction(PIN_TOUCH_IRQ, GPIO_MODE_INPUT);
        pause_ms(50);
        const uint8_t addresses[] = {0x5d, 0x14};
        for (unsigned i = 0; i < sizeof(addresses); ++i) {
            if (i2c_master_probe(i2c, addresses[i], 20) == ESP_OK) {
                ESP_RETURN_ON_ERROR(add_device(addresses[i], &touch), TAG, "touch handle");
                ESP_LOGI(TAG, "GT911 address=0x%02x", addresses[i]);
                return ESP_OK;
            }
        }
    }
    return ESP_ERR_NOT_FOUND;
}

static esp_err_t touch_read(ink_touch_t *out, bool *fresh)
{
    *fresh = false;
    if (!touch) return ESP_ERR_INVALID_STATE;
    uint8_t reg[] = {0x81, 0x4e}, status;
    esp_err_t e = i2c_master_transmit_receive(touch, reg, 2, &status, 1, 20);
    if (e != ESP_OK || !(status & 0x80)) return e;
    ink_touch_t frame = {.home = (status & 0x10) != 0, .contacts = status & 0x0f};
    if (frame.contacts > 5) return ESP_ERR_INVALID_RESPONSE;
    if (frame.contacts) {
        uint8_t point[8];
        reg[1] = 0x50;
        e = i2c_master_transmit_receive(touch, reg, 2, point, sizeof(point), 20);
        if (e != ESP_OK) return e; /* Do not turn a failed read into a release. */
        uint16_t x = point[0] | (point[1] << 8), y = point[2] | (point[3] << 8);
        frame.x = y < PANEL_WIDTH ? y : PANEL_WIDTH - 1;
        frame.y = PANEL_HEIGHT - 1 - (x < PANEL_HEIGHT ? x : PANEL_HEIGHT - 1);
    }
    const uint8_t clear[] = {0x81, 0x4e, 0};
    e = i2c_master_transmit(touch, clear, sizeof(clear), 20);
    if (e != ESP_OK) return e;
    *out = frame;
    *fresh = true;
    return ESP_OK;
}

static int bcd(uint8_t b) { return (b >> 4) * 10 + (b & 15); }

static esp_err_t rtc_read(struct tm *out)
{
    if (!rtc) return ESP_ERR_INVALID_STATE;
    uint8_t reg = 2, data[7];
    ESP_RETURN_ON_ERROR(i2c_master_transmit_receive(rtc, &reg, 1, data, 7, 20), TAG, "RTC read");
    if (data[0] & 0x80) return ESP_ERR_INVALID_STATE; /* Oscillator/voltage-low: time untrusted. */
    const uint8_t masks[] = {0x7f, 0x7f, 0x3f, 0x3f, 7, 0x1f, 0xff};
    for (unsigned i = 0; i < 7; ++i) {
        uint8_t n = data[i] & masks[i];
        if ((n & 15) > 9 || (n >> 4) > 9) return ESP_ERR_INVALID_RESPONSE;
    }
    struct tm t = {
        .tm_sec = bcd(data[0] & 0x7f), .tm_min = bcd(data[1] & 0x7f),
        .tm_hour = bcd(data[2] & 0x3f), .tm_mday = bcd(data[3] & 0x3f),
        .tm_wday = bcd(data[4] & 7), .tm_mon = bcd(data[5] & 0x1f) - 1,
        .tm_year = bcd(data[6]) + ((data[5] & 0x80) ? 0 : 100), .tm_isdst = -1,
    };
    if (t.tm_sec > 59 || t.tm_min > 59 || t.tm_hour > 23 || t.tm_wday > 6 ||
        t.tm_mon < 0 || t.tm_mon > 11 || t.tm_mday < 1 || t.tm_mday > 31)
        return ESP_ERR_INVALID_RESPONSE;
    *out = t;
    return ESP_OK;
}

esp_err_t ink_board_init(void)
{
    i2c_lock=xSemaphoreCreateMutex();
    if(!i2c_lock) return ESP_ERR_NO_MEM;
    ESP_RETURN_ON_ERROR(output(PIN_RAIL, 1), TAG, "peripheral rail");
    const gpio_config_t buttons = {
        .pin_bit_mask = (1ULL << PIN_PREV) | (1ULL << PIN_NEXT) | (1ULL << PIN_POWER),
        .mode = GPIO_MODE_INPUT, .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&buttons), TAG, "buttons");
    ESP_RETURN_ON_ERROR(gpio_set_direction(PIN_CHARGING, GPIO_MODE_INPUT), TAG, "charge input");
    i2c_master_bus_config_t bus = {
        .i2c_port = I2C_NUM_0, .sda_io_num = PIN_I2C_SDA, .scl_io_num = PIN_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT, .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus, &i2c), TAG, "I2C bus");
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE, .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = LEDC_TIMER_0, .freq_hz = 25000, .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&timer), TAG, "light timer");
    for (unsigned i = 0; i < 2; ++i) {
        ESP_RETURN_ON_ERROR(gpio_hold_dis(i ? PIN_WARM : PIN_COOL), TAG, "light hold");
        ledc_channel_config_t channel = {
            .gpio_num = i ? PIN_WARM : PIN_COOL, .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel = (ledc_channel_t)i, .timer_sel = LEDC_TIMER_0, .duty = 0,
        };
        ESP_RETURN_ON_ERROR(ledc_channel_config(&channel), TAG, "light channel");
    }
    esp_err_t e = ink_touch_enable(true);
    if (e != ESP_OK) ESP_LOGW(TAG, "touch unavailable: %s", esp_err_to_name(e));
    if (i2c_master_probe(i2c, 0x51, 20) == ESP_OK) {
        ESP_RETURN_ON_ERROR(add_device(0x51, &rtc), TAG, "RTC handle");
    }
    if (i2c_master_probe(i2c, 0x63, 20) == ESP_OK) {
        ESP_RETURN_ON_ERROR(add_device(0x63, &gauge), TAG, "gauge handle");
    }
    return ESP_OK;
}

esp_err_t ink_touch_enable(bool enabled)
{
    xSemaphoreTake(i2c_lock,portMAX_DELAY);
    esp_err_t e=touch_enable(enabled); xSemaphoreGive(i2c_lock); return e;
}
esp_err_t ink_touch_read(ink_touch_t *out,bool *fresh)
{
    xSemaphoreTake(i2c_lock,portMAX_DELAY);
    esp_err_t e=touch_read(out,fresh); xSemaphoreGive(i2c_lock); return e;
}
esp_err_t ink_rtc_read(struct tm *out)
{
    xSemaphoreTake(i2c_lock,portMAX_DELAY);
    esp_err_t e=rtc_read(out); xSemaphoreGive(i2c_lock); return e;
}

esp_err_t ink_rtc_set(const struct tm *t)
{
    if (!rtc || t->tm_year<100 || t->tm_year>199 || t->tm_mon<0 || t->tm_mon>11 ||
        t->tm_mday<1 || t->tm_mday>31 || t->tm_hour<0 || t->tm_hour>23 ||
        t->tm_min<0 || t->tm_min>59 || t->tm_sec<0 || t->tm_sec>59 || t->tm_wday<0 || t->tm_wday>6)
        return ESP_ERR_INVALID_ARG;
    const int values[]={t->tm_sec,t->tm_min,t->tm_hour,t->tm_mday,t->tm_wday,t->tm_mon+1,t->tm_year-100};
    uint8_t data[8]={2};
    for(unsigned i=0;i<7;i++) data[i+1]=(uint8_t)((values[i]/10)*16+values[i]%10);
    xSemaphoreTake(i2c_lock,portMAX_DELAY);
    /* PCF8563 STOP freezes the divider during the calendar write. Always restart. */
    uint8_t stop[]={0,0x20},start[]={0,0};
    esp_err_t e=i2c_master_transmit(rtc,stop,2,20);
    if(e==ESP_OK) e=i2c_master_transmit(rtc,data,sizeof(data),20);
    esp_err_t resume=i2c_master_transmit(rtc,start,2,20);
    xSemaphoreGive(i2c_lock);
    if(e==ESP_OK&&resume==ESP_OK) {
        struct tm copy=*t; struct timeval now={.tv_sec=mktime(&copy)}; settimeofday(&now,NULL);
    }
    return e==ESP_OK?resume:e;
}

esp_err_t ink_battery_read(unsigned *percent,bool *charging)
{
    if(!gauge) return ESP_ERR_INVALID_STATE;
    xSemaphoreTake(i2c_lock,portMAX_DELAY);
    uint8_t mode,version,soc,flag,reg=8;
    esp_err_t e=i2c_master_transmit_receive(gauge,&reg,1,&mode,1,20);
    reg=0; if(e==ESP_OK) e=i2c_master_transmit_receive(gauge,&reg,1,&version,1,20);
    reg=11; if(e==ESP_OK) e=i2c_master_transmit_receive(gauge,&reg,1,&flag,1,20);
    reg=4; if(e==ESP_OK) e=i2c_master_transmit_receive(gauge,&reg,1,&soc,1,20);
    /* Verify the resident X4 Pro OEM profile, without modifying gauge state. */
    static const uint8_t profile[80]={
        0x50,0,0,0,0,0,0,0,0xaa,0xbf,0xb5,0xb4,0xa4,0x9c,0xeb,0xe2,
        0xdf,0xe5,0xca,0xa0,0x8a,0x62,0x53,0x48,0x40,0x3a,0x32,0xb1,0xae,0xda,0xb5,0xff,
        0xff,0xff,0xe8,0xdb,0xd9,0xd6,0xd4,0xd2,0xd0,0xcb,0xc3,0xbc,0x9e,0x87,0x7b,0x71,
        0x72,0x7c,0x8c,0xa3,0xb7,0xc8,0xa5,0x4f,0,0,0xab,0x02,0,0,0,0,
        0,0,0x64,0,0,0,0,0,0,0,0,0,0,0,0,0x23};
    uint8_t resident[80]; reg=0x10;
    if(e==ESP_OK) e=i2c_master_transmit_receive(gauge,&reg,1,resident,sizeof(resident),20);
    if(e==ESP_OK&&memcmp(resident,profile,sizeof(profile))) e=ESP_ERR_INVALID_STATE;
    xSemaphoreGive(i2c_lock);
    if(e!=ESP_OK) return e;
    if(mode!=0 || (version&0xfd)!=0x0d || !(flag&0x80) || soc>100) return ESP_ERR_INVALID_STATE;
    *percent=soc; *charging=gpio_get_level(PIN_CHARGING)!=0; return ESP_OK;
}

esp_err_t ink_sd_mount(void)
{
    if (card) return ESP_OK;
    const esp_vfs_fat_sdmmc_mount_config_t mount = {
        .format_if_mount_failed = false, .max_files = 32, .allocation_unit_size = 16 * 1024,
    };
    uint8_t *sector = heap_caps_malloc(512, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!sector) return ESP_ERR_NO_MEM;
    esp_err_t e = ESP_FAIL;
    for (unsigned attempt = 0; attempt < 4; ++attempt) {
        e = output(PIN_SD_ENABLE, 1);
        if (e != ESP_OK) break;
        pause_ms(80);
        gpio_set_level(PIN_SD_ENABLE, 0);
        pause_ms(120);
        sdmmc_host_t host = SDMMC_HOST_DEFAULT();
        host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;
        sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
        slot.width = 1; slot.clk = PIN_SD_CLK; slot.cmd = PIN_SD_CMD; slot.d0 = PIN_SD_D0;
        slot.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
        e = esp_vfs_fat_sdmmc_mount("/sd", &host, &slot, &mount, &card);
        if (e == ESP_OK) {
            e = sdmmc_read_sectors(card, sector, 0, 1);
            if (e == ESP_OK) break;
            esp_vfs_fat_sdcard_unmount("/sd", card);
            card = NULL;
        }
        ESP_LOGW(TAG, "SD mount attempt %u: %s", attempt + 1, esp_err_to_name(e));
    }
    free(sector);
    return e;
}

esp_err_t ink_sd_probe(void)
{
    if (!card) return ESP_ERR_INVALID_STATE;
    /* O_EXCL ensures a prior interrupted probe or user file is never replaced. */
    const char *path = "/sd/.inkpy-bringup.tmp";
    const char sample[] = "InkPy SD read/write probe\n";
    int fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (fd < 0) return ESP_FAIL;
    bool ok = write(fd, sample, sizeof(sample)) == (ssize_t)sizeof(sample);
    if (fsync(fd) != 0) ok = false;
    if (close(fd) != 0) ok = false;
    char data[sizeof(sample)];
    fd = open(path, O_RDONLY);
    if (fd < 0) ok = false;
    else {
        if (read(fd, data, sizeof(data)) != (ssize_t)sizeof(data) || memcmp(data, sample, sizeof(data))) ok = false;
        if (close(fd) != 0) ok = false;
    }
    if (unlink(path) != 0) ok = false;
    return ok ? ESP_OK : ESP_FAIL;
}

void ink_board_report(void)
{
    ESP_LOGI(TAG, "free internal=%u largest=%u PSRAM=%u",
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    struct tm t;
    esp_err_t e = ink_rtc_read(&t);
    if (e == ESP_OK) ESP_LOGI(TAG, "RTC %04d-%02d-%02d %02d:%02d:%02d", t.tm_year + 1900,
                            t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
    else ESP_LOGW(TAG, "RTC unavailable/unset: %s", esp_err_to_name(e));
    if (gauge) {
        uint8_t reg = 2, raw[4];
        if (i2c_master_transmit_receive(gauge, &reg, 1, raw, sizeof(raw), 20) == ESP_OK)
            ESP_LOGI(TAG, "gauge raw voltage=%02x%02x SoC=%02x%02x charging=%d (profile not validated)",
                     raw[0], raw[1], raw[2], raw[3], gpio_get_level(PIN_CHARGING));
    }
    if (card) sdmmc_card_print_info(stdout, card);
}
