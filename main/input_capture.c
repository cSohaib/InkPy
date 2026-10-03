#include "input_capture.h"
#include "board.h"
#include "pins.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <stdatomic.h>
enum { PAUSE_REQUEST=1,PAUSED=2 };
static QueueHandle_t queue;
static EventGroupHandle_t state;
static TaskHandle_t task;
static atomic_uint activity,dropped;
static void capture(void *unused)
{
    (void)unused; ink_event_state events={0}; ink_touch_t touch={0};
    for(;;) {
        if(xEventGroupGetBits(state)&PAUSE_REQUEST) {
            events=(ink_event_state){0}; touch=(ink_touch_t){0};
            xEventGroupSetBits(state,PAUSED);
            while(xEventGroupGetBits(state)&PAUSE_REQUEST) ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
            xEventGroupClearBits(state,PAUSED);
        }
        bool fresh=false; ink_touch_t next;
        if(ink_touch_read(&next,&fresh)==ESP_OK&&fresh) touch=next;
        ink_raw_input raw={.prev=!gpio_get_level(PIN_PREV),.next=!gpio_get_level(PIN_NEXT),
            .power=!gpio_get_level(PIN_POWER),.home=touch.home,.contacts=touch.contacts,.x=touch.x,.y=touch.y};
        uint32_t now=(uint32_t)(esp_timer_get_time()/1000);
        if(raw.prev||raw.next||raw.power||raw.home||raw.contacts) atomic_store(&activity,now);
        ink_event batch[5]; unsigned n=ink_events_update(&events,&raw,now,batch);
        for(unsigned i=0;i<n;i++) if(xQueueSend(queue,&batch[i],0)!=pdTRUE) atomic_fetch_add(&dropped,1);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
esp_err_t ink_capture_start(void)
{
    queue=xQueueCreate(256,sizeof(ink_event)); state=xEventGroupCreate();
    if(!queue||!state) goto failed;
    atomic_store(&activity,(unsigned)(esp_timer_get_time()/1000));
    if(xTaskCreatePinnedToCore(capture,"inkpy-input",4096,NULL,3,&task,0)==pdPASS) return ESP_OK;
failed:
    if(queue) vQueueDelete(queue);
    if(state) vEventGroupDelete(state);
    queue=NULL; state=NULL; return ESP_ERR_NO_MEM;
}
bool ink_capture_next(ink_event *event,TickType_t wait) { return xQueueReceive(queue,event,wait)==pdTRUE; }
uint32_t ink_capture_activity(void) { return atomic_load(&activity); }
unsigned ink_capture_dropped(void) { return atomic_load(&dropped); }
void ink_capture_pause(void)
{
    xEventGroupSetBits(state,PAUSE_REQUEST); xTaskNotifyGive(task);
    xEventGroupWaitBits(state,PAUSED,pdFALSE,pdTRUE,portMAX_DELAY);
}
void ink_capture_resume(void)
{
    /* Discard stale/wake gestures, not normal queued typing. */
    xQueueReset(queue); atomic_store(&activity,(unsigned)(esp_timer_get_time()/1000));
    xEventGroupClearBits(state,PAUSE_REQUEST); xTaskNotifyGive(task);
    while(xEventGroupGetBits(state)&PAUSED) vTaskDelay(1);
}
