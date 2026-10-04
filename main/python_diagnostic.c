#include "python_worker.h"
#include "esp_log.h"
#include "freertos/task.h"
#include <string.h>
static ink_python_worker session;
static ink_console snapshot;
/* Test watchdogs below are diagnostic checks, never a Python runtime limit. */
static bool wait_state(bool busy,bool want_paused,bool want_closed)
{
    for(unsigned i=0;i<500;i++) {
        bool paused,closed; ink_python_worker_snapshot(&session,&snapshot,&paused,&closed,UINT32_MAX);
        if(snapshot.busy==busy&&paused==want_paused&&closed==want_closed) return true;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    return false;
}
#define CHECK(expr) do { if(!(expr)) { ESP_LOGE("python-test","FAILED: %s",#expr); return; } } while(0)
void app_main(void)
{
    CHECK(ink_python_worker_start(&session)==ESP_OK);
    CHECK(ink_python_worker_submit(&session,"6*7",false));
    CHECK(wait_state(false,false,false));
    ESP_LOGI("python-test","expression output: %s",ink_console_line(&snapshot,0));
    CHECK(!strcmp(ink_console_line(&snapshot,0),"42"));
    CHECK(ink_python_worker_submit(&session,"while True:\n try:\n  pass\n except BaseException:\n  pass\n\n",false));
    vTaskDelay(pdMS_TO_TICKS(100));
    ink_python_worker_pause(&session,true); CHECK(wait_state(true,true,false));
    ink_python_worker_stop(&session); CHECK(wait_state(false,false,false));
    CHECK(ink_python_worker_submit(&session,"import gc; gc.collect()",false));
    CHECK(wait_state(false,false,false));
    ink_python_worker_close(&session); CHECK(wait_state(false,false,true));
    ink_python_worker_release(&session);
    CHECK(ink_python_worker_start(&session)==ESP_OK);
    CHECK(ink_python_worker_submit(&session,"import gc; gc.collect(); 6*7",false));
    CHECK(wait_state(false,false,false));
    CHECK(!strcmp(ink_console_line(&snapshot,0),"42"));
    ink_python_worker_close(&session); CHECK(wait_state(false,false,true));
    unsigned stack_free=(unsigned)uxTaskGetStackHighWaterMark(session.task);
    ink_python_worker_release(&session);
    ESP_LOGI("python-test","PASS: execute, pause, hard stop/reset, Close cleanup, reopen; stack free=%u bytes",
             stack_free);
}
