#include "app.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
static ink_host_app app;
static union { max_align_t align; unsigned char bytes[256*1024]; } heap;
static void check(bool ok,const char *why)
{ if(!ok) { fprintf(stderr,"FAIL: %s\n",why); exit(1); } }
static void wait_change(void)
{
    struct timespec deadline; clock_gettime(CLOCK_REALTIME,&deadline); deadline.tv_sec+=10;
    check(pthread_cond_timedwait(&app.python.changed,&app.python.lock,&deadline)==0,"host watchdog");
}
static void idle(void)
{
    pthread_mutex_lock(&app.python.lock);
    while(app.python.console.busy) wait_change();
    pthread_mutex_unlock(&app.python.lock);
}
static void closed(void)
{
    pthread_mutex_lock(&app.python.lock);
    while(!app.python.closed) wait_change();
    pthread_mutex_unlock(&app.python.lock);
    check(ink_host_poll(&app) && !app.console_active && app.browser.view==INK_FILES,"return browser");
}
static unsigned locate(const char *name)
{
    for(;;) {
        for(unsigned i=0;i<app.browser.count;i++) if(!strcmp(app.browser.rows[i].name,name)) return i;
        check(ink_browser_page(&app.browser,1),"fixture exists");
    }
}
static void menu(const char *name)
{
    unsigned row=locate(name); ink_host_tap(&app,40,150+row*42,true);
    check(app.browser.view==INK_FILE_MENU,"file long press");
}
static void capture(const char *path)
{
    uint8_t frame[48000]; ink_host_draw(&app,frame);
    FILE *file=fopen(path,"wb"); check(file!=NULL,"capture"); fputs("P4\n800 480\n",file);
    for(unsigned i=0;i<sizeof(frame);i++) fputc(frame[i]^255,file);
    check(!fclose(file),"capture close");
}
int main(void)
{
    check(!ink_host_init(&app,"../../fixtures",heap.bytes,sizeof(heap.bytes)),"browser");
    ink_host_tap(&app,300,60,false); check(app.console_active,"top Console button");
    ink_host_home(&app,true); closed();
    menu("plain-text.txt"); ink_host_tap(&app,60,270,false);
    check(app.browser.view==INK_NOTICE && !strcmp(app.browser.message,"not executable"),"Execute text rejected");
    ink_host_home(&app,false);
    menu("python-demo.py"); capture("build/file-menu.pbm");
    unsigned page=app.browser.page;
    ink_host_tap(&app,60,270,false); check(app.console_active,"Execute starts console"); idle();
    pthread_mutex_lock(&app.python.lock); bool found=false;
    ink_console *c=&app.python.console;
    for(unsigned i=0;i<c->count;i++) if(!strcmp(c->lines[(c->first+i)%INK_CONSOLE_LINES],"sqrt: 9.0")) found=true;
    pthread_mutex_unlock(&app.python.lock); check(found,"file stdout in console");
    /* Actual keyboard taps: 6 * 7 Enter, through the host application router. */
    ink_host_tap(&app,106,626,false); /* symbols */
    ink_host_tap(&app,238,402,false); /* 6 */
    ink_host_tap(&app,326,458,false); /* * */
    ink_host_tap(&app,282,402,false); /* 7 */
    ink_host_tap(&app,370,626,false); /* Enter */
    idle();
    pthread_mutex_lock(&app.python.lock); found=false;
    for(unsigned i=0;i<c->count;i++) if(!strcmp(c->lines[(c->first+i)%INK_CONSOLE_LINES],"42")) found=true;
    pthread_mutex_unlock(&app.python.lock); check(found,"interactive command after script");
    capture("build/browser-console.pbm");
    ink_host_home(&app,false); ink_host_tap(&app,60,270,false); closed();
    check(app.browser.page==page,"browser page preserved");
    menu("python-loop.py"); ink_host_tap(&app,60,270,false);
    pthread_mutex_lock(&app.python.lock);
    while(app.python.queued) wait_change();
    pthread_mutex_unlock(&app.python.lock);
    ink_host_home(&app,true); closed();
    puts("OK: browser Console, long press Execute, script output, tapped REPL, Close returns, running-file long Home");
    return 0;
}
