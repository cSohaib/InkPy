#include "board.h"
#include "boot.h"
#include "orientation.h"
#include "input_capture.h"
#include "python_worker.h"
#include "ink_browser.h"
#include "ink_editor.h"
#include "ink_power.h"
#include "ink_reader.h"
#include "ink_math.h"
#include <sys/stat.h>
#include "sdkconfig.h"
#include <stdio.h>
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"

static ink_browser browser;
_Static_assert(CONFIG_ESP_MAIN_TASK_STACK_SIZE>=32768,"Reader/math needs >=32 KiB main stack; use build-browser.sh");
static ink_editor editor;
static ink_reader reader;
static bool reader_active,math_attempted,math_ready;
static void notice(const char *text);
static int render_math(const char *source,int display,unsigned pixels,uint8_t *bitmap,unsigned *w,unsigned *h,unsigned *baseline)
{
    ink_math_result r; int status=ink_math_render(source,display,(int)pixels,bitmap,&r);
    *w=(unsigned)r.width; *h=(unsigned)r.height; *baseline=(unsigned)r.baseline; return status;
}
static void indexing_progress(void) { vTaskDelay(1); }
static ink_python_worker python;
static ink_console console;
static bool editor_active,console_active,python_started,paused,closed;
static ink_power power_menu={.brightness=20,.warmth=50};
static uint32_t milliseconds(void) { return (uint32_t)(esp_timer_get_time()/1000); }
static void draw(uint8_t *frame)
{
    if(power_menu.open) ink_power_draw(&power_menu,frame);
    else if(editor_active) ink_editor_draw(&editor,frame);
    else if(console_active) ink_console_draw(&console,frame);
    else if(reader_active) {
        if(ink_reader_draw(&reader,frame)) {
            ink_reader_close(&reader); reader_active=false; notice(reader.error); ink_browser_draw(&browser,frame);
        }
    }
    else ink_browser_draw(&browser,frame);
    if(power_menu.night) for(unsigned i=0;i<PANEL_BYTES;i++) frame[i]^=255;
    ink_frame_rotate_180(frame);
}
static void notice(const char *text)
{ snprintf(browser.message,sizeof(browser.message),"%s",text); browser.view=INK_NOTICE; }
static void open_requests(void)
{
    if(editor_active||console_active||reader_active) return;
    if(browser.view==INK_OPEN_MARKDOWN) {
        struct stat st;
        if(!math_attempted&&!stat("/sd/inkpy/math",&st)&&S_ISDIR(st.st_mode)) {
            char error[160]; math_attempted=true; math_ready=ink_math_init("/sd/inkpy/math",error,sizeof(error))==0;
            if(!math_ready) ESP_LOGW("reader","math init: %s",error);
        }
        if(ink_reader_open(&reader,browser.selected,browser.root,math_ready?render_math:NULL,indexing_progress)) notice(reader.error);
        else reader_active=true;
        return;
    }
    if(browser.view==INK_EDIT_TEXT) {
        if(ink_editor_open(&editor,browser.selected)) notice(editor.error);
        else editor_active=true;
    } else if(browser.view==INK_OPEN_CONSOLE||browser.view==INK_EXECUTE_PYTHON) {
        bool file=browser.view==INK_EXECUTE_PYTHON;
        if(!python_started) {
            if(ink_python_worker_start(&python)!=ESP_OK) { notice("Cannot allocate Python session"); return; }
            python_started=true;
        } else if(!ink_python_worker_reopen(&python)) { notice("Python session still closing"); return; }
        console_active=true;
        if(file&&!ink_python_worker_submit(&python,browser.selected,true)) {
            ink_python_worker_close(&python);
            /* Return only after the worker has acknowledged cleanup. */
        }
        ink_python_worker_snapshot(&python,&console,&paused,&closed);
    }
}
static void home(bool long_press)
{
    if(power_menu.open) power_menu.open=false;
    else if(reader_active) ink_reader_home(&reader);
    else if(console_active) ink_python_worker_home(&python,long_press);
    else if(editor_active) {
        if(ink_editor_home(&editor,long_press)) { editor_active=false; ink_browser_home(&browser); }
    } else ink_browser_home(&browser);
}
static bool dispatch(const ink_event *event)
{
    unsigned x,y;
    switch(event->kind) {
    case INK_EVENT_PREV: case INK_EVENT_NEXT:
        if(power_menu.open||event->press!=INK_PRESS_SHORT) return false;
        if(editor_active) ink_editor_page(&editor,event->kind==INK_EVENT_PREV?-1:1);
        else if(reader_active) ink_reader_page(&reader,event->kind==INK_EVENT_PREV?-1:1);
        else if(console_active) ink_python_worker_page(&python,event->kind==INK_EVENT_PREV?-1:1);
        else ink_browser_page(&browser,event->kind==INK_EVENT_PREV?-1:1);
        return true;
    case INK_EVENT_HOME: home(event->press==INK_PRESS_LONG); return true;
    case INK_EVENT_TOUCH:
        if(event->x>=800||event->y>=480) return false;
        ink_panel_to_ui(event->x,event->y,&x,&y);
        if(event->press==INK_PRESS_LONG) {
            if(!power_menu.open&&!editor_active&&!console_active&&!reader_active) return ink_browser_long_press(&browser,x,y);
            return false;
        }
        if(power_menu.open) {
            if(ink_power_tap(&power_menu,x,y)==INK_POWER_LIGHT &&
               ink_light_set(power_menu.brightness,power_menu.warmth,power_menu.on)!=ESP_OK)
                snprintf(power_menu.message,sizeof(power_menu.message),"Light update failed");
        } else if(editor_active) {
            if(ink_editor_tap(&editor,x,y)) { editor_active=false; ink_browser_home(&browser); }
        } else if(reader_active) {
            if(ink_reader_tap(&reader,x,y)) { ink_reader_close(&reader); reader_active=false; ink_browser_home(&browser); }
        } else if(console_active) ink_python_worker_tap(&python,x,y);
        else ink_browser_tap(&browser,x,y);
        return true;
    case INK_EVENT_POWER:
        if(event->press==INK_PRESS_SHORT) {
            power_menu.open=!power_menu.open; power_menu.message[0]=0;
            if(power_menu.open) {
                struct tm time;
                if(ink_rtc_read(&time)==ESP_OK) strftime(power_menu.time,sizeof(power_menu.time),"%Y-%m-%d %H:%M",&time);
                else power_menu.time[0]=0;
            }
            return true;
        }
        if(event->press==INK_PRESS_DOUBLE) {
            power_menu.on=!power_menu.on;
            ink_light_set(power_menu.brightness,power_menu.warmth,power_menu.on);
            return power_menu.open;
        }
    }
    return false;
}
void app_main(void)
{
    const char *tag="browser";
    ink_boot_report();
    esp_err_t e=ink_board_init();
    if(e!=ESP_OK) { ESP_LOGE(tag,"board: %s",esp_err_to_name(e)); return; }
    e=ink_display_init();
    if(e!=ESP_OK) { ESP_LOGE(tag,"display: %s",esp_err_to_name(e)); return; }
    e=ink_sd_mount(); ESP_LOGI(tag,"SD: %s",esp_err_to_name(e));
    ink_browser_init(&browser,"/sd");
    uint8_t *frame=heap_caps_malloc(PANEL_BYTES,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    if(!frame) { ESP_LOGE(tag,"frame allocation failed"); return; }
    e=ink_capture_start();
    if(e!=ESP_OK) { heap_caps_free(frame); ESP_LOGE(tag,"input: %s",esp_err_to_name(e)); return; }
    draw(frame);
    if(ink_display_frame(frame)!=ESP_OK) { heap_caps_free(frame); return; }
    e=ink_boot_confirm();
    if(e!=ESP_OK) ESP_LOGE(tag,"boot confirmation: %s",esp_err_to_name(e));
    uint32_t revision=0,last_draw=milliseconds(),sleep_at=0,last_sleep=0;
    unsigned dropped=0;
    bool dirty=false,sleep_requested=false;
    for(;;) {
        ink_event event;
        /* Capture continues while this task draws. Drain FIFO and paint once,
         * retaining repeated keys rather than polling only between refreshes. */
        unsigned batch=0;
        while(ink_capture_next(&event,batch?0:pdMS_TO_TICKS(10))) {
            if(event.kind==INK_EVENT_POWER&&event.press==INK_PRESS_LONG) {
                sleep_requested=true; sleep_at=milliseconds();
                if(console_active) ink_python_worker_pause(&python,true);
            } else dirty|=dispatch(&event);
            open_requests();
            if(++batch==256) break;
        }
        uint32_t now=milliseconds();
        if(console_active) {
            uint32_t next_revision=ink_python_worker_snapshot(&python,&console,&paused,&closed);
            if(next_revision!=revision) { revision=next_revision; dirty=true; }
            if(closed) { console_active=false; ink_browser_home(&browser); dirty=true; }
        }
        unsigned lost=ink_capture_dropped();
        if(lost!=dropped) {
            ESP_LOGW(tag,"input queue overflow: %u events",lost-dropped); dropped=lost;
        }
        if(!sleep_requested&&now-ink_capture_activity()>=300000&&now-last_sleep>=300000&&!(console_active&&console.busy)) {
            sleep_requested=true; sleep_at=now;
            if(console_active) ink_python_worker_pause(&python,true);
        }
        if(sleep_requested) {
            if(!console_active||paused) {
                if(editor_active&&fflush(editor.work)) {
                    snprintf(editor.error,sizeof(editor.error),"Cannot flush before sleep"); dirty=true;
                } else {
                    ink_capture_pause();
                    e=ink_sleep(); ESP_LOGI(tag,"sleep: %s",esp_err_to_name(e));
                    ink_capture_resume();
                }
                if(console_active) ink_python_worker_pause(&python,false);
                sleep_requested=false; last_sleep=milliseconds();
            } else if(now-sleep_at>=5000) {
                /* Native blocking extensions must reach a safe point. Never
                 * suspend a task that could hold filesystem/driver locks. */
                ink_python_worker_pause(&python,false); sleep_requested=false; last_sleep=now;
                ESP_LOGW(tag,"Python did not acknowledge sleep; staying awake");
            }
        }
        if(dirty&&now-last_draw>=250) {
            draw(frame);
            if(ink_display_frame(frame)!=ESP_OK) { ESP_LOGE(tag,"display failed"); break; }
            last_draw=milliseconds(); dirty=false;
        }
    }
    if(reader_active) ink_reader_close(&reader);
    if(editor_active) ink_editor_discard(&editor);
    if(console_active) ink_python_worker_close(&python);
    ink_capture_pause(); heap_caps_free(frame);
}
