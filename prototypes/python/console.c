#include "session.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
static ink_session session;
static union { max_align_t align; unsigned char bytes[256*1024]; } heap;
static void check(bool ok,const char *why)
{ if(!ok) { fprintf(stderr,"FAIL: %s\n",why); exit(1); } }
/* Host watchdog only, never a Python runtime limit. Caller holds lock. */
static void wait_change(void)
{
    struct timespec deadline; clock_gettime(CLOCK_REALTIME,&deadline); deadline.tv_sec+=10;
    check(pthread_cond_timedwait(&session.changed,&session.lock,&deadline)==0,"host check watchdog");
}
static void wait_idle(void)
{
    pthread_mutex_lock(&session.lock);
    while(session.console.busy) wait_change();
    pthread_mutex_unlock(&session.lock);
}
static void tap_character(char ch)
{
    ink_console *c=&session.console;
    for(unsigned row=0;row<5;row++) for(unsigned col=0;col<(row==4?5u:10u);col++)
        if(ink_keyboard_key(&c->keyboard,row,col)==ch) {
            unsigned w=INK_KB_WIDTH*(row==4?2:1);
            ink_console_key(c,ink_keyboard_tap(&c->keyboard,INK_KB_X+col*w+2,INK_KB_Y+row*INK_KB_HEIGHT+2)); return;
        }
    ink_keyboard_tap(&c->keyboard,INK_KB_X+90,INK_KB_Y+4*INK_KB_HEIGHT+2);
    for(unsigned row=0;row<4;row++) for(unsigned col=0;col<10;col++)
        if(ink_keyboard_key(&c->keyboard,row,col)==ch) {
            ink_console_key(c,ink_keyboard_tap(&c->keyboard,INK_KB_X+col*INK_KB_WIDTH+2,INK_KB_Y+row*INK_KB_HEIGHT+2)); return;
        }
    check(false,"character unavailable");
}
static void submit(const char *line,bool wait)
{
    pthread_mutex_lock(&session.lock);
    ink_console *c=&session.console;
    for(;*line;line++) tap_character(*line);
    int key=ink_keyboard_tap(&c->keyboard,INK_KB_X+4*88+2,INK_KB_Y+4*INK_KB_HEIGHT+2);
    check(ink_console_key(c,key),"Enter submits"); ink_session_submit(&session);
    pthread_mutex_unlock(&session.lock);
    if(wait) wait_idle();
}
static bool has_line(const char *text)
{
    bool found=false; pthread_mutex_lock(&session.lock);
    ink_console *c=&session.console;
    for(unsigned i=0;i<c->count;i++) if(!strcmp(c->lines[(c->first+i)%INK_CONSOLE_LINES],text)) found=true;
    pthread_mutex_unlock(&session.lock); return found;
}
static void capture(const char *path)
{
    ink_console snapshot;
    pthread_mutex_lock(&session.lock); snapshot=session.console; pthread_mutex_unlock(&session.lock);
    uint8_t frame[48000]; ink_console_draw(&snapshot,frame);
    FILE *file=fopen(path,"wb"); check(file!=NULL,"preview"); fputs("P4\n800 480\n",file);
    for(size_t i=0;i<sizeof(frame);i++) frame[i]=~frame[i];
    check(fwrite(frame,1,sizeof(frame),file)==sizeof(frame),"write preview"); fclose(file);
}
static void endless(void)
{
    submit("while 1:",true); submit("    pass",true); submit("",false);
    pthread_mutex_lock(&session.lock);
    while(session.queued) wait_change();
    ink_session_pause(&session,true);
    while(!session.paused) wait_change();
    check(session.console.busy,"pause acknowledges running command");
    pthread_mutex_unlock(&session.lock);
}
int main(void)
{
    check(ink_session_start(&session,heap.bytes,sizeof(heap.bytes))==0,"worker");
    submit("x=2+3",true); submit("x",true); check(has_line("5"),"persistent globals");
    submit("for i in range(3):",true); submit("    print(i)",true); submit("",true);
    check(has_line("2"),"multiline output");
    endless();
    pthread_mutex_lock(&session.lock);
    ink_console_home(&session.console,false);
    check(ink_console_tap(&session.console,60,330)==INK_CONSOLE_NONE && !session.console.menu,"Cancel");
    ink_console_home(&session.console,false); pthread_mutex_unlock(&session.lock);
    capture("build/console-menu.pbm");
    pthread_mutex_lock(&session.lock);
    check(ink_console_tap(&session.console,60,200)==INK_CONSOLE_STOP,"Stop selection");
    ink_session_stop(&session); pthread_mutex_unlock(&session.lock);
    wait_idle(); check(has_line("Stopped; Python reset"),"stop acknowledgment");
    submit("6*7",true); check(has_line("42"),"new command after reset");
    capture("build/console.pbm");
    puts("OK: Home menu, Cancel, pause acknowledgment, Stop while paused, VM reset, result 42");
    endless();
    pthread_mutex_lock(&session.lock); ink_console_home(&session.console,false);
    check(ink_console_tap(&session.console,60,270)==INK_CONSOLE_CLOSE,"Close selection");
    ink_session_close(&session); while(!session.closed) wait_change();
    pthread_mutex_unlock(&session.lock); ink_session_join(&session);
    puts("OK: Close while running and paused acknowledges VM cleanup");
    check(ink_session_start(&session,heap.bytes,sizeof(heap.bytes))==0,"reopen console");
    pthread_mutex_lock(&session.lock);
    ink_console_home(&session.console,false); ink_console_home(&session.console,false);
    check(!session.console.menu,"second Home cancels");
    check(ink_console_home(&session.console,true)==INK_CONSOLE_CLOSE,"long Home closes");
    ink_session_close(&session); while(!session.closed) wait_change();
    pthread_mutex_unlock(&session.lock); ink_session_join(&session);
    puts("OK: second Home cancels, long Home closes, fresh console opens");
    return 0;
}
