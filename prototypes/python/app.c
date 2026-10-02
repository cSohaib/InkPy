#include "app.h"
#include <stdio.h>
int ink_host_init(ink_host_app *a,const char *root,void *heap,size_t size)
{
    a->heap=heap; a->heap_size=size; a->console_active=false;
    return ink_browser_init(&a->browser,root);
}
static void launch(ink_host_app *a)
{
    bool file=a->browser.view==INK_EXECUTE_PYTHON;
    if(!file && a->browser.view!=INK_OPEN_CONSOLE) return;
    if(ink_session_start(&a->python,a->heap,a->heap_size)) {
        snprintf(a->browser.message,sizeof(a->browser.message),"Cannot start Python");
        a->browser.view=INK_NOTICE; return;
    }
    a->console_active=true;
    if(file) {
        pthread_mutex_lock(&a->python.lock);
        ink_session_file(&a->python,a->browser.selected);
        pthread_mutex_unlock(&a->python.lock);
    }
}
static void action(ink_host_app *a,int event)
{
    if(event==INK_CONSOLE_STOP) ink_session_stop(&a->python);
    if(event==INK_CONSOLE_CLOSE) ink_session_close(&a->python);
}
void ink_host_tap(ink_host_app *a,unsigned x,unsigned y,bool long_press)
{
    if(!a->console_active) {
        if(long_press) ink_browser_long_press(&a->browser,x,y);
        else ink_browser_tap(&a->browser,x,y);
        launch(a); return;
    }
    if(long_press) return;
    pthread_mutex_lock(&a->python.lock);
    if(!a->python.closing) {
        ink_console *c=&a->python.console;
        if(c->menu) action(a,ink_console_tap(c,x,y));
        else if(!c->busy && ink_console_key(c,ink_keyboard_tap(&c->keyboard,x,y)))
            ink_session_submit(&a->python);
    }
    pthread_mutex_unlock(&a->python.lock);
}
void ink_host_home(ink_host_app *a,bool long_press)
{
    if(!a->console_active) { ink_browser_home(&a->browser); return; }
    pthread_mutex_lock(&a->python.lock);
    if(!a->python.closing) action(a,ink_console_home(&a->python.console,long_press));
    pthread_mutex_unlock(&a->python.lock);
}
void ink_host_page(ink_host_app *a,int direction)
{
    if(!a->console_active) { ink_browser_page(&a->browser,direction); return; }
    pthread_mutex_lock(&a->python.lock);
    ink_console_page(&a->python.console,direction);
    pthread_mutex_unlock(&a->python.lock);
}
bool ink_host_poll(ink_host_app *a)
{
    if(!a->console_active) return false;
    pthread_mutex_lock(&a->python.lock); bool closed=a->python.closed;
    pthread_mutex_unlock(&a->python.lock);
    if(!closed) return false;
    ink_session_join(&a->python); a->console_active=false;
    ink_browser_home(&a->browser); return true;
}
void ink_host_draw(ink_host_app *a,uint8_t frame[48000])
{
    if(!a->console_active) { ink_browser_draw(&a->browser,frame); return; }
    ink_console snapshot;
    pthread_mutex_lock(&a->python.lock); snapshot=a->python.console;
    pthread_mutex_unlock(&a->python.lock);
    ink_console_draw(&snapshot,frame);
}
