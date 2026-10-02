#include "board.h"
#include "input.h"
#include "pins.h"
#include "ink_browser.h"
#include "ink_editor.h"
#include "sdkconfig.h"
#include <stdio.h>
#include <stdlib.h>
#include "driver/gpio.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static ink_browser browser;
_Static_assert(CONFIG_ESP_MAIN_TASK_STACK_SIZE>=16384,"Editor needs >=16 KiB main stack; update sdkconfig");
static ink_editor editor;
static bool editor_active;
static void draw(uint8_t *frame)
{ if(editor_active) ink_editor_draw(&editor,frame); else ink_browser_draw(&browser,frame); }
static void open_editor(void)
{
    if(editor_active || browser.view!=INK_EDIT_TEXT) return;
    if(ink_editor_open(&editor,browser.selected)) {
        snprintf(browser.message,sizeof(browser.message),"%s",editor.error); browser.view=INK_NOTICE;
    } else editor_active=true;
}
static void home(bool long_press)
{
    if(editor_active) {
        if(ink_editor_home(&editor,long_press)) { editor_active=false; ink_browser_home(&browser); }
    } else ink_browser_home(&browser);
}
void app_main(void)
{
    const char *tag="browser";
    esp_err_t e=ink_board_init();
    if(e!=ESP_OK) { ESP_LOGE(tag,"board: %s",esp_err_to_name(e)); return; }
    e=ink_display_init();
    if(e!=ESP_OK) { ESP_LOGE(tag,"display: %s",esp_err_to_name(e)); return; }
    e=ink_sd_mount();
    ESP_LOGI(tag,"SD: %s",esp_err_to_name(e));
    /* New file and editor write to SD; no diagnostic SD write probe. */
    ink_browser_init(&browser,"/sd");
    uint8_t *frame=heap_caps_malloc(48000,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    if(!frame) { ESP_LOGE(tag,"frame allocation failed"); return; }
    draw(frame);
    if(ink_display_frame(frame)!=ESP_OK) { heap_caps_free(frame); return; }
    ink_button_t prev={0},next={0},home={0},power={0},contact={0};
    ink_touch_t touch={0};
    unsigned origin_x=0,origin_y=0;
    bool moved=false,light_on=false;
    uint32_t last_activity=(uint32_t)(esp_timer_get_time()/1000);
    for(;;) {
        uint32_t now=(uint32_t)(esp_timer_get_time()/1000);
        bool fresh=false,changed=false; ink_touch_t t;
        if(ink_touch_read(&t,&fresh)==ESP_OK && fresh) {
            if(!touch.contacts && t.contacts) { origin_x=t.x; origin_y=t.y; moved=false; }
            if(t.contacts>1 || (t.contacts && (abs((int)t.x-(int)origin_x)>12 || abs((int)t.y-(int)origin_y)>12))) moved=true;
            touch=t;
        }
        bool p=!gpio_get_level(PIN_POWER),a=!gpio_get_level(PIN_PREV),b=!gpio_get_level(PIN_NEXT);
        if(p||a||b||touch.home||touch.contacts) last_activity=now;
        ink_press_t pe=ink_button_update(&power,p,now,true);
        ink_press_t ae=ink_button_update(&prev,a,now,false);
        ink_press_t be=ink_button_update(&next,b,now,false);
        ink_press_t he=ink_button_update(&home,touch.home,now,false);
        ink_press_t te=ink_button_update(&contact,touch.contacts!=0,now,false);
        if(ae==INK_PRESS_SHORT || be==INK_PRESS_SHORT) {
            int direction=ae==INK_PRESS_SHORT?-1:1;
            if(editor_active) { ink_editor_page(&editor,direction); changed=true; }
            else changed|=ink_browser_page(&browser,direction);
        }
        if(he==INK_PRESS_SHORT || he==INK_PRESS_LONG) { home(he==INK_PRESS_LONG); changed=true; }
        if(te==INK_PRESS_SHORT && !moved && origin_x<800 && origin_y<480) {
            if(editor_active) {
                if(ink_editor_tap(&editor,origin_y,799-origin_x)) { editor_active=false; ink_browser_home(&browser); }
                changed=true;
            } else changed|=ink_browser_tap(&browser,origin_y,799-origin_x);
        }
        if(te==INK_PRESS_LONG && !editor_active && !moved && origin_x<800 && origin_y<480)
            changed|=ink_browser_long_press(&browser,origin_y,799-origin_x);
        if(changed) open_editor();
        if(pe==INK_PRESS_SHORT) {
            snprintf(browser.message,sizeof(browser.message),"Power menu: coming later");
            browser.view=INK_NOTICE; changed=true;
        }
        if(pe==INK_PRESS_DOUBLE) { light_on=!light_on; ink_light_set(20,50,light_on); }
        if(pe==INK_PRESS_LONG || now-last_activity>=300000) {
            if(editor_active && fflush(editor.work)) {
                snprintf(editor.error,sizeof(editor.error),"Cannot flush before sleep");
                last_activity=now;
                draw(frame); ink_display_frame(frame); vTaskDelay(pdMS_TO_TICKS(10)); continue;
            }
            e=ink_sleep(); ESP_LOGI(tag,"sleep: %s",esp_err_to_name(e));
            prev=next=home=power=contact=(ink_button_t){0}; touch=(ink_touch_t){0};
            last_activity=(uint32_t)(esp_timer_get_time()/1000); changed=false;
        }
        if(changed) {
            draw(frame);
            if(ink_display_frame(frame)!=ESP_OK) { ESP_LOGE(tag,"display failed"); break; }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    if(editor_active) ink_editor_discard(&editor);
    heap_caps_free(frame);
}
