#include "ink_browser.h"
#include "ink_ui_font.h"
#include <stdio.h>
#include <string.h>

static void pixel(uint8_t *frame,unsigned x,unsigned y)
{
    if(x>=480 || y>=800) return;
    unsigned dx=799-y,dy=x;
    frame[dy*100+dx/8]&=(uint8_t)~(0x80>>(dx%8));
}
static void text(uint8_t *frame,unsigned x,unsigned y,const char *s)
{
    unsigned columns=0;
    for(;*s && columns<25;++s) {
        unsigned char c=(unsigned char)*s;
        if((c&0xc0)==0x80) continue; /* One visible fallback per non-ASCII codepoint. */
        if(c<32 || c>126) c='?';
        if(x+18>464) break;
        for(unsigned row=0;row<16;++row) for(unsigned col=0;col<9;++col)
            if(ink_ui_font[c-32][row]&(1u<<col))
                for(unsigned sy=0;sy<2;++sy) for(unsigned sx=0;sx<2;++sx)
                    pixel(frame,x+col*2+sx,y+row*2+sy);
        x+=18; ++columns;
    }
}
void ink_browser_draw(const ink_browser *b,uint8_t frame[48000])
{
    memset(frame,0xff,48000);
    text(frame,16,8,"InkPy");
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
