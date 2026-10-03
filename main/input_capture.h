#pragma once
#include "input_events.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
esp_err_t ink_capture_start(void);
bool ink_capture_next(ink_event *event,TickType_t wait);
uint32_t ink_capture_activity(void);
unsigned ink_capture_dropped(void);
/* Cooperative pause acknowledgment before touch power/sleep transitions. */
void ink_capture_pause(void);
void ink_capture_resume(void);
