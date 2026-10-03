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

/* Consume a held press and release bounce. A wake click has no awake action. */
static void wait_power_released(void)
{
    int64_t released_at = esp_timer_get_time();
    while (esp_timer_get_time() - released_at < INK_DEBOUNCE_MS * 1000) {
        if (!gpio_get_level(PIN_POWER)) released_at = esp_timer_get_time();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

esp_err_t ink_sleep(void)
{
    /* Product caller acknowledges VM pause, flushes editor work and pauses input
     * capture before entry. Never suspend a task that may own driver/FS locks. */
    ESP_RETURN_ON_ERROR(ink_display_sleep(), TAG, "panel standby");
    ESP_RETURN_ON_ERROR(ink_light_sleep(true), TAG, "light off");
    esp_err_t e = ink_touch_enable(false);
    if (e != ESP_OK) goto restore;
    /* Finish the entering press before arming an active-low wake source. */
    wait_power_released();
    e = esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    if (e != ESP_OK) goto restore;
    e = gpio_wakeup_enable(PIN_POWER, GPIO_INTR_LOW_LEVEL);
    if (e != ESP_OK) goto restore;
    e = esp_sleep_enable_gpio_wakeup();
    if (e != ESP_OK) goto restore;
    /* Keep flash/PSRAM supply: light sleep must preserve the in-memory state. */
    e = esp_sleep_pd_config(ESP_PD_DOMAIN_VDDSDIO, ESP_PD_OPTION_ON);
    if (e != ESP_OK) goto restore;
    ESP_LOGI(TAG, "light sleep; one Power press wakes, pixels remain untouched");
    e = esp_light_sleep_start();
    ESP_LOGI(TAG,"light sleep returned: %s wake cause=%d",esp_err_to_name(e),(int)esp_sleep_get_wakeup_cause());
    if (e == ESP_OK) wait_power_released();

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

