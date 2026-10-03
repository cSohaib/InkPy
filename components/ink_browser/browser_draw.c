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
#include "ink_view.h"
#endif

static void pixel(uint8_t *frame,unsigned x,unsigned y)
{
#ifdef INK_USE_FONTS
    ink_view_pixel(frame,x,y,480,800); return;
#endif
    if(x>=480 || y>=800) return;
    unsigned dx=799-y,dy=x;
    frame[dy*100+dx/8]&=(uint8_t)~(0x80>>(dx%8));
}
static void text_size(uint8_t *frame,unsigned x,unsigned y,const char *s,unsigned scale)
{
#ifdef INK_USE_FONTS
    ink_font_text(frame,x,y,s,9*scale,16*scale,0,464,pixel); return;
#endif
    unsigned columns=0;
    for(;*s && columns<25;++s) {
        unsigned char c=(unsigned char)*s;
        if((c&0xc0)==0x80) continue; /* One visible fallback per non-ASCII codepoint. */
        if(c<32 || c>126) c='?';
        if(x+9*scale>464) break;
        for(unsigned row=0;row<16;++row) for(unsigned col=0;col<9;++col)
            if(ink_ui_font[c-32][row]&(1u<<col))
                for(unsigned sy=0;sy<scale;++sy) for(unsigned sx=0;sx<scale;++sx)
                    pixel(frame,x+col*scale+sx,y+row*scale+sy);
        x+=9*scale; ++columns;
    }
}
static void text(uint8_t *frame,unsigned x,unsigned y,const char *s)
{ text_size(frame,x,y,s,2); }
static void text_grid(uint8_t *frame,unsigned x,unsigned y,const char *s)
{
#ifdef INK_USE_FONTS
    ink_font_text(frame,x,y,s,18,32,256,464,pixel);
#else
    text(frame,x,y,s);
#endif
}
static void box(uint8_t *frame,unsigned x,unsigned y,unsigned w,unsigned h)
{
    for(unsigned i=0;i<w;++i) { pixel(frame,x+i,y); pixel(frame,x+i,y+h-1); }
    for(unsigned i=0;i<h;++i) { pixel(frame,x,y+i); pixel(frame,x+w-1,y+i); }
}
static void new_file(const ink_browser *b,uint8_t *frame)
{
    ink_icon(frame,224,24,ICON_ADD,pixel);
    box(frame,16,96,448,64);
    size_t length=strlen(b->new_name);
    const char *visible=b->new_name+(length>23?length-23:0);
    text_grid(frame,24,108,visible);
    unsigned cursor=24+(unsigned)strlen(visible)*18;
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
            if(key==INK_KEY_SYMBOLS) text_size(frame,x+(w-27)/2,y+19,keyboard->symbols?"abc":"#+=",1);
            else ink_icon(frame,x+(w-32)/2,y+12,key==INK_KEY_INDENT?ICON_TAB:key==INK_KEY_SHIFT?ICON_SHIFT:
                key==' '?ICON_SPACE:key==INK_KEY_DELETE?ICON_DELETE:ICON_ENTER,pixel);
        } else {
            char label[2]={(char)key,0};
#ifdef INK_USE_FONTS
            ink_font_text(frame,x+(w-18)/2,y+10,label,18,32,0,480,pixel);
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
    memset(frame,0xff,48000);
    ink_icon(frame,16,8,ICON_CLOCK,pixel); text_size(frame,64,16,p->time[0]?p->time:"--",1);
    ink_icon(frame,16,48,ICON_BATTERY,pixel); text_size(frame,64,56,p->battery[0]?p->battery:"--",1);
    if(p->view==1) {
        for(unsigned i=0;i<6&&p->font_first+i<p->font_count;i++) {
            box(frame,32,128+i*64,416,64); text_size(frame,48,150+i*64,p->font_names[i],1);
        }
        box(frame,32,576,416,64); ink_icon(frame,224,592,ICON_BACK,pixel);
        if(p->message[0]) ink_icon(frame,224,736,ICON_ERROR,pixel);
        return;
    }
    if(p->view==2) {
        for(unsigned i=0;i<5;i++) {
            unsigned y=128+i*64; char label[32]; snprintf(label,sizeof(label),"%04d",p->calendar[i]);
            box(frame,32,y,416,64); text_size(frame,48,y+22,label,1);
            box(frame,288,y,80,64); box(frame,368,y,80,64); text(frame,320,y+16,"-"); text(frame,400,y+16,"+");
        }
        box(frame,32,512,416,64); ink_icon(frame,224,528,ICON_SAVE,pixel);
        box(frame,32,576,416,64); ink_icon(frame,224,592,ICON_BACK,pixel);
        if(p->message[0]) ink_icon(frame,224,736,ICON_ERROR,pixel);
        return;
    }
    const unsigned icons[]={ICON_SUN,ICON_WARM,ICON_LIGHT,ICON_MOON,ICON_ROTATE,ICON_CLOCK,ICON_FONT,ICON_REFRESH,ICON_CLOSE};
    for(unsigned row=0;row<9;row++) {
        unsigned y=128+row*64; box(frame,32,y,416,64);
        if(row<2) {
            char label[20]; snprintf(label,sizeof(label),"%u",row==0?p->brightness:p->warmth);
            ink_icon(frame,48,y+16,icons[row],pixel); text(frame,112,y+16,label);
            box(frame,288,y,80,64); box(frame,368,y,80,64);
            text(frame,320,y+16,"-"); text(frame,400,y+16,"+");
        } else { ink_icon(frame,224,y+16,icons[row],pixel);
            if((row==2&&p->on)||(row==3&&p->night)||(row==4&&p->landscape)) box(frame,276,y+26,12,12); }
    }
    if(p->message[0]) ink_icon(frame,224,736,ICON_ERROR,pixel);
}
void ink_console_draw(const ink_console *c,uint8_t frame[48000])
{
    memset(frame,0xff,48000);
    if(c->menu) {
        const unsigned icons[]={ICON_STOP,ICON_CLOSE,ICON_BACK};
        for(unsigned i=0;i<3;i++) {
            box(frame,32,180+i*64,416,64);
            ink_icon(frame,224,196+i*64,icons[i],pixel);
        }
        return;
    }
    for(unsigned row=0;row<INK_CONSOLE_PAGE_ROWS;row++) text_grid(frame,16,16+row*34,ink_console_line(c,row));
    box(frame,16,464,448,48);
    const char *visible=c->input+(c->used>23?c->used-23:0);
    char line[24]; size_t i=0;
    for(;visible[i] && i<23;i++) line[i]=visible[i]=='\n'?' ':visible[i];
    line[i]=0; text(frame,24,472,line);
    keyboard_draw_at(&c->keyboard,frame,INK_CONSOLE_KEYBOARD_OFFSET);
    if(c->full) ink_icon(frame,224,426,ICON_ERROR,pixel);
}
void ink_editor_draw(const ink_editor *e,uint8_t frame[48000])
{
    memset(frame,0xff,48000);
    if(e->menu) {
        const char *name=strrchr(e->path,'/'); text(frame,16,8,name?name+1:e->path);
        const unsigned icons[]={ICON_SAVE,ICON_CLOSE,ICON_BACK};
        for(unsigned i=0;i<3;i++) { box(frame,32,180+i*64,416,64); ink_icon(frame,224,196+i*64,icons[i],pixel); }
    } else {
        for(unsigned row=0;row<e->rows;row++) text_grid(frame,16,64+row*34,e->lines[row]);
        unsigned x=16+e->cursor_column*18,y=64+e->cursor_row*34;
        for(unsigned i=0;i<32;i++) pixel(frame,x,y+i);
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
        for(unsigned row=0;row<b->text.rows;++row) text(frame,16,16+row*34,b->text.lines[row]);
        return;
    }
    if(b->view==INK_FILE_MENU) {
        const char *name=strrchr(b->selected,'/');
        text(frame,16,80,name?name+1:b->selected);
        box(frame,32,180,416,64); box(frame,32,244,416,64);
        ink_icon(frame,224,196,ICON_EDIT,pixel); ink_icon(frame,224,260,ICON_PLAY,pixel); return;
    }
    if(b->view!=INK_FILES) { ink_icon(frame,224,368,ICON_ERROR,pixel); return; }
    const char *folder=b->folder+strlen(b->root);
    if(*folder) text_size(frame,16,8,folder,1);
    box(frame,0,INK_BROWSER_ACTION_Y,240,72); box(frame,240,INK_BROWSER_ACTION_Y,240,72);
    ink_icon(frame,104,748,ICON_ADD,pixel); ink_icon(frame,344,748,ICON_CONSOLE,pixel);
    for(unsigned i=0;i<b->count;++i) {
        char line[258]; snprintf(line,sizeof(line),"%s%s",b->rows[i].name,b->rows[i].directory?"/":"");
        text(frame,16,INK_BROWSER_LIST_Y+i*42,line);
    }
}
