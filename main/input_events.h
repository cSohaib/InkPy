#pragma once
#include "input.h"
typedef struct {
    bool prev,next,power,home;
    unsigned contacts,x,y;
} ink_raw_input;
enum { INK_EVENT_PREV,INK_EVENT_NEXT,INK_EVENT_HOME,INK_EVENT_TOUCH,INK_EVENT_POWER };
typedef struct { unsigned kind; ink_press_t press; unsigned x,y; } ink_event;
typedef struct {
    ink_button_t prev,next,home,power,contact;
    unsigned contacts,x,y;
    bool moved;
} ink_event_state;
/* At most five discrete events. Coordinates stay native until UI dispatch. */
unsigned ink_events_update(ink_event_state *s,const ink_raw_input *raw,uint32_t now,ink_event events[5]);
