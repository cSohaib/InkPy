#include "ink_font.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_SYNTHESIS_H
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
bool ink_view_landscape;

static FT_Library library;
static FT_Face face;
static char names[32][96],paths[32][512];
static unsigned count=1,selected;
#ifdef ESP_PLATFORM
extern const unsigned char font_start[] asm("_binary_InkPyMono_ttf_start");
extern const unsigned char font_end[] asm("_binary_InkPyMono_ttf_end");
#endif
unsigned ink_font_count(void) { return count; }
unsigned ink_font_selected(void) { return selected; }
const char *ink_font_name(unsigned i) { return i<count?names[i]:""; }
int ink_font_select(unsigned i)
{
    if(!library||i>=count) return -1;
    FT_Face next=NULL; FT_Error e;
    if(i) e=FT_New_Face(library,paths[i],0,&next);
#ifdef ESP_PLATFORM
    else e=FT_New_Memory_Face(library,font_start,font_end-font_start,0,&next);
#else
    else e=FT_New_Face(library,"/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",0,&next);
#endif
    if(e) return -1;
    if(FT_Select_Charmap(next,FT_ENCODING_UNICODE)) { FT_Done_Face(next); return -1; }
    if(face) FT_Done_Face(face);
    face=next; selected=i; return 0;
}
int ink_font_init(void)
{
    strcpy(names[0],"InkPy Mono");
    if(FT_Init_FreeType(&library)) return -1;
    DIR *d=opendir("/sd/fonts"); struct dirent *entry;
    if(d) {
        while(count<32&&(entry=readdir(d))) {
            const char *ext=strrchr(entry->d_name,'.');
            if(entry->d_name[0]=='.'||!ext||(strcasecmp(ext,".ttf")&&strcasecmp(ext,".otf"))) continue;
            int n=snprintf(paths[count],512,"/sd/fonts/%s",entry->d_name);
            if(n<0||n>=512) continue;
            snprintf(names[count],96,"%.95s",entry->d_name); count++;
        }
        closedir(d);
    }
    return ink_font_select(0);
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
void ink_font_text(uint8_t *frame,unsigned x,unsigned y,const char *s,
    unsigned cell,unsigned height,unsigned style,unsigned limit,
    void (*pixel)(uint8_t *,unsigned,unsigned))
{
    if(!face||!cell||height<4) return;
    /* Compact controls keep their fixed hit grid in landscape; glyphs retain
     * their aspect ratio instead of stretching with that grid. Reader pages
     * reflow at native 800x480 and use the ordinary branch. */
    bool compact=ink_view_landscape&&limit==464;
    unsigned glyph_height=compact?height*480/800:height;
    unsigned advance=compact&&!(style&256)?(cell*480/800)*480/800:cell;
    if(!advance) advance=1;
    FT_Set_Pixel_Sizes(face,0,glyph_height*7/8);
    const unsigned char *p=(const unsigned char *)s;
    while(*p&&x+cell<=limit) {
        unsigned cp=decode(&p); FT_UInt glyph=FT_Get_Char_Index(face,cp);
        if(!glyph) glyph=FT_Get_Char_Index(face,0xfffd);
        if(!glyph) glyph=FT_Get_Char_Index(face,'?');
        if(!FT_Load_Glyph(face,glyph,FT_LOAD_DEFAULT)) {
            if(style&1) FT_GlyphSlot_Embolden(face->glyph);
            if(style&2) FT_GlyphSlot_Oblique(face->glyph);
            if(!FT_Render_Glyph(face->glyph,FT_RENDER_MODE_MONO)) {
                FT_GlyphSlot g=face->glyph; FT_Bitmap *b=&g->bitmap;
                unsigned glyph_cell=compact?advance*800/480:cell;
                int top=(int)(glyph_height*3/4)-g->bitmap_top;
                int left=((int)glyph_cell-(int)b->width)/2;
                for(unsigned row=0;row<b->rows;row++) for(unsigned col=0;col<b->width;col++) {
                    int px=left+(int)col,py=top+(int)row;
                    if(px<0||px>=(int)glyph_cell||py<0||py>=(int)glyph_height) continue;
                    const unsigned char *bits=b->buffer+(b->pitch>=0?row:b->rows-1-row)*(unsigned)(b->pitch>=0?b->pitch:-b->pitch);
                    if(bits[col/8]&(0x80>>(col%8))) {
                        if(compact) {
                            unsigned nx=x*800/480+(unsigned)px,ny=y*480/800+(unsigned)py;
                            if(nx<800&&ny<480) { unsigned dx=799-nx,dy=479-ny; frame[dy*100+dx/8]&=(uint8_t)~(0x80>>(dx%8)); }
                        } else pixel(frame,x+(unsigned)px,y+(unsigned)py);
                    }
                }
            }
        }
        x+=advance;
    }
}
