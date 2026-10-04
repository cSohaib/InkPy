#define _POSIX_C_SOURCE 200809L
#include "ink_reader.h"
#include "ink_math.h"
#include "ink_font.h"
#include <assert.h>
#include <string.h>
static int render(const char *s,int display,unsigned pixels,uint8_t *bits,unsigned *w,unsigned *h,unsigned *base)
{
    ink_math_result r;int status=ink_math_render(s,display,(int)pixels,bits,&r);
    if(status) fprintf(stderr,"%s: %s\n",s,r.error);
    *w=r.width;*h=r.height;*base=r.baseline;return status;
}
static void pages(ink_reader *r,const char *folder,const char *prefix)
{
    uint8_t frame[48000];char path[1024];
    for(r->page=1;r->page<=r->stats.pages;r->page++) {
        assert(!ink_reader_draw(r,frame));
        snprintf(path,sizeof(path),"%s/%s-%u.bin",folder,prefix,r->page);
        FILE *f=fopen(path,"wb");assert(f);assert(fwrite(frame,1,sizeof(frame),f)==sizeof(frame));assert(!fclose(f));
    }
    r->page=1;
}
int main(int argc,char **argv)
{
    assert(argc==4);ink_reader r;char error[160],path[1024];
    assert(!ink_font_init());assert(!ink_math_init(argv[3],error,sizeof(error)));
    assert(!ink_reader_open(&r,argv[1],argv[2],render,NULL));
    assert(r.stats.formulas==9&&r.stats.math_fallbacks==0&&r.stats.chapters==3);
    pages(&r,argv[2],"table");
    assert(!ink_reader_rotate(&r,argv[1],argv[2],render,NULL));
    pages(&r,argv[2],"table-landscape");ink_reader_close(&r);
    snprintf(path,sizeof(path),"%s/tall-table.md",argv[2]);
    FILE *f=fopen(path,"wb");assert(f);
    fputs("| Left | Mid | Right |\n| --- | :---: | ---: |\n| ",f);
    for(unsigned i=0;i<1000;i++) fputc('x',f);
    fputs(" | $\\bar{x}$ | end |\n",f);
    for(unsigned i=0;i<40;i++) fprintf(f,"| row %u | $\\mu$ | café |\n",i);
    fputs("\nAfter the table.\n",f);assert(!fclose(f));
    assert(!ink_reader_open(&r,path,argv[2],render,NULL));
    assert(r.stats.pages>5&&r.stats.formulas==41&&!r.stats.math_fallbacks&&!r.stats.literal_blocks);
    pages(&r,argv[2],"tall");
    rewind(r.draw);unsigned x_count=0,rules=0;unsigned char h[20];
    while(fread(h,1,20,r.draw)==20) {
        unsigned size=h[8]|h[9]<<8|h[10]<<16|h[11]<<24,style=h[6]|h[7]<<8;
        if(style==INK_RULE) rules++;
        for(unsigned i=0;i<size;i++) { int c=fgetc(r.draw);assert(c!=EOF);if(style!=INK_BITMAP&&style!=INK_RULE&&c=='x')x_count++; }
    }
    assert(x_count==1000&&rules>40);ink_reader_close(&r);
    assert(!ink_reader_open(&r,argv[1],argv[2],NULL,NULL));
    assert(!r.stats.formulas&&!r.stats.literal_blocks);pages(&r,argv[2],"no-math");ink_reader_close(&r);
    ink_math_shutdown();puts("PASS: all requested math commands, bordered/aligned/wrapped tables, inline math, tall-row/page continuation, landscape and missing-math fallback");
}
