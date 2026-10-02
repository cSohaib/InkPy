#include "app.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
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
/* Type through the same tap router used by browser, editor and console. */
static void type(const char *text)
{
    for(;*text;text++) {
        bool found=false;
        for(unsigned layer=0;layer<2 && !found;layer++) {
            ink_keyboard *keyboard=app.editor_active?&app.editor.keyboard:&app.browser.keyboard;
            for(unsigned row=0;row<5 && !found;row++) for(unsigned col=0;col<(row==4?5u:10u);col++)
                if(ink_keyboard_key(keyboard,row,col)==*text) {
                    unsigned width=row==4?88:44;
                    ink_host_tap(&app,INK_KB_X+col*width+width/2,INK_KB_Y+row*56+28,false); found=true; break;
                }
            if(!found) ink_host_tap(&app,INK_KB_X+132,INK_KB_Y+252,false);
        }
        check(found,"typing character");
    }
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
    char folder[]="build/editor-XXXXXX"; check(mkdtemp(folder)!=NULL,"editor folder");
    char path[512]; snprintf(path,sizeof(path),"%s/script.py",folder);
    FILE *file=fopen(path,"wb"); check(file!=NULL,"editor fixture");
    fputs("print(1)\n",file); fclose(file);
    check(!ink_host_init(&app,folder,heap.bytes,sizeof(heap.bytes)),"editor browser");
    menu("script.py"); ink_host_tap(&app,60,200,false); check(app.editor_active,"Edit opens editor");
    ink_host_tap(&app,16+7*18,66,false); /* after 1 */
    ink_host_tap(&app,282,626,false); /* Delete */
    ink_host_tap(&app,106,626,false); /* symbols */
    ink_host_tap(&app,62,402,false); /* 2 */
    ink_host_home(&app,false); ink_host_tap(&app,60,330,false); check(app.editor_active && !app.editor.menu,"Cancel keeps editor");
    capture("build/editor.pbm");
    ink_host_home(&app,false); capture("build/editor-menu.pbm");
    ink_host_tap(&app,60,200,false); check(!app.editor_active,"Save closes");
    file=fopen(path,"rb"); check(file!=NULL,"saved fixture"); char text[32]={0}; check(fread(text,1,sizeof(text)-1,file)==9,"saved length"); fclose(file);
    check(!strcmp(text,"print(2)\n"),"exact edit saved");
    menu("script.py"); ink_host_tap(&app,60,200,false); ink_host_tap(&app,18,402,false); /* q */
    ink_host_home(&app,false); ink_host_home(&app,false); check(!app.editor.menu,"second Home cancels editor menu");
    ink_host_home(&app,false); ink_host_tap(&app,60,270,false); check(!app.editor_active,"Discard closes");
    menu("script.py"); ink_host_tap(&app,60,200,false); ink_host_tap(&app,18,402,false); ink_host_home(&app,true);
    check(!app.editor_active,"long Home discards");
    file=fopen(path,"rb"); check(file!=NULL,"discard fixture"); memset(text,0,sizeof(text)); check(fread(text,1,sizeof(text)-1,file)==9,"discard length"); fclose(file);
    check(!strcmp(text,"print(2)\n"),"discard preserves original");
    check(!unlink(path),"old fixture removed");
    check(!ink_browser_reload(&app.browser),"refresh after fixture removal");
    ink_host_tap(&app,50,70,false); type("new.py"); ink_host_tap(&app,350,745,false);
    check(app.editor_active && app.editor.size==0,"New file opens empty editor");
    type("print(6*7)"); ink_host_home(&app,false); ink_host_tap(&app,60,200,false);
    check(!app.editor_active,"new script saved");
    menu("new.py"); ink_host_tap(&app,60,270,false); idle();
    pthread_mutex_lock(&app.python.lock); found=false; c=&app.python.console;
    for(unsigned i=0;i<c->count;i++) if(!strcmp(c->lines[(c->first+i)%INK_CONSOLE_LINES],"42")) found=true;
    pthread_mutex_unlock(&app.python.lock); check(found,"created script executes");
    capture("build/created-script.pbm"); ink_host_home(&app,true); closed();
    snprintf(path,sizeof(path),"%s/new.py",folder); check(!unlink(path),"created script cleanup");
    check(!ink_browser_reload(&app.browser),"refresh");
    ink_host_tap(&app,50,70,false); type("scratch"); ink_host_tap(&app,350,745,false);
    check(app.editor_active,"extensionless creation opens editor");
    type("discard me"); ink_host_home(&app,true);
    snprintf(path,sizeof(path),"%s/scratch",folder);
    file=fopen(path,"rb"); check(file!=NULL && fgetc(file)==EOF,"discard keeps newly created file empty"); fclose(file);
    check(!unlink(path) && !rmdir(folder),"working copies removed");
    puts("OK: browser Edit, tapped cursor/Delete/type, Save, Cancel, Discard, long Home, temp cleanup");
    puts("OK: create .py, type, Save, Execute -> 42; extensionless creation and Discard");
    return 0;
}
