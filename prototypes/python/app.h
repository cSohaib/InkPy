#pragma once
#include "session.h"
#include "../../components/ink_browser/ink_browser.h"
/* Host composition of portable browser/console and POSIX worker. */
typedef struct {
    ink_browser browser;
    ink_session python;
    void *heap;
    size_t heap_size;
    bool console_active;
} ink_host_app;
int ink_host_init(ink_host_app *a,const char *root,void *heap,size_t size);
void ink_host_tap(ink_host_app *a,unsigned x,unsigned y,bool long_press);
void ink_host_home(ink_host_app *a,bool long_press);
void ink_host_page(ink_host_app *a,int direction);
/* Poll Close acknowledgment, then return to same browser folder/page. */
bool ink_host_poll(ink_host_app *a);
void ink_host_draw(ink_host_app *a,uint8_t frame[48000]);
