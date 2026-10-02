#include "ink_python.h"
#include "ink_console.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t changed=PTHREAD_COND_INITIALIZER;
static ink_console console;
static bool queued,closing;
static char command[INK_CONSOLE_INPUT];
static union { max_align_t align; unsigned char bytes[256*1024]; } heap;
static void check(bool ok,const char *why)
{ if(!ok) { fprintf(stderr,"FAIL: %s\n",why); exit(1); } }
static bool control(void *context)
{
    (void)context; pthread_mutex_lock(&lock);
    bool stop=closing; pthread_mutex_unlock(&lock); return stop;
}
static void output(void *context,const char *bytes,size_t length)
{
    (void)context; pthread_mutex_lock(&lock);
    ink_console_output(&console,bytes,length); pthread_mutex_unlock(&lock);
}
static void *worker(void *context)
{
    (void)context; int stack_top;
    ink_python_callbacks(control,output,NULL);
    ink_python_init(heap.bytes,sizeof(heap.bytes),&stack_top);
    for(;;) {
        pthread_mutex_lock(&lock);
        while(!queued && !closing) pthread_cond_wait(&changed,&lock);
        if(closing) { pthread_mutex_unlock(&lock); break; }
        queued=false;
        pthread_mutex_unlock(&lock);
        /* Queue slot remains immutable while console.busy is true. */
        bool more=ink_python_more(command);
        int result=more?0:ink_python_text(command,true);
        pthread_mutex_lock(&lock);
        ink_console_completed(&console,more);
        pthread_cond_broadcast(&changed);
        pthread_mutex_unlock(&lock);
        if(result==2) break;
    }
    ink_python_close(); return NULL;
}
static void tap_character(char ch)
{
    for(unsigned row=0;row<5;row++) for(unsigned col=0;col<(row==4?5u:10u);col++)
        if(ink_keyboard_key(&console.keyboard,row,col)==ch) {
            unsigned w=INK_KB_WIDTH*(row==4?2:1);
            int key=ink_keyboard_tap(&console.keyboard,INK_KB_X+col*w+2,INK_KB_Y+row*INK_KB_HEIGHT+2);
            ink_console_key(&console,key); return;
        }
    ink_keyboard_tap(&console.keyboard,INK_KB_X+90,INK_KB_Y+4*INK_KB_HEIGHT+2);
    for(unsigned row=0;row<4;row++) for(unsigned col=0;col<10;col++)
        if(ink_keyboard_key(&console.keyboard,row,col)==ch) {
            ink_console_key(&console,ink_keyboard_tap(&console.keyboard,INK_KB_X+col*INK_KB_WIDTH+2,INK_KB_Y+row*INK_KB_HEIGHT+2)); return;
        }
    check(false,"character unavailable");
}
static void submit(const char *line)
{
    pthread_mutex_lock(&lock);
    for(;*line;line++) tap_character(*line);
    int key=ink_keyboard_tap(&console.keyboard,INK_KB_X+4*88+2,INK_KB_Y+4*INK_KB_HEIGHT+2);
    check(ink_console_key(&console,key),"Enter submits");
    ink_console_output(&console,console.more?"... ":">>> ",4);
    /* Echo just the newly typed line, not the complete continuation source. */
    const char *last=strrchr(console.input,'\n'); last=last?last+1:console.input;
    ink_console_output(&console,last,strlen(last)); ink_console_output(&console,"\n",1);
    memcpy(command,console.input,console.used+1); queued=true;
    pthread_cond_broadcast(&changed);
    struct timespec deadline; clock_gettime(CLOCK_REALTIME,&deadline); deadline.tv_sec+=10;
    while(console.busy) check(pthread_cond_timedwait(&changed,&lock,&deadline)==0,"host check watchdog");
    pthread_mutex_unlock(&lock);
}
int main(void)
{
    ink_console_init(&console); pthread_t thread;
    check(pthread_create(&thread,NULL,worker,NULL)==0,"worker");
    submit("x=2+3"); submit("x");
    bool found=false;
    for(unsigned i=0;i<console.count;i++) if(!strcmp(console.lines[(console.first+i)%INK_CONSOLE_LINES],"5")) found=true;
    check(found,"persistent globals and expression result");
    submit("for i in range(3):"); submit("    print(i)"); submit("");
    check(!console.more,"multiline completed");
    found=false;
    for(unsigned i=0;i<console.count;i++) if(!strcmp(console.lines[(console.first+i)%INK_CONSOLE_LINES],"2")) found=true;
    check(found,"multiline output");
    for(unsigned i=0;i<console.count;i++) puts(console.lines[(console.first+i)%INK_CONSOLE_LINES]);
    uint8_t frame[48000]; ink_console_draw(&console,frame);
    FILE *file=fopen("build/console.pbm","wb"); check(file!=NULL,"preview");
    fputs("P4\n800 480\n",file);
    for(size_t i=0;i<sizeof(frame);i++) frame[i]=~frame[i];
    check(fwrite(frame,1,sizeof(frame),file)==sizeof(frame),"write preview"); fclose(file);
    /* Check bounded history wraps and physical page directions. */
    for(unsigned i=0;i<200;i++) ink_console_output(&console,"older\n",6);
    check(console.count==INK_CONSOLE_LINES,"history cap");
    ink_console_page(&console,-1); check(console.page==1,"previous page");
    ink_console_page(&console,1); check(console.page==0,"next page");
    pthread_mutex_lock(&lock); closing=true; pthread_cond_broadcast(&changed); pthread_mutex_unlock(&lock);
    pthread_join(thread,NULL); puts("OK: keyboard, worker REPL, multiline, bounded history, pages, close");
    return 0;
}
