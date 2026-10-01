#include "board.h"
#include "input.h"
#include "pins.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "sleep";

esp_err_t ink_sleep(void)
{
    /* The diagnostic has no VM, open files, Wi-Fi or other worker tasks. When
     * these are added, this entry point must be preceded by a quiesce handshake,
     * never vTaskSuspend on a worker that may own a bus/filesystem lock. */
    ESP_RETURN_ON_ERROR(ink_display_sleep(), TAG, "panel standby");
    ESP_RETURN_ON_ERROR(ink_light_sleep(true), TAG, "light off");
    esp_err_t e = ink_touch_enable(false);
    if (e != ESP_OK) goto restore;
    /* Finish the entering press before arming an active-low wake source. */
    while (!gpio_get_level(PIN_POWER)) vTaskDelay(pdMS_TO_TICKS(10));
    e = esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    if (e != ESP_OK) goto restore;
    e = gpio_wakeup_enable(PIN_POWER, GPIO_INTR_LOW_LEVEL);
    if (e != ESP_OK) goto restore;
    e = esp_sleep_enable_gpio_wakeup();
    if (e != ESP_OK) goto restore;
    /* Keep flash/PSRAM supply: light sleep must preserve the in-memory state. */
    e = esp_sleep_pd_config(ESP_PD_DOMAIN_VDDSDIO, ESP_PD_OPTION_ON);
    if (e != ESP_OK) goto restore;
    ESP_LOGI(TAG, "light sleep; long Power hold wakes, pixels remain untouched");
    for (;;) {
        e = esp_light_sleep_start();
        if (e != ESP_OK) break;
        int64_t start = esp_timer_get_time();
        while (!gpio_get_level(PIN_POWER) && esp_timer_get_time() - start < INK_HOLD_MS * 1000)
            vTaskDelay(pdMS_TO_TICKS(10));
        if (!gpio_get_level(PIN_POWER)) {
            /* Consume the waking press so it cannot immediately sleep again. */
            while (!gpio_get_level(PIN_POWER)) vTaskDelay(pdMS_TO_TICKS(10));
            break;
        }
        /* Short press: do not restore input or light, simply re-enter sleep. */
    }
restore:
    gpio_wakeup_disable(PIN_POWER);
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    esp_err_t touch = ink_touch_enable(true);
    esp_err_t light = ink_light_sleep(false);
    if (touch != ESP_OK) ESP_LOGW(TAG, "touch resume: %s", esp_err_to_name(touch));
    if (light != ESP_OK) ESP_LOGW(TAG, "light resume: %s", esp_err_to_name(light));
    if (e == ESP_OK && touch != ESP_OK) e = touch;
    if (e == ESP_OK && light != ESP_OK) e = light;
    ESP_LOGI(TAG, "resumed without display refresh");
    return e;
}
