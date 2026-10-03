#include "input_events.h"
#include <stdlib.h>
unsigned ink_events_update(ink_event_state *s,const ink_raw_input *r,uint32_t now,ink_event out[5])
{
    if(!s->contacts&&r->contacts) { s->x=r->x; s->y=r->y; s->moved=false; }
    if(r->contacts>1 || (r->contacts&&(abs((int)r->x-(int)s->x)>12||abs((int)r->y-(int)s->y)>12))) s->moved=true;
    s->contacts=r->contacts;
    ink_press_t presses[]={ink_button_update(&s->prev,r->prev,now,false),
        ink_button_update(&s->next,r->next,now,false),ink_button_update(&s->home,r->home,now,false),
        ink_button_update(&s->contact,r->contacts!=0,now,false),ink_button_update(&s->power,r->power,now,true)};
    unsigned n=0;
    for(unsigned i=0;i<5;i++) if(presses[i]!=INK_PRESS_NONE && (i!=INK_EVENT_TOUCH||!s->moved))
        out[n++]=(ink_event){i,presses[i],s->x,s->y};
    return n;
}
