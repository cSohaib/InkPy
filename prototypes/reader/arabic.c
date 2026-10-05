#include "ink_reader.h"
#include "ink_font.h"
#include "ink_math.h"
#include "ink_bidi.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
static unsigned decode(const unsigned char **p)
{
    unsigned c=*(*p)++;if(c<128)return c;
    unsigned n=c<0xe0?1:c<0xf0?2:3,value=c&((1u<<(6-n))-1);
    while(n--)value=(value<<6)|(*(*p)++&63);
    return value;
}
static int render(const char *s,int display,unsigned pixels,uint8_t *bits,unsigned *w,unsigned *h,unsigned *base)
{
    ink_math_result result;int status=ink_math_render(s,display,(int)pixels,bits,&result);
    *w=result.width;*h=result.height;*base=result.baseline;return status;
}
static void direction(const char *text,bool rtl)
{
    uint32_t logical[128];unsigned n=0;const unsigned char *p=(const unsigned char*)text;
    while(*p)logical[n++]=decode(&p);
    ink_bidi result;assert(!ink_bidi_shape(logical,n,&result));assert(result.rtl==rtl);
    bool seen[128]={0};
    for(unsigned i=0;i<n;i++){assert(result.map[i]>=0&&result.map[i]<(int)n&&!seen[result.map[i]]);seen[result.map[i]]=true;}
}
int main(int argc,char **argv)
{
    assert(argc==4);assert(!ink_font_init());char error[160];assert(!ink_math_init(argv[3],error,sizeof(error)));
    direction("123 العربية Python 456",true);direction("123 Latin العربية",false);direction("123 + ٤٥٦",false);
    direction("(العربية) Latin",true);direction("English العربية",false);
    ink_reader r;assert(!ink_reader_open(&r,argv[1],argv[2],render,NULL));assert(r.stats.formulas==1&&!r.stats.math_fallbacks);
    uint8_t frame[48000];char path[1024];bool tapped=false;
    for(unsigned page=1;page<=r.stats.pages;page++) {
        r.page=page;assert(!ink_reader_draw(&r,frame));
        snprintf(path,sizeof(path),"%s/arabic-%u.bin",argv[2],page);FILE *f=fopen(path,"wb");assert(f);assert(fwrite(frame,1,sizeof(frame),f)==sizeof(frame));fclose(f);
    }
    rewind(r.draw);unsigned char h[20];
    while(!tapped&&fread(h,1,sizeof(h),r.draw)==sizeof(h)) {
        unsigned style=h[6]|h[7]<<8,size=h[8]|h[9]<<8|h[10]<<16|h[11]<<24;
        if(style&INK_SHAPED&&style!=INK_BITMAP) {
            char payload[1027];assert(size<=1026&&fread(payload,1,size,r.draw)==size);
            unsigned visual=(unsigned char)payload[0]|(unsigned char)payload[1]<<8;payload[size]=0;
            if(!strcmp(payload+2+visual,"مرحبا")) {
                r.page=1;unsigned x=h[0]|h[1]<<8,y=h[2]|h[3]<<8;
                assert(!ink_reader_tap(&r,x+2,y+2));assert(!strcmp(r.word,"مرحبا"));tapped=true;
            }
        }else assert(!fseek(r.draw,size,SEEK_CUR));
    }
    assert(tapped);ink_reader_close(&r);
    assert(!ink_reader_open(&r,argv[1],argv[2],render,NULL));assert(!ink_reader_rotate(&r,argv[1],argv[2],render,NULL));
    assert(!ink_reader_draw(&r,frame));snprintf(path,sizeof(path),"%s/arabic-landscape.bin",argv[2]);FILE *f=fopen(path,"wb");assert(f);assert(fwrite(frame,1,sizeof(frame),f)==sizeof(frame));fclose(f);
    ink_reader_close(&r);ink_math_shutdown();
    puts("PASS: per-line first-letter direction, visual/logical map, Arabic/mixed/math/tables rendering, logical tap-word lookup, portrait/landscape");
}
