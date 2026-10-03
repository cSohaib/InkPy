#include "input_events.h"
#include <assert.h>
#include <stdio.h>
static ink_event_state state;
static ink_event pending[32];
static unsigned count;
static void sample(ink_raw_input raw,unsigned now)
{
    ink_event out[5]; unsigned n=ink_events_update(&state,&raw,now,out);
    for(unsigned i=0;i<n;i++) { assert(count<32); pending[count++]=out[i]; }
}
static void tap(unsigned at,unsigned x)
{
    ink_raw_input raw={.contacts=1,.x=x,.y=100};
    sample(raw,at); sample(raw,at+30);
    raw.contacts=0; sample(raw,at+60); sample(raw,at+90);
}
int main(void)
{
    /* Producer observes distinct taps while the display consumer waits 1 sec. */
    tap(0,40); tap(120,40); tap(240,80); tap(360,120);
    assert(count==4);
    for(unsigned i=0;i<count;i++) {
        assert(pending[i].kind==INK_EVENT_TOUCH&&pending[i].press==INK_PRESS_SHORT);
        assert(pending[i].x==(i<2?40:i==2?80:120));
    }
    state=(ink_event_state){0}; count=0;
    ink_raw_input raw={.contacts=1,.x=40,.y=100};
    sample(raw,0); sample(raw,30); raw.x=80; sample(raw,50);
    raw.contacts=0; sample(raw,60); sample(raw,90); assert(!count);
    state=(ink_event_state){0};
    raw=(ink_raw_input){.home=true}; sample(raw,0); sample(raw,30); sample(raw,830);
    raw.home=false; sample(raw,840); sample(raw,870);
    assert(count==1&&pending[0].kind==INK_EVENT_HOME&&pending[0].press==INK_PRESS_LONG);
    state=(ink_event_state){0}; count=0;
    raw=(ink_raw_input){.power=true}; sample(raw,0); sample(raw,30);
    raw.power=false; sample(raw,60); sample(raw,90);
    raw.power=true; sample(raw,150); sample(raw,180);
    raw.power=false; sample(raw,210); sample(raw,240); sample(raw,600);
    assert(count==1&&pending[0].kind==INK_EVENT_POWER&&pending[0].press==INK_PRESS_DOUBLE);
    puts("PASS: repeated queued taps/order, swipe suppression, Home hold, Power double click");
}
