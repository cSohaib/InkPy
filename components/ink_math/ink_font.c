#include "ink_font.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_SYNTHESIS_H

static FT_Library library;
static FT_Face face;
static unsigned pixel_height;
#ifdef ESP_PLATFORM
extern const unsigned char font_start[] asm("_binary_InkPyMono_ttf_start");
extern const unsigned char font_end[] asm("_binary_InkPyMono_ttf_end");
#endif
int ink_font_init(void)
{
    if(FT_Init_FreeType(&library)) return -1;
#ifdef ESP_PLATFORM
    if(FT_New_Memory_Face(library,font_start,font_end-font_start,0,&face)) return -1;
#else
    if(FT_New_Face(library,"/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",0,&face)) return -1;
#endif
    return FT_Select_Charmap(face,FT_ENCODING_UNICODE)?-1:0;
}
static unsigned decode(const unsigned char **p)
{
    unsigned c=*(*p)++; if(c<128) return c;
    unsigned n=c<0xe0?1:c<0xf0?2:3,value=c&((1u<<(6-n))-1);
    if(c<0xc2||c>0xf4) return 0xfffd;
    for(unsigned i=0;i<n;i++) { c=**p; if((c&0xc0)!=0x80) return 0xfffd; (*p)++; value=value*64+(c&63); }
    if(value>0x10ffff||(value>=0xd800&&value<=0xdfff)) return 0xfffd;
    return value;
}
static bool arabic_mark(unsigned cp)
{
    return (cp>=0x610&&cp<=0x61a)||(cp>=0x64b&&cp<=0x65f)||cp==0x670||
        (cp>=0x6d6&&cp<=0x6dc)||(cp>=0x6df&&cp<=0x6e4)||(cp>=0x6e7&&cp<=0x6e8)||
        (cp>=0x6ea&&cp<=0x6ed)||(cp>=0x8ca&&cp<=0x8e1)||(cp>=0x8e3&&cp<=0x8ff)||
        (cp>=0x300&&cp<=0x36f);
}
void ink_font_text(uint8_t *frame,unsigned x,unsigned y,const char *s,
    unsigned cell,unsigned height,unsigned style,unsigned limit,
    void (*pixel)(uint8_t *,unsigned,unsigned))
{
    if(!face||!cell||height<4) return;
    unsigned pixels=height*7/8;
    if(pixel_height!=pixels) {
        if(FT_Set_Pixel_Sizes(face,0,pixels)) return;
        pixel_height=pixels;
    }
    const unsigned char *p=(const unsigned char *)s;
    while(*p) {
        unsigned cp=decode(&p);bool shaped=(style&128)!=0,mark=shaped&&arabic_mark(cp);
        if(!mark&&x+cell>limit)break;
        FT_Face active=face;
        FT_UInt glyph=FT_Get_Char_Index(active,cp);
        if(!glyph) glyph=FT_Get_Char_Index(active,0xfffd);
        if(!glyph) glyph=FT_Get_Char_Index(active,'?');
        if(!FT_Load_Glyph(active,glyph,FT_LOAD_DEFAULT)) {
            if(style&1) FT_GlyphSlot_Embolden(active->glyph);
            if(style&2) FT_GlyphSlot_Oblique(active->glyph);
            if(!FT_Render_Glyph(active->glyph,FT_RENDER_MODE_MONO)) {
                FT_GlyphSlot g=active->glyph; FT_Bitmap *b=&g->bitmap;
                int top=(int)(height*3/4)-g->bitmap_top;
                int left=((int)cell-(int)b->width)/2;
                bool joined=shaped&&((cp>=0x600&&cp<=0x8ff)||(cp>=0xfb50&&cp<=0xfeff));
                unsigned advance=(unsigned)(g->advance.x>0?g->advance.x/64:cell);
                if(!advance)advance=cell;
                for(unsigned row=0;row<b->rows;row++) for(unsigned col=0;col<b->width;col++) {
                    int px=left+(int)col,py=top+(int)row;
                    if(py<0||py>=(int)height) continue;
                    const unsigned char *bits=b->buffer+(b->pitch>=0?row:b->rows-1-row)*(unsigned)(b->pitch>=0?b->pitch:-b->pitch);
                    if(bits[col/8]&(0x80>>(col%8))) {
                        if(joined) {
                            /* Scale the glyph's advance, not its ink bounding box:
                             * Arabic joining strokes meet at adjacent cell edges. */
                            int origin=(int)x-(mark?(int)cell:0);
                            int first=(g->bitmap_left+(int)col)*(int)cell/(int)advance;
                            int last=(g->bitmap_left+(int)col+1)*(int)cell/(int)advance;
                            if(last<=first)last=first+1;
                            for(int at=first;at<last;at++)if(origin+at>=0&&origin+at<(int)limit)
                                pixel(frame,(unsigned)(origin+at),y+(unsigned)py);
                        }else if(px>=0&&px<(int)cell)pixel(frame,x+(unsigned)px,y+(unsigned)py);
                    }
                }
            }
        }
        if(!mark)x+=cell;
    }
}
