#include "ink_browser.h"
#include "ink_ui_font.h"
#include "ink_editor.h"
#include "ink_power.h"
#include "../ink_python/ink_console.h"
#include <stdio.h>
#include <string.h>

static void pixel(uint8_t *frame,unsigned x,unsigned y)
{
    if(x>=480 || y>=800) return;
    unsigned dx=799-y,dy=x;
    frame[dy*100+dx/8]&=(uint8_t)~(0x80>>(dx%8));
}
static void text_size(uint8_t *frame,unsigned x,unsigned y,const char *s,unsigned scale)
{
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
static void box(uint8_t *frame,unsigned x,unsigned y,unsigned w,unsigned h)
{
    for(unsigned i=0;i<w;++i) { pixel(frame,x+i,y); pixel(frame,x+i,y+h-1); }
    for(unsigned i=0;i<h;++i) { pixel(frame,x,y+i); pixel(frame,x+w-1,y+i); }
}
static void new_file(const ink_browser *b,uint8_t *frame)
{
    text(frame,16,8,"New file"); text(frame,16,56,"Filename and extension");
    box(frame,16,96,448,64);
    size_t length=strlen(b->new_name);
    const char *visible=b->new_name+(length>23?length-23:0);
    text(frame,24,108,visible);
    unsigned cursor=24+(unsigned)strlen(visible)*18;
    for(unsigned y=110;y<140;++y) pixel(frame,cursor,y);
    text(frame,16,180,b->message);
    /* Shared keyboard draw follows below. */
}
static void keyboard_draw_at(const ink_keyboard *keyboard,uint8_t *frame,unsigned offset)
{
    for(unsigned row=0;row<5;++row) for(unsigned col=0;col<(row==4?5u:10u);++col) {
        int key=ink_keyboard_key(keyboard,row,col); if(!key) continue;
        unsigned w=INK_KB_WIDTH*(row==4?2:1),x=INK_KB_X+col*w,y=INK_KB_Y+offset+row*INK_KB_HEIGHT;
        box(frame,x+1,y+1,w-2,INK_KB_HEIGHT-2);
        if(row==4 || key==INK_KEY_INDENT) {
            const char *label=key==INK_KEY_INDENT?"Tab":key==INK_KEY_SHIFT?(keyboard->shift?"SHIFT":"Shift"):
                key==INK_KEY_SYMBOLS?(keyboard->symbols?"abc":"#+="):
                key==' '?"Space":key==INK_KEY_DELETE?"Del":"Enter";
            text_size(frame,x+(w-(unsigned)strlen(label)*9)/2,y+19,label,1);
        } else {
            char label[2]={(char)key,0}; text(frame,x+(w-18)/2,y+10,label);
        }
    }
}
static void keyboard_draw(const ink_keyboard *keyboard,uint8_t *frame)
{ keyboard_draw_at(keyboard,frame,0); }
void ink_power_draw(const ink_power *p,uint8_t frame[48000])
{
    memset(frame,0xff,48000);
    text(frame,16,8,p->time[0]?p->time:"Time: unavailable");
    text(frame,16,48,"Battery: unavailable");
    const char *labels[]={"Brightness","Warmth",p->on?"Light: on":"Light: off",
        p->night?"Night mode: on":"Night mode: off","Orientation","Time settings",
        "Font selector","Refresh screen","Close"};
    for(unsigned row=0;row<9;row++) {
        unsigned y=128+row*64; box(frame,32,y,416,64);
        if(row<2) {
            char label[20]; snprintf(label,sizeof(label),"%s %u",labels[row],row==0?p->brightness:p->warmth);
            text_size(frame,48,y+22,label,1);
            box(frame,288,y,80,64); box(frame,368,y,80,64);
            text(frame,320,y+16,"-"); text(frame,400,y+16,"+");
        } else text(frame,48,y+16,labels[row]);
    }
    text_size(frame,16,736,p->message,1);
}
void ink_console_draw(const ink_console *c,uint8_t frame[48000])
{
    memset(frame,0xff,48000);
    text(frame,16,8,"Python console");
    if(c->menu) {
        text(frame,16,80,c->busy?"Python is running":"Console is idle");
        const char *labels[]={"Stop process","Close console","Cancel"};
        for(unsigned i=0;i<3;i++) {
            box(frame,32,180+i*64,416,64);
            text(frame,48,196+i*64,labels[i]);
        }
        return;
    }
    char status[32]; snprintf(status,sizeof(status),"%s  History %u",c->busy?"Running":c->more?"...":">>>",c->page+1);
    text(frame,16,48,status);
    for(unsigned row=0;row<INK_CONSOLE_PAGE_ROWS;row++) text(frame,16,92+row*34,ink_console_line(c,row));
    box(frame,16,342,448,48);
    const char *visible=c->input+(c->used>23?c->used-23:0);
    char line[24]; size_t i=0;
    for(;visible[i] && i<23;i++) line[i]=visible[i]=='\n'?' ':visible[i];
    line[i]=0; text(frame,24,350,line);
    keyboard_draw(&c->keyboard,frame);
    text(frame,16,730,c->full?"Input full":"Home: console menu");
}
void ink_editor_draw(const ink_editor *e,uint8_t frame[48000])
{
    memset(frame,0xff,48000);
    if(e->menu) {
        const char *name=strrchr(e->path,'/'); text(frame,16,8,name?name+1:e->path);
        const char *labels[]={"Save","Discard","Cancel"};
        for(unsigned i=0;i<3;i++) { box(frame,32,180+i*64,416,64); text(frame,48,196+i*64,labels[i]); }
    } else {
        for(unsigned row=0;row<e->rows;row++) text(frame,16,64+row*34,e->lines[row]);
        unsigned x=16+e->cursor_column*18,y=64+e->cursor_row*34;
        for(unsigned i=0;i<32;i++) pixel(frame,x,y+i);
        keyboard_draw_at(&e->keyboard,frame,INK_EDITOR_KEYBOARD_OFFSET);
    }
    if(e->error[0]) text(frame,16,110,e->error);
}
void ink_browser_draw(const ink_browser *b,uint8_t frame[48000])
{
    memset(frame,0xff,48000);
    if(b->view==INK_NEW_FILE) {
        new_file(b,frame); keyboard_draw(&b->keyboard,frame);
        box(frame,16,720,224,56); box(frame,240,720,224,56);
        text(frame,74,730,"Cancel"); text(frame,298,730,"Create"); return;
    }
    if(b->view==INK_OPEN_TEXT) {
        for(unsigned row=0;row<b->text.rows;++row) text(frame,16,16+row*34,b->text.lines[row]);
        return;
    }
    if(b->view==INK_FILE_MENU) {
        const char *name=strrchr(b->selected,'/');
        text(frame,16,80,name?name+1:b->selected);
        box(frame,32,180,416,64); box(frame,32,244,416,64);
        text(frame,48,196,"Edit"); text(frame,48,260,"Execute"); return;
    }
    if(b->view==INK_OPEN_CONSOLE || b->view==INK_EXECUTE_PYTHON || b->view==INK_EDIT_TEXT) {
        text(frame,16,80,b->view==INK_EDIT_TEXT?"Editor: pending":"Python runtime: pending");
        text(frame,16,300,"Home: back"); return;
    }
    if(b->view!=INK_FILES) {
        const char *name=strrchr(b->selected,'/');
        text(frame,16,80,b->view==INK_NOTICE?b->message:
             b->view==INK_OPEN_MARKDOWN?"Markdown selected":"Text selected");
        if(b->view!=INK_NOTICE) {
            text(frame,16,130,name?name+1:b->selected);
            text(frame,16,200,"Reader: next stage");
        }
        text(frame,16,300,"Home: back"); return;
    }
    text(frame,16,52,"New file"); text(frame,256,52,"Console");
    const char *folder=b->folder+strlen(b->root);
    text(frame,16,100,*folder?folder:"/");
    for(unsigned x=16;x<464;++x) pixel(frame,x,140);
    for(unsigned i=0;i<b->count;++i) {
        char line[258]; snprintf(line,sizeof(line),"%s%s",b->rows[i].name,b->rows[i].directory?"/":"");
        text(frame,16,148+i*42,line);
    }
    if(!b->count) text(frame,16,148,"Empty folder");
}
