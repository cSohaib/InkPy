#include "ink_font.h"
#include "ink_math.h"
#include "ink_reader.h"
#include "ink_epub.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
static unsigned progress_calls;
static void progress(void){progress_calls++;}
static int render(const char *s,int display,unsigned pixels,uint8_t *bits,unsigned *w,unsigned *h,unsigned *base)
{
    ink_math_result r;int status=ink_math_render(s,display,(int)pixels,bits,&r);
    *w=r.width;*h=r.height;*base=r.baseline;return status;
}
static unsigned images(ink_reader *r)
{
    rewind(r->draw);unsigned char h[20];unsigned count=0;
    while(fread(h,1,20,r->draw)==20){unsigned style=h[6]|h[7]<<8,size=h[8]|h[9]<<8|h[10]<<16|h[11]<<24;
        if(style==INK_BITMAP)count++;
        assert(!fseek(r->draw,(long)size,SEEK_CUR));}
    return count;
}
static void screen(ink_reader *r,const char *root,const char *prefix,unsigned serial)
{
    uint8_t frame[48000];char path[1024];assert(!ink_reader_draw(r,frame));
    snprintf(path,sizeof(path),"%s/%s-%u.bin",root,prefix,serial);FILE *f=fopen(path,"wb");assert(f);
    assert(fwrite(frame,1,sizeof(frame),f)==sizeof(frame));assert(!fclose(f));
}
int main(int argc,char **argv)
{
    assert(argc==5);ink_reader r;char error[160];assert(!ink_font_init());
    assert(!ink_math_init(argv[3],error,sizeof(error)));ink_epub_debug_start(argv[2],argv[1]);
    assert(!ink_reader_open(&r,argv[1],argv[2],render,progress));bool fallback=!strcmp(argv[4],"fallback");
    assert(r.epub&&r.documents==2&&r.stats.chapters==(fallback?2u:3u));
    const char *names[]={"Chapter One","TOC Section","Chapter Two"};ink_epub_chapter c;
    for(unsigned i=1;i<=3&&!fallback;i++){assert(!ink_epub_get_chapter(r.book_cache,i,&c));assert(!strcmp(c.title,names[i-1]));}
    /* Only the first spine item is imported. Later chapters/images are untouched. */
    char unseen[1024];snprintf(unseen,sizeof(unseen),"%s/d2/epub-body",r.book_cache);struct stat st;assert(stat(unseen,&st));
    unsigned before=progress_calls;unsigned page=r.page;uint64_t bytes=r.local_stats.source_bytes;
    uint8_t original[48000],again[48000];assert(!ink_reader_draw(&r,original));
    ink_reader_close(&r);assert(!ink_reader_open(&r,argv[1],argv[2],render,progress));
    assert(progress_calls==before&&r.page==page&&r.local_stats.source_bytes==bytes);
    assert(!ink_reader_draw(&r,again)&&!memcmp(original,again,sizeof(original)));
    if(fallback) {
        assert(!ink_epub_get_chapter(r.book_cache,1,&c)&&!strcmp(c.title,"Unused title"));
        ink_reader_home(&r);assert(!ink_reader_tap(&r,48,200));assert(!ink_reader_tap(&r,48,184));
        assert(r.document==2);assert(!ink_reader_draw(&r,again));assert(!strcmp(r.title,"Second chapter"));
        ink_reader_close(&r);ink_math_shutdown();puts("PASS: missing TOC uses document titles instead of filenames");return 0;
    }
    if(!strcmp(argv[4],"heavy")) {
        char body[1024];snprintf(body,sizeof(body),"%s/epub-body",r.cache);assert(!stat(body,&st)&&st.st_size>100000);
        assert(r.stats.pages<10&&r.local_stats.source_bytes<4096);
        ink_reader_page(&r,1);assert(!r.error[0]);ink_reader_page(&r,-1);assert(!r.error[0]);
        ink_reader_close(&r);ink_math_shutdown();puts("PASS: heavy chapter has no full pagination at open; cached reopen and Next/Previous work");return 0;
    }
    unsigned serial=0;
    while(!(r.document==r.documents&&r.eof&&r.page==r.stats.pages)) {
        screen(&r,argv[2],argv[4],++serial);unsigned doc=r.document,old=r.page;
        ink_reader_page(&r,1);assert(!r.error[0]);
        assert(r.document!=doc||r.page!=old||r.eof);assert(serial<200);
    }
    screen(&r,argv[2],argv[4],++serial);assert(images(&r)>=1);
    /* Chapter navigation can load a document directly, including fragment targets. */
    ink_reader_home(&r);assert(!ink_reader_tap(&r,48,200));assert(!ink_reader_tap(&r,48,184));
    assert(r.document==1&&r.view==INK_READER_PAGE);assert(!ink_reader_draw(&r,again));
    assert(!strcmp(r.title,"TOC Section"));
    before=progress_calls;page=r.page;bytes=r.local_stats.source_bytes;ink_reader_close(&r);
    assert(!ink_reader_open(&r,argv[1],argv[2],render,progress));assert(progress_calls==before&&r.page==page&&r.local_stats.source_bytes==bytes);
    assert(!ink_reader_rotate(&r,argv[1],argv[2],render,progress)&&r.landscape);
    screen(&r,argv[2],"landscape",1);ink_reader_home(&r);
    assert(ink_reader_tap(&r,48,260)==INK_READER_ROTATE);ink_reader_close(&r);ink_math_shutdown();
    puts("PASS: lazy EPUB, TOC fragments/H2 exclusion, images/math/tables, Previous/Next, persistent continuation/reopen and rotation");
}
