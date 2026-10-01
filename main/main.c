#include "board.h"
#include "input.h"
#include "pins.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "driver/gpio.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "inkpy";
static uint8_t *frame;

static void pixel(int x, int y)
{
    if (x >= 0 && x < PANEL_WIDTH && y >= 0 && y < PANEL_HEIGHT)
        frame[y * PANEL_STRIDE + x / 8] &= (uint8_t)~(0x80 >> (x % 8));
}

/* Asymmetric diagnostic pattern, not the eventual product UI. Corner squares
 * have sizes 10/20/30/40 clockwise from top-left; cross marks the last tap. */
static void pattern(unsigned page, int touch_x, int touch_y)
{
    memset(frame, 0xff, PANEL_BYTES);
    for (int x = 10; x < PANEL_WIDTH - 10; ++x) { pixel(x, 10); pixel(x, PANEL_HEIGHT - 11); }
    for (int y = 10; y < PANEL_HEIGHT - 10; ++y) { pixel(10, y); pixel(PANEL_WIDTH - 11, y); }
    for (int n = 0; n < 4; ++n) {
        int size = (n + 1) * 10;
        int x0 = (n == 1 || n == 2) ? PANEL_WIDTH - 20 - size : 20;
        int y0 = n >= 2 ? PANEL_HEIGHT - 20 - size : 20;
        for (int y = 0; y < size; ++y) for (int x = 0; x < size; ++x) pixel(x0 + x, y0 + y);
    }
    for (unsigned i = 0; i <= page % 5; ++i)
        for (int y = 180; y < 250; ++y) for (int x = 0; x < 30; ++x) pixel(180 + i * 70 + x, y);
    for (int i = -15; i <= 15; ++i) { pixel(touch_x + i, touch_y); pixel(touch_x, touch_y + i); }
}

void app_main(void)
{
    ESP_LOGI(TAG, "InkPy STAGE 2 HARDWARE DIAGNOSTIC; no reader/editor/Python yet");
    const esp_partition_t *running = esp_ota_get_running_partition();
    if (running) ESP_LOGI(TAG, "running partition %s @0x%lx size=0x%lx", running->label,
                         (unsigned long)running->address, (unsigned long)running->size);
    esp_err_t e = ink_board_init();
    if (e != ESP_OK) { ESP_LOGE(TAG, "board init: %s", esp_err_to_name(e)); return; }
    e = ink_display_init();
    bool display_ok = e == ESP_OK;
    ESP_LOGI(TAG, "display init: %s", esp_err_to_name(e));
    e = ink_sd_mount();
    ESP_LOGI(TAG, "SD mount: %s (never formats)", esp_err_to_name(e));
    if (e == ESP_OK) ESP_LOGI(TAG, "SD exclusive write/read/delete probe: %s", esp_err_to_name(ink_sd_probe()));
    ink_board_report();
    frame = heap_caps_malloc(PANEL_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!frame) { ESP_LOGE(TAG, "PSRAM framebuffer allocation failed"); return; }
    unsigned page = 0, warmth = 50;
    bool light_on = false;
    pattern(page, -100, -100);
    if (display_ok) {
        e = ink_display_frame(frame);
        display_ok = e == ESP_OK;
        ESP_LOGI(TAG, "initial pattern: %s", esp_err_to_name(e));
    }
    ESP_LOGI(TAG, "Prev/Next=pattern; Home tap=warmth 0/50/100; Home hold=report; Power tap=report, double=light, hold=sleep");
    ink_button_t prev = {0}, next = {0}, home = {0}, power = {0}, contact = {0};
    ink_touch_t touch = {0};
    bool moved = false;
    uint16_t origin_x = 0, origin_y = 0;
    uint32_t last_activity = (uint32_t)(esp_timer_get_time() / 1000);
    for (;;) {
        uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);
        bool changed = false, fresh;
        ink_touch_t t;
        e = ink_touch_read(&t, &fresh);
        if (e == ESP_OK && fresh) {
            if (!touch.contacts && t.contacts) { origin_x = t.x; origin_y = t.y; moved = false; }
            if (t.contacts > 1 || (t.contacts && (abs((int)t.x - origin_x) > 12 || abs((int)t.y - origin_y) > 12)))
                moved = true; /* A slide is ignored; never reinterpret it as a tap. */
            touch = t;
        }
        bool p = !gpio_get_level(PIN_POWER), a = !gpio_get_level(PIN_PREV), b = !gpio_get_level(PIN_NEXT);
        if (p || a || b || touch.home || touch.contacts) last_activity = now;
        ink_press_t pe = ink_button_update(&power, p, now, true);
        ink_press_t ae = ink_button_update(&prev, a, now, false);
        ink_press_t be = ink_button_update(&next, b, now, false);
        ink_press_t he = ink_button_update(&home, touch.home, now, false);
        ink_press_t te = ink_button_update(&contact, touch.contacts != 0, now, false);
        if (ae == INK_PRESS_SHORT || be == INK_PRESS_SHORT) {
            page = (page + (be == INK_PRESS_SHORT ? 1 : 4)) % 5;
            pattern(page, -100, -100); changed = true;
        }
        if ((te == INK_PRESS_SHORT || te == INK_PRESS_LONG) && !moved) {
            ESP_LOGI(TAG, "touch %s x=%u y=%u", te == INK_PRESS_LONG ? "hold" : "tap", origin_x, origin_y);
            pattern(page, origin_x, origin_y); changed = true;
        }
        if (he == INK_PRESS_SHORT) {
            warmth = (warmth + 50) % 150;
            ESP_LOGI(TAG, "Home tap, warmth=%u (light state unchanged)", warmth);
            ESP_LOGI(TAG, "light: %s", esp_err_to_name(ink_light_set(20, warmth, light_on)));
        }
        if (he == INK_PRESS_LONG || pe == INK_PRESS_SHORT) ink_board_report();
        if (pe == INK_PRESS_DOUBLE) {
            light_on = !light_on;
            ESP_LOGI(TAG, "light: %s", esp_err_to_name(ink_light_set(20, warmth, light_on)));
        }
        if (pe == INK_PRESS_LONG || now - last_activity >= 300000) {
            e = ink_sleep();
            ESP_LOGI(TAG, "sleep probe: %s", esp_err_to_name(e));
            prev = next = home = power = contact = (ink_button_t){0};
            touch = (ink_touch_t){0};
            last_activity = (uint32_t)(esp_timer_get_time() / 1000);
            changed = false; /* Wake must not redraw, even if input had been pending. */
        }
        if (changed && display_ok) {
            e = ink_display_frame(frame);
            if (e != ESP_OK) { display_ok = false; ESP_LOGE(TAG, "display stopped after error: %s", esp_err_to_name(e)); }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
