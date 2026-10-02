#include "ink_console.h"
#include <string.h>
void ink_console_init(ink_console *c)
{ memset(c,0,sizeof(*c)); c->count=1; }
bool ink_console_key(ink_console *c,int key)
{
    if(c->busy || c->menu) return false;
    c->full=false;
    if(key==INK_KEY_ENTER) {
        c->busy=true; c->page=0; return true;
    }
    if(key==INK_KEY_DELETE) {
        if(c->used) c->input[--c->used]=0;
    } else if(key==INK_KEY_INDENT || (key>=32 && key<=126)) {
        size_t n=key==INK_KEY_INDENT?4:1;
        /* Reserve one byte for a continuation newline and one for NUL. */
        if(c->used+n>sizeof(c->input)-2) { c->full=true; return false; }
        for(size_t i=0;i<n;i++) c->input[c->used++]=key==INK_KEY_INDENT?' ':(char)key;
        c->input[c->used]=0;
    }
    return false;
}
static void newline(ink_console *c)
{
    if(c->count<INK_CONSOLE_LINES) c->count++;
    else c->first=(c->first+1)%INK_CONSOLE_LINES;
    unsigned last=(c->first+c->count-1)%INK_CONSOLE_LINES;
    memset(c->lines[last],0,sizeof(c->lines[last])); c->column=0;
}
void ink_console_output(ink_console *c,const char *bytes,size_t length)
{
    for(size_t i=0;i<length;i++) {
        unsigned char ch=(unsigned char)bytes[i];
        if(ch=='\r') continue;
        if(ch=='\n') { newline(c); continue; }
        if(c->column==INK_CONSOLE_COLUMNS) newline(c);
        unsigned last=(c->first+c->count-1)%INK_CONSOLE_LINES;
        c->lines[last][c->column++]=(ch>=32 && ch<=126)?(char)ch:'?';
    }
    unsigned pages=(c->count-1)/INK_CONSOLE_PAGE_ROWS;
    if(c->page>pages) c->page=pages;
}
void ink_console_completed(ink_console *c,bool more)
{
    if(more && c->used>=sizeof(c->input)-1) {
        ink_console_output(c,"Input full; block cleared\n",26);
        more=false; c->full=true;
    }
    c->busy=false; c->more=more;
    if(more) c->input[c->used++]='\n';
    else c->used=0;
    c->input[c->used]=0;
}
void ink_console_page(ink_console *c,int direction)
{
    if(c->menu) return;
    unsigned max=(c->count-1)/INK_CONSOLE_PAGE_ROWS;
    if(direction<0 && c->page<max) c->page++;
    if(direction>0 && c->page) c->page--;
}
int ink_console_home(ink_console *c,bool long_press)
{
    if(long_press) { c->menu=false; return INK_CONSOLE_CLOSE; }
    c->menu=!c->menu; return INK_CONSOLE_NONE;
}
int ink_console_tap(ink_console *c,unsigned x,unsigned y)
{
    if(!c->menu) return INK_CONSOLE_NONE;
    if(x<32 || x>=448 || y<180 || y>=372) return INK_CONSOLE_NONE;
    unsigned row=(y-180)/64; c->menu=false;
    return row==0?INK_CONSOLE_STOP:row==1?INK_CONSOLE_CLOSE:INK_CONSOLE_NONE;
}
