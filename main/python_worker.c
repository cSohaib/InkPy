#include "python_worker.h"
#include "ink_python.h"
#include "esp_heap_caps.h"
#include <string.h>
#include <strings.h>
#include <stdlib.h>
enum { VM_HEAP=256*1024,VM_STACK=48*1024 };
static void lock(ink_python_worker *s) { xSemaphoreTake(s->lock,portMAX_DELAY); }
static void unlock(ink_python_worker *s) { xSemaphoreGive(s->lock); }
static bool control(void *context)
{
    ink_python_worker *s=context;
    for(;;) {
        lock(s); bool stop=s->stop||s->closing,wait=s->pause&&!stop;
        s->paused=wait; unlock(s);
        if(!wait) {
            /* Let the core's idle task feed its watchdog during endless bytecode.
             * This schedules cooperatively; it never imposes a runtime limit. */
            TickType_t now=xTaskGetTickCount();
            if(!stop && now-s->last_yield>=pdMS_TO_TICKS(10)) {
                s->last_yield=now; vTaskDelay(1);
            }
            return stop;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
static void output(void *context,const char *bytes,size_t n)
{ ink_python_worker *s=context; lock(s); ink_console_output(&s->console,bytes,n); s->revision++; unlock(s); }
static void worker(void *context)
{
    ink_python_worker *s=context; int stack_top;
    ink_python_callbacks(control,output,s);
    for(;;) {
        lock(s); bool opening=s->opening; s->opening=false; unlock(s);
        if(!opening) { ulTaskNotifyTake(pdTRUE,portMAX_DELAY); continue; }
        ink_python_init(s->heap,VM_HEAP,&stack_top);
        for(;;) {
            lock(s);
            if(s->closing) { unlock(s); break; }
            bool run=s->queued&&!s->pause;
            s->paused=s->pause;
            if(run) { s->queued=false; s->paused=false; }
            unlock(s);
            if(!run) { ulTaskNotifyTake(pdTRUE,portMAX_DELAY); continue; }
            bool more=!s->file_job&&ink_python_more(s->command);
            int result=s->file_job?ink_python_file(s->path):more?0:ink_python_text(s->command,true);
            if(result==2) { ink_python_close(); ink_python_init(s->heap,VM_HEAP,&stack_top); more=false; }
            lock(s);
            if(result==2) ink_console_output(&s->console,"Stopped; Python reset\n",22);
            /* Closing wins over completion; no new jobs until Close acknowledgment. */
            ink_console_completed(&s->console,more); s->stop=false; s->paused=false; s->revision++;
            unlock(s);
        }
        ink_python_close();
        lock(s); s->closed=true; s->paused=false; s->queued=false;
        s->console.busy=false; s->revision++; unlock(s);
    }
}
esp_err_t ink_python_worker_start(ink_python_worker *s)
{
    memset(s,0,sizeof(*s)); ink_console_init(&s->console);
    s->lock=xSemaphoreCreateMutex();
    s->heap=heap_caps_malloc(VM_HEAP,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    if(!s->lock||!s->heap) {
        if(s->lock) vSemaphoreDelete(s->lock);
        free(s->heap); memset(s,0,sizeof(*s)); return ESP_ERR_NO_MEM;
    }
    s->opening=true;
    /* ESP-IDF stack size is bytes; xTaskCreate allocates an internal task stack. */
    if(xTaskCreatePinnedToCore(worker,"inkpy-vm",VM_STACK,s,2,&s->task,1)!=pdPASS) {
        vSemaphoreDelete(s->lock); free(s->heap); memset(s,0,sizeof(*s)); return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}
bool ink_python_worker_submit(ink_python_worker *s,const char *text,bool file)
{
    size_t n=strlen(text); lock(s);
    bool valid=!s->closed&&!s->closing&&!s->console.busy&&!s->queued;
    if(file) {
        const char *ext=strrchr(text,'.');
        valid=valid&&n<sizeof(s->path)&&ext&&!strcasecmp(ext,".py");
    } else valid=valid&&n<sizeof(s->command);
    if(valid) {
        memcpy(file?s->path:s->command,text,n+1); s->file_job=file;
        if(!file) { memcpy(s->console.input,text,n+1); s->console.used=n; }
        s->console.busy=true; s->queued=true; s->revision++;
    }
    unlock(s); if(valid) xTaskNotifyGive(s->task); return valid;
}
void ink_python_worker_stop(ink_python_worker *s)
{
    lock(s);
    if(s->queued) { s->queued=false; ink_console_completed(&s->console,false); }
    else if(s->console.busy) s->stop=true;
    else ink_console_completed(&s->console,false);
    s->pause=false; s->revision++; unlock(s); xTaskNotifyGive(s->task);
}
void ink_python_worker_pause(ink_python_worker *s,bool paused)
{ lock(s); s->pause=paused; if(!paused) s->paused=false; unlock(s); xTaskNotifyGive(s->task); }
void ink_python_worker_close(ink_python_worker *s)
{ lock(s); s->closing=true; s->revision++; unlock(s); xTaskNotifyGive(s->task); }
bool ink_python_worker_reopen(ink_python_worker *s)
{
    lock(s); bool ready=s->closed;
    if(ready) { ink_console_init(&s->console); s->closing=s->closed=s->stop=s->pause=s->paused=false; s->opening=true; s->revision++; }
    unlock(s); if(ready) xTaskNotifyGive(s->task); return ready;
}
uint32_t ink_python_worker_snapshot(ink_python_worker *s,ink_console *console,bool *paused,bool *closed)
{ lock(s); *console=s->console; *paused=s->paused; *closed=s->closed; uint32_t revision=s->revision; unlock(s); return revision; }

void ink_python_worker_tap(ink_python_worker *s,unsigned x,unsigned y)
{
    lock(s); int action=INK_CONSOLE_NONE; bool submit=false;
    if(!s->closing&&!s->closed) {
        if(s->console.menu) action=ink_console_tap(&s->console,x,y);
        else if(!s->console.busy) {
            int key=ink_keyboard_tap(&s->console.keyboard,x,y>=INK_CONSOLE_KEYBOARD_OFFSET?y-INK_CONSOLE_KEYBOARD_OFFSET:0);
            if(ink_console_key(&s->console,key)) {
                memcpy(s->command,s->console.input,s->console.used+1);
                const char *line=strrchr(s->command,'\n'); line=line?line+1:s->command;
                ink_console_output(&s->console,s->console.more?"... ":">>> ",4);
                ink_console_output(&s->console,line,strlen(line));
                ink_console_output(&s->console,"\n",1);
                s->file_job=false; s->queued=true; submit=true;
            }
        }
        s->revision++;
    }
    unlock(s);
    if(submit) xTaskNotifyGive(s->task);
    if(action==INK_CONSOLE_STOP) ink_python_worker_stop(s);
    if(action==INK_CONSOLE_CLOSE) ink_python_worker_close(s);
}
void ink_python_worker_home(ink_python_worker *s,bool long_press)
{
    lock(s); int action=ink_console_home(&s->console,long_press); s->revision++; unlock(s);
    if(action==INK_CONSOLE_CLOSE) ink_python_worker_close(s);
}
void ink_python_worker_page(ink_python_worker *s,int direction)
{ lock(s); ink_console_page(&s->console,direction); s->revision++; unlock(s); }
