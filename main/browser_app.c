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
#include "ink_font.h"
#include "ink_epub.h"
#include "ink_icons.h"
#include "ink_ui.h"
#include "python_port/native.h"
#include <sys/stat.h>
#include "sdkconfig.h"
#include <stdio.h>
#include <string.h>
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"

static ink_browser browser;
_Static_assert(CONFIG_ESP_MAIN_TASK_STACK_SIZE>=32768,"Reader/math needs >=32 KiB main stack; use build-browser.sh");
static ink_editor editor;
static ink_reader reader;
static ink_dict dictionary;
static bool reader_active,math_attempted,math_ready;
static void notice(const char *text);
static int render_math(const char *source,int display,unsigned pixels,uint8_t *bitmap,unsigned *w,unsigned *h,unsigned *baseline)
{
    if(!math_attempted) {
        char error[160]; math_attempted=true;
        math_ready=ink_math_init("/sd/inkpy/math",error,sizeof(error))==0;
        if(!math_ready) ESP_LOGW("reader","math init: %s",error);
    }
    if(!math_ready) { *w=*h=*baseline=0; return -1; }
    ink_math_result r; int status=ink_math_render(source,display,(int)pixels,bitmap,&r);
    if(status) ESP_LOGW("reader","math render: %s",r.error);
    *w=(unsigned)r.width; *h=(unsigned)r.height; *baseline=(unsigned)r.baseline; return status;
}
static uint8_t *loading_frame;
static bool loading;
static bool loading_painted;
static void loading_pixel(uint8_t *f,unsigned x,unsigned y)
{
    if(x>=480||y>=800)return;
    unsigned dx=799-y,dy=x;f[dy*100+dx/8]&=(uint8_t)~(0x80>>(dx%8));
}
static void indexing_progress(void)
{
    uint32_t now=(uint32_t)(esp_timer_get_time()/1000);
    if(loading){ink_event event;while(ink_capture_next(&event,0))if(event.kind==INK_EVENT_HOME&&event.press==INK_PRESS_LONG)ink_epub_cancel();}
    if(loading&&loading_frame&&!loading_painted) {
        memset(loading_frame,255,PANEL_BYTES);
        ink_icon_size(loading_frame,208,336,ICON_WAIT,64,loading_pixel);
        ink_frame_rotate_180(loading_frame);
        ink_display_update(loading_frame,false);loading_painted=true;
    }
    /* Do not add one RTOS tick of delay for every decompression chunk. */
    static uint32_t yielded;
    if(now-yielded>=100){vTaskDelay(1);yielded=(uint32_t)(esp_timer_get_time()/1000);}
    else taskYIELD();
}
static void loading_begin(void){ink_epub_reset_cancel();loading=true;loading_painted=false;}
static void loading_end(void){loading=false;}

static ink_python_worker python;
static ink_console console;
static bool editor_active,console_active,python_started,paused,closed,home_requested;
static ink_power power_menu={.brightness=20,.warmth=50};
static bool full_refresh;
static bool dictionary_active,dictionary_input;
static ink_keyboard lookup_keyboard;
static ink_reader lookup_popup;
static void lookup_submit(void)
{
    lookup_popup.dictionary=&dictionary;
    lookup_popup.view=INK_READER_DEFINITION;
    lookup_popup.error[0]=0;
    dictionary_input=false;
    loading_begin();ink_dict_lookup(&dictionary,lookup_popup.word);loading_end();
}
static void lookup_tap(unsigned x,unsigned y)
{
    if(!dictionary_input) {
        loading_begin();ink_reader_tap(&lookup_popup,x,y);loading_end();
        if(lookup_popup.view==INK_READER_PAGE)dictionary_input=true;
        if(lookup_popup.view==INK_READER_DEFINITION&&!lookup_popup.word[0])dictionary_input=true;
        return;
    }
    if(y>=712) {
        lookup_popup.view=INK_READER_DICTIONARIES;lookup_popup.dictionary=&dictionary;
        ink_dict_catalog(&dictionary,0);dictionary_input=false;return;
    }
    int key=ink_keyboard_tap(&lookup_keyboard,x,y);
    size_t n=strlen(lookup_popup.word);
    if(key==INK_KEY_ENTER) {if(n)lookup_submit();}
    else if(key==INK_KEY_DELETE) {if(n)lookup_popup.word[n-1]=0;}
    else if(key>0&&key<128&&n+1<sizeof(lookup_popup.word)) {
        lookup_popup.word[n]=(char)key;lookup_popup.word[n+1]=0;
    }
}
static void power_header(void)
{
    struct tm t;
    if(ink_rtc_read(&t)==ESP_OK) strftime(power_menu.time,sizeof(power_menu.time),"%Y-%m-%d %H:%M",&t);
    else power_menu.time[0]=0;
    unsigned percent; bool charging;
    if(ink_battery_read(&percent,&charging)==ESP_OK)
        snprintf(power_menu.battery,sizeof(power_menu.battery),"%u%%",percent);
    else strcpy(power_menu.battery,"--");
}
static void power_action(int action)
{
    if(action==INK_POWER_LIGHT&&ink_light_set(power_menu.brightness,power_menu.warmth,power_menu.on)!=ESP_OK)
        strcpy(power_menu.message,"Light update failed");
    else if(action==INK_POWER_REFRESH) full_refresh=true;
    else if(action==INK_POWER_DICTIONARY) {
        dictionary_active=dictionary_input=true;
        memset(&lookup_popup,0,sizeof(lookup_popup));lookup_keyboard=(ink_keyboard){0};
    }
}
static uint32_t milliseconds(void) { return (uint32_t)(esp_timer_get_time()/1000); }
static void draw(uint8_t *frame)
{
    if(power_menu.open) ink_power_draw(&power_menu,frame);
    else if(dictionary_active) {
        if(dictionary_input) ink_lookup_input_draw(lookup_popup.word,&lookup_keyboard,frame);
        else ink_reader_draw(&lookup_popup,frame);
    }
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
{ ESP_LOGW("ui","%s",text); snprintf(browser.message,sizeof(browser.message),"%s",text); browser.view=INK_NOTICE; }
static void open_requests(void)
{
    if(dictionary_active||editor_active||console_active||reader_active) return;
    if(browser.view==INK_OPEN_MARKDOWN) {
        struct stat st;
        bool assets=!stat("/sd/inkpy/math",&st)&&S_ISDIR(st.st_mode);
        loading_begin();
        if(ink_reader_open(&reader,browser.selected,browser.root,assets?render_math:NULL,indexing_progress)) notice(reader.error);
        else { reader.dictionary=&dictionary; reader_active=true; }
        loading_end();
        if(ink_epub_cancelled()){ink_reader_close(&reader);reader_active=false;ink_browser_root(&browser);}
        return;
    }
    if(browser.view==INK_EDIT_TEXT) {
        /* Release cached source streams before editing dictionary files. */
        size_t dictionary_root=strlen(dictionary.root);
        if(dictionary_root&&!strncmp(browser.selected,dictionary.root,dictionary_root)&&browser.selected[dictionary_root]=='/')
            ink_dict_close(&dictionary);
        if(ink_editor_open(&editor,browser.selected)) notice(editor.error);
        else editor_active=true;
    } else if(browser.view==INK_OPEN_CONSOLE||browser.view==INK_EXECUTE_PYTHON) {
        bool file=browser.view==INK_EXECUTE_PYTHON;
        if(!python_started) {
            if(ink_python_worker_start(&python)!=ESP_OK) { notice("Cannot allocate Python session"); return; }
            python_started=true;
        }
        console_active=true;
        if(file&&!ink_python_worker_submit(&python,browser.selected,true)) {
            ink_python_worker_close(&python);
            /* Return only after the worker has acknowledged cleanup. */
        }
        ink_python_worker_snapshot(&python,&console,&paused,&closed,UINT32_MAX);
    }
}
static void home(bool long_press)
{
    if(long_press) {
        power_menu.open=false; dictionary_active=false;
        if(reader_active) { ink_reader_close(&reader); reader_active=false; }
        if(editor_active) { ink_editor_discard(&editor); editor_active=false; }
        if(console_active) {
            home_requested=true; ink_python_worker_close(&python);
        } else ink_browser_root(&browser);
        return;
    }
    if(power_menu.open) power_menu.open=false;
    else if(dictionary_active) {
        if(dictionary_input)dictionary_active=false;
        else if(lookup_popup.view==INK_READER_DICTIONARIES)lookup_popup.view=INK_READER_DEFINITION;
        else dictionary_input=true;
    }
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
        if(event->press!=INK_PRESS_SHORT) return false;
        if(power_menu.open) return true;
        if(dictionary_active) {if(!dictionary_input)ink_reader_page(&lookup_popup,event->kind==INK_EVENT_PREV?-1:1);return true;}
        if(editor_active) ink_editor_page(&editor,event->kind==INK_EVENT_PREV?-1:1);
        else if(reader_active){loading_begin();ink_reader_page(&reader,event->kind==INK_EVENT_PREV?-1:1);loading_end();if(ink_epub_cancelled())home(true);}
        else if(console_active) ink_python_worker_page(&python,event->kind==INK_EVENT_PREV?-1:1);
        else ink_browser_page(&browser,event->kind==INK_EVENT_PREV?-1:1);
        return true;
    case INK_EVENT_HOME: home(event->press==INK_PRESS_LONG); return true;
    case INK_EVENT_TOUCH:
        if(event->x>=800||event->y>=480) return false;
        ink_panel_to_ui(event->x,event->y,&x,&y);
        if(reader_active&&!power_menu.open&&!dictionary_active&&reader.view==INK_READER_PAGE&&reader.landscape) {
            x=event->x; y=event->y;
        }
        if(event->press==INK_PRESS_LONG) {
            if(!power_menu.open&&!dictionary_active&&!editor_active&&!console_active&&!reader_active) return ink_browser_long_press(&browser,x,y);
            return false;
        }
        if(power_menu.open) {
            power_action(ink_power_tap(&power_menu,x,y));
        } else if(dictionary_active) lookup_tap(x,y);
        else if(editor_active) {
            if(ink_editor_tap(&editor,x,y)) { editor_active=false; ink_browser_home(&browser); }
        } else if(reader_active) {
            loading_begin();int action=ink_reader_tap(&reader,x,y);
            if(action==INK_READER_CLOSE) { ink_reader_close(&reader); reader_active=false; ink_browser_home(&browser); }
            else if(action==INK_READER_ROTATE&&ink_reader_rotate(&reader,browser.selected,browser.root,
                render_math,indexing_progress)) {
                ink_reader_close(&reader); reader_active=false; notice(reader.error);
            }
            loading_end();
            if(ink_epub_cancelled())home(true);
        } else if(console_active) ink_python_worker_tap(&python,x,y);
        else ink_browser_tap(&browser,x,y);
        return true;
    case INK_EVENT_POWER:
        if(event->press==INK_PRESS_SHORT) {
            power_menu.open=!power_menu.open; power_menu.message[0]=0;
            if(power_menu.open) {
                power_header();
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
    if(ink_font_init()) { ESP_LOGE(tag,"default font failed"); return; }
    ink_dict_init(&dictionary,"/sd",indexing_progress);
    uint8_t *frame=heap_caps_malloc(PANEL_BYTES,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    if(!frame) { ESP_LOGE(tag,"frame allocation failed"); return; }
    loading_frame=frame;
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
            uint32_t next_revision=ink_python_worker_snapshot(&python,&console,&paused,&closed,revision);
            if(next_revision!=revision) { revision=next_revision; dirty=true; }
            if(closed) {
                ink_python_worker_release(&python); python_started=false; console_active=false;
                if(home_requested) { ink_browser_root(&browser); home_requested=false; }
                else ink_browser_home(&browser);
                dirty=true;
            }
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
                    int network=ink_python_network_sleep(true);
                    e=network?ESP_FAIL:ink_sleep(); ESP_LOGI(tag,"sleep: %s",esp_err_to_name(e));
                    if(ink_python_network_sleep(false)) ESP_LOGW(tag,"Wi-Fi resume failed");
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
            if(ink_display_update(frame,full_refresh)!=ESP_OK) { ESP_LOGE(tag,"display failed"); break; }
            full_refresh=false;
            last_draw=milliseconds(); dirty=false;
        }
    }
    if(reader_active) ink_reader_close(&reader);
    ink_dict_close(&dictionary);
    if(editor_active) ink_editor_discard(&editor);
    if(console_active) ink_python_worker_close(&python);
    ink_capture_pause(); heap_caps_free(frame);
}
