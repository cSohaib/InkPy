#include "board.h"
#include "input.h"
#include "pins.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_check.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Consume the entering press before cutting the battery latch or arming wake. */
void ink_power_released(void)
{
    int64_t released_at=esp_timer_get_time();
    while(esp_timer_get_time()-released_at<INK_DEBOUNCE_MS*1000) {
        if(!gpio_get_level(PIN_POWER))released_at=esp_timer_get_time();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
static void hold_off(int pin,int level)
{
    gpio_hold_dis(pin);gpio_set_level(pin,level);
    gpio_set_direction(pin,GPIO_MODE_OUTPUT);gpio_hold_en(pin);
}
esp_err_t ink_sleep(void)
{
    /* Caller has flushed its checkpoint and closed all filesystem/VM owners. */
    ESP_RETURN_ON_ERROR(ink_display_sleep(),"power","panel standby");
    ESP_RETURN_ON_ERROR(ink_light_sleep(true),"power","light off");
    ESP_RETURN_ON_ERROR(ink_touch_enable(false),"power","touch off");
    ink_power_released();
    ESP_RETURN_ON_ERROR(esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL),"power","wake reset");
    ESP_RETURN_ON_ERROR(esp_sleep_enable_ext0_wakeup(PIN_POWER,0),"power","power wake");
    rtc_gpio_pullup_en(PIN_POWER);rtc_gpio_pulldown_dis(PIN_POWER);
    /* Prevent SD signal pins from feeding the unpowered card. */
    int sd[]={PIN_SD_CLK,PIN_SD_CMD,PIN_SD_D0};
    for(unsigned i=0;i<sizeof(sd)/sizeof(sd[0]);i++) {
        gpio_set_direction(sd[i],GPIO_MODE_INPUT);gpio_set_pull_mode(sd[i],GPIO_FLOATING);
    }
    hold_off(PIN_COOL,0);hold_off(PIN_WARM,0);
    hold_off(PIN_SD_ENABLE,1);hold_off(PIN_TOUCH_ENABLE,1);
    gpio_deep_sleep_hold_en();
    /* GPIO1 is the X4 Pro keep-alive. Battery cuts here; USB may keep the
     * chip powered, so deep sleep below supplies the same cold-start behavior. */
    hold_off(PIN_RAIL,0);
    esp_deep_sleep_start();
    return ESP_FAIL; /* deep sleep never returns */
}
