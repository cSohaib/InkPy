#pragma once
#include "ink_console.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
/* One per boot. Worker/heap remain reserved; Close destroys VM/globals and
 * acknowledges cleanup. The idle worker can open a fresh session later. */
typedef struct {
    SemaphoreHandle_t lock;
    TaskHandle_t task;
    void *heap;
    uint32_t revision;
    TickType_t last_yield; /* VM-owned; bounded scheduling latency, no timeout. */
    ink_console console;
    char command[INK_CONSOLE_INPUT],path[512];
    bool opening,queued,file_job,stop,pause,paused,closing,closed,input_ready;
} ink_python_worker;
esp_err_t ink_python_worker_start(ink_python_worker *s);
bool ink_python_worker_submit(ink_python_worker *s,const char *text,bool file);
void ink_python_worker_stop(ink_python_worker *s);
void ink_python_worker_pause(ink_python_worker *s,bool paused);
void ink_python_worker_close(ink_python_worker *s);
bool ink_python_worker_reopen(ink_python_worker *s);
uint32_t ink_python_worker_snapshot(ink_python_worker *s,ink_console *console,bool *paused,bool *closed);

void ink_python_worker_tap(ink_python_worker *s,unsigned x,unsigned y);
void ink_python_worker_home(ink_python_worker *s,bool long_press);
void ink_python_worker_page(ink_python_worker *s,int direction);
