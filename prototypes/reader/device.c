#define _POSIX_C_SOURCE 200809L
#include "ink_reader.h"
#include "ink_math.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#ifdef INK_USE_FONTS
#include "ink_font.h"
#endif
static int render(const char *s,int display,unsigned pixels,uint8_t *bits,unsigned *w,unsigned *h,unsigned *base)
{
    ink_math_result r; int status=ink_math_render(s,display,(int)pixels,bits,&r);
    *w=(unsigned)r.width; *h=(unsigned)r.height; *base=(unsigned)r.baseline; return status;
}
int main(int argc,char **argv)
{
    assert(argc==4); ink_reader r; uint8_t frame[48000]; char error[160];
#ifdef INK_USE_FONTS
    assert(!ink_font_init());
#endif
    assert(!ink_math_init(argv[3],error,sizeof(error)));
    assert(!ink_reader_open(&r,argv[1],argv[2],render,NULL));
    assert(r.stats.chapters==3&&r.stats.formulas==6&&r.stats.math_fallbacks==1&&r.stats.pages>1);
    r.page=2; assert(!ink_reader_draw(&r,frame));
    char path[1024]; snprintf(path,sizeof(path),"%s/page.bin",argv[2]);
    FILE *out=fopen(path,"wb"); assert(out); assert(fwrite(frame,1,sizeof(frame),out)==sizeof(frame)); assert(!fclose(out));
    ink_reader_home(&r); assert(r.view==INK_READER_MENU); assert(!ink_reader_draw(&r,frame));
    snprintf(path,sizeof(path),"%s/menu.bin",argv[2]); out=fopen(path,"wb"); assert(out);
    assert(fwrite(frame,1,sizeof(frame),out)==sizeof(frame)); assert(!fclose(out));
    assert(!ink_reader_tap(&r,48,200)); assert(r.view==INK_READER_CHAPTERS);
    assert(!ink_reader_tap(&r,48,120+64*2)); assert(r.view==INK_READER_PAGE);
    assert(!ink_reader_draw(&r,frame)); assert(r.chapter==3&&!strcmp(r.title,"Readable fallback"));
    ink_reader_home(&r); assert(!ink_reader_tap(&r,48,260)); assert(r.view==INK_READER_GOTO);
    assert(!ink_reader_tap(&r,48,240)); assert(!strcmp(r.digits,"1"));
    assert(!ink_reader_tap(&r,340,480)); assert(r.view==INK_READER_PAGE&&r.page==1);
    ink_reader_page(&r,1); assert(r.page==2); ink_reader_page(&r,-1); assert(r.page==1);
    ink_reader_home(&r);assert(!ink_reader_tap(&r,48,464)&&r.view==INK_READER_PAGE);
    ink_reader_home(&r); assert(ink_reader_tap(&r,48,330)); ink_reader_close(&r);
    snprintf(path,sizeof(path),"%s/.inkpy-reader/draw",argv[2]); struct stat st; assert(stat(path,&st));
    assert(!ink_reader_open(&r,argv[1],argv[2],NULL,NULL));
    assert(r.stats.chapters==3&&r.stats.formulas==0); assert(!ink_reader_draw(&r,frame)); ink_reader_close(&r);
    snprintf(path,sizeof(path),"%s/binary.md",argv[2]); out=fopen(path,"wb"); assert(out); fputs("text",out); fputc(0,out); fclose(out);
    assert(ink_reader_open(&r,path,argv[2],NULL,NULL)); assert(!r.draw&&!r.pages&&!r.chapters);
#ifdef INK_USE_FONTS
    assert(!ink_reader_open(&r,argv[1],argv[2],render,NULL));
    r.page=(unsigned)r.stats.pages; assert(!ink_reader_draw(&r,frame));
    unsigned chapter_before=r.chapter;
    ink_reader_home(&r);
    FILE *page_draw=r.draw; r.draw=NULL; /* A menu must not render/read page runs. */
    assert(!ink_reader_draw(&r,frame)); r.draw=page_draw;
    assert(ink_reader_tap(&r,48,400)==INK_READER_ROTATE);
    assert(!ink_reader_rotate(&r,argv[1],argv[2],render,NULL)&&r.landscape);
    assert(!ink_reader_draw(&r,frame)&&r.chapter==chapter_before);
    snprintf(path,sizeof(path),"%s/landscape.bin",argv[2]); out=fopen(path,"wb"); assert(out);
    assert(fwrite(frame,1,sizeof(frame),out)==sizeof(frame)); assert(!fclose(out));
    ink_reader_home(&r); assert(!ink_reader_draw(&r,frame));
    assert(ink_reader_tap(&r,48,400)==INK_READER_ROTATE);
    assert(!ink_reader_rotate(&r,argv[1],argv[2],render,NULL)&&!r.landscape);
    assert(!ink_reader_draw(&r,frame)&&r.chapter==chapter_before); ink_reader_close(&r);
#endif
    ink_math_shutdown();
    puts("PASS: device renderer, formula bitmaps/fallback, H2 navigation, page entry/paging, Close cleanup, missing-math-assets fallback, binary rejection");
}
