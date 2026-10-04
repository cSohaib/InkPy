#include "ink_browser.h"
#include "ink_ui_font.h"
#include "ink_editor.h"
#include "ink_power.h"
#include "ink_icons.h"
#include "../ink_python/ink_console.h"
#include <stdio.h>
#include <string.h>
#ifdef INK_USE_FONTS
#include "ink_font.h"
#endif

static void pixel(uint8_t *frame,unsigned x,unsigned y)
{
    if(x>=480 || y>=800) return;
    unsigned dx=799-y,dy=x;
    frame[dy*100+dx/8]&=(uint8_t)~(0x80>>(dx%8));
}
static void text_at(uint8_t *frame,unsigned x,unsigned y,const char *s,unsigned cell,unsigned height,unsigned limit)
{
#ifdef INK_USE_FONTS
    ink_font_text(frame,x,y,s,cell,height,0,limit,pixel);
#else
    for(;*s&&x+cell<=limit;s++) {
        unsigned char c=(unsigned char)*s;
        if((c&0xc0)==0x80) continue;
        if(c<32||c>126)c='?';
        for(unsigned row=0;row<height;row++)for(unsigned col=0;col<cell-1;col++)
            if(ink_ui_font[c-32][row*16/height]&(1u<<(col*9/(cell-1))))pixel(frame,x+col,y+row);
        x+=cell;
    }
#endif
}
static void text(uint8_t *f,unsigned x,unsigned y,const char *s)
{ text_at(f,x,y,s,INK_UI_CELL,INK_UI_FONT,474); }
static void text_size(uint8_t *f,unsigned x,unsigned y,const char *s,unsigned scale)
{ text_at(f,x,y,s,scale==1?12:INK_UI_CELL,scale==1?20:INK_UI_FONT,474); }
static void box(uint8_t *frame,unsigned x,unsigned y,unsigned w,unsigned h)
{
    for(unsigned i=0;i<w;++i) { pixel(frame,x+i,y); pixel(frame,x+i,y+h-1); }
    for(unsigned i=0;i<h;++i) { pixel(frame,x,y+i); pixel(frame,x+w-1,y+i); }
}
static void new_file(const ink_browser *b,uint8_t *frame)
{
    ink_icon_size(frame,208,12,ICON_ADD,64,pixel);
    box(frame,16,96,448,64);
    size_t length=strlen(b->new_name);
    const char *visible=b->new_name+(length>26?length-26:0);
    text(frame,24,108,visible);
    unsigned cursor=24+(unsigned)strlen(visible)*INK_UI_CELL;
    for(unsigned y=110;y<140;++y) pixel(frame,cursor,y);
    if(b->message[0]) ink_icon(frame,224,180,ICON_ERROR,pixel);
    /* Shared keyboard draw follows below. */
}
static void keyboard_draw_at(const ink_keyboard *keyboard,uint8_t *frame,unsigned offset)
{
    for(unsigned row=0;row<5;++row) for(unsigned col=0;col<(row==4?5u:10u);++col) {
        int key=ink_keyboard_key(keyboard,row,col); if(!key) continue;
        unsigned w=INK_KB_WIDTH*(row==4?2:1),x=INK_KB_X+col*w,y=INK_KB_Y+offset+row*INK_KB_HEIGHT;
        box(frame,x+1,y+1,w-2,INK_KB_HEIGHT-2);
        if(row==4 || key==INK_KEY_INDENT) {
            if(key==INK_KEY_SYMBOLS) text_size(frame,x+(w-27)/2,y+19,"abc",1);
            else ink_icon(frame,x+(w-32)/2,y+12,key==INK_KEY_INDENT?ICON_TAB:key==INK_KEY_SHIFT?ICON_SHIFT:
                key==' '?ICON_SPACE:key==INK_KEY_DELETE?ICON_DELETE:ICON_ENTER,pixel);
        } else {
            char label[2]={(char)key,0};
#ifdef INK_USE_FONTS
            ink_font_text(frame,x+(w-INK_UI_CELL)/2,y+14,label,INK_UI_CELL,INK_UI_FONT,0,480,pixel);
#else
            text(frame,x+(w-18)/2,y+10,label);
#endif
        }
    }
}
static void keyboard_draw(const ink_keyboard *keyboard,uint8_t *frame)
{ keyboard_draw_at(keyboard,frame,0); }
void ink_power_draw(const ink_power *p,uint8_t frame[48000])
{
    memset(frame,255,48000);
    text_size(frame,8,20,p->time[0]?p->time:"--",1);
    box(frame,386,14,80,32);box(frame,466,24,6,12);
    text_at(frame,396,20,p->battery[0]?p->battery:"--",11,20,462);
    if(p->view) {
        for(unsigned i=0;i<8&&p->font_first+i<p->font_count;i++) {
            box(frame,8,96+i*64,464,64);text(frame,16,114+i*64,p->font_names[i]);
        }
        box(frame,8,624,464,64);ink_icon(frame,224,640,ICON_BACK,pixel);
    } else {
        for(unsigned row=0;row<2;row++) {
            unsigned y=96+row*96;char value[8];snprintf(value,sizeof(value),"%u",row?p->warmth:p->brightness);
            box(frame,8,y,96,96);box(frame,104,y,272,96);box(frame,376,y,96,96);
            text(frame,47,y+34,"-");text(frame,414,y+34,"+");
            ink_icon_size(frame,154,y+16,row?ICON_WARM:p->on?ICON_BULB_ON:ICON_BULB,64,pixel);
            text(frame,242,y+34,value);
        }
        const unsigned icons[]={ICON_FONT,ICON_REFRESH,ICON_CONTRAST};
        for(unsigned i=0;i<3;i++) { unsigned x=8+i*155;
            box(frame,x,304,154,152);ink_icon_size(frame,x+29,332,icons[i],96,pixel);
        }
    }
    if(p->message[0]) ink_icon(frame,224,720,ICON_ERROR,pixel);
}
void ink_console_draw(const ink_console *c,uint8_t frame[48000])
{
    memset(frame,0xff,48000);
    if(c->menu) {
        const unsigned icons[]={ICON_STOP,ICON_EXIT,ICON_BACK};
        for(unsigned i=0;i<3;i++) {
            box(frame,32,180+i*64,416,64);
            ink_icon(frame,224,196+i*64,icons[i],pixel);
        }
        return;
    }
    for(unsigned row=0;row<INK_CONSOLE_PAGE_ROWS;row++) text(frame,INK_UI_MARGIN,INK_UI_MARGIN+row*INK_UI_LINE,ink_console_line(c,row));
    box(frame,0,460,480,60);
    const char *visible=c->input+(c->used>INK_UI_COLUMNS?c->used-INK_UI_COLUMNS:0);
    char line[INK_UI_COLUMNS+1]; size_t i=0;
    for(;visible[i] && i<INK_UI_COLUMNS;i++) line[i]=visible[i]=='\n'?' ':visible[i];
    line[i]=0; text(frame,INK_UI_MARGIN,477,line);
    keyboard_draw_at(&c->keyboard,frame,INK_CONSOLE_KEYBOARD_OFFSET);
    if(c->full) ink_icon(frame,224,426,ICON_ERROR,pixel);
}
void ink_editor_draw(const ink_editor *e,uint8_t frame[48000])
{
    memset(frame,0xff,48000);
    if(e->menu) {
        const char *name=strrchr(e->path,'/'); text(frame,16,8,name?name+1:e->path);
        const unsigned icons[]={ICON_SAVE,ICON_DISCARD,ICON_BACK};
        for(unsigned i=0;i<3;i++) { box(frame,32,180+i*64,416,64); ink_icon(frame,224,196+i*64,icons[i],pixel); }
    } else {
        for(unsigned row=0;row<e->rows;row++) text(frame,INK_UI_MARGIN,INK_UI_MARGIN+row*INK_UI_LINE,e->lines[row]);
        unsigned x=INK_UI_MARGIN+e->cursor_column*INK_UI_CELL,y=INK_UI_MARGIN+e->cursor_row*INK_UI_LINE;
        for(unsigned i=0;i<INK_UI_FONT;i++) pixel(frame,x,y+i);
        keyboard_draw_at(&e->keyboard,frame,INK_EDITOR_KEYBOARD_OFFSET);
    }
    if(e->error[0]) ink_icon(frame,224,110,ICON_ERROR,pixel);
}
void ink_browser_draw(const ink_browser *b,uint8_t frame[48000])
{
    memset(frame,0xff,48000);
    if(b->view==INK_NEW_FILE) {
        new_file(b,frame); keyboard_draw(&b->keyboard,frame);
        box(frame,16,720,224,56); box(frame,240,720,224,56);
        ink_icon(frame,112,732,ICON_BACK,pixel); ink_icon(frame,336,732,ICON_SAVE,pixel); return;
    }
    if(b->view==INK_OPEN_TEXT) {
        for(unsigned row=0;row<b->text.rows;++row) text(frame,INK_UI_MARGIN,INK_UI_MARGIN+row*INK_UI_LINE,b->text.lines[row]);
        return;
    }
    if(b->view==INK_FILE_MENU) {
        const char *name=strrchr(b->selected,'/');
        text(frame,16,80,name?name+1:b->selected);
        const unsigned icons[]={ICON_EDIT,ICON_PLAY,ICON_TRASH};
        for(unsigned i=0;i<3;i++){box(frame,8,140+i*80,464,80);ink_icon_size(frame,216,156+i*80,icons[i],48,pixel);}
        return;
    }
    if(b->view!=INK_FILES) { ink_icon(frame,224,368,ICON_ERROR,pixel); return; }
    const char *folder=b->folder+strlen(b->root);
    if(*folder) text_size(frame,6,4,folder,1);
    box(frame,0,INK_BROWSER_ACTION_Y,240,800-INK_BROWSER_ACTION_Y); box(frame,240,INK_BROWSER_ACTION_Y,240,800-INK_BROWSER_ACTION_Y);
    ink_icon_size(frame,96,732,ICON_ADD,48,pixel); ink_icon_size(frame,336,732,ICON_CONSOLE,48,pixel);
    for(unsigned i=0;i<b->count;++i) {
        unsigned y=INK_BROWSER_LIST_Y+i*INK_BROWSER_ROW_HEIGHT;
        ink_icon(frame,6,y+4,b->rows[i].directory?ICON_FOLDER:ICON_FILE,pixel);
        text(frame,44,y+4,b->rows[i].name);
    }
}
