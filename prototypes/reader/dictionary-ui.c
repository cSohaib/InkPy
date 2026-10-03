#define _POSIX_C_SOURCE 200809L
#include "ink_reader.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
static bool has(ink_dict *d,const char *s)
{ for(unsigned i=0;i<INK_DICT_LINES;i++) if(strstr(d->lines[i],s)) return true; return false; }
static void snapshot(ink_reader *r,const char *name)
{
    uint8_t frame[48000]; assert(!ink_reader_draw(r,frame));
    const char *dir=getenv("INK_DICT_RESULTS"); if(!dir) return;
    char path[1024]; snprintf(path,sizeof(path),"%s/%s.bin",dir,name); FILE *f=fopen(path,"wb"); assert(f);
    assert(fwrite(frame,1,sizeof(frame),f)==sizeof(frame)); assert(!fclose(f));
}
static void choose(ink_reader *r,const char *name)
{
    ink_reader_tap(r,40,730); assert(r->view==INK_READER_DICTIONARIES);
    ink_dict *d=r->dictionary; unsigned found=INK_DICT_ROWS;
    for(unsigned page=0;page<2;page++) {
        for(unsigned i=0;i<d->count;i++) if(!strcmp(d->rows[i].name,name)) { found=i; break; }
        if(found<INK_DICT_ROWS) break;
        ink_reader_page(r,1);
    }
    assert(found<d->count); ink_reader_tap(r,40,136+found*64); assert(r->view==INK_READER_DEFINITION);
}
int main(int argc,char **argv)
{
    assert(argc==2); char source[512],path[512];
    snprintf(source,sizeof(source),"%s/book.md",argv[1]); FILE *f=fopen(source,"wb"); assert(f);
    assert(fputs("ap**pl**e, café, pomme.\n",f)>=0); assert(!fclose(f));
    ink_reader r; ink_dict d; ink_dict_init(&d,argv[1],NULL);
    assert(!ink_reader_open(&r,source,argv[1],NULL,NULL)); r.dictionary=&d;
    ink_reader_tap(&r,45,20); assert(r.view==INK_READER_DEFINITION&&!strcmp(r.word,"apple")&&has(&d,"a red fruit"));
    snapshot(&r,"definition"); unsigned page=r.page;
    ink_reader_tap(&r,40,730); assert(r.view==INK_READER_DICTIONARIES); snapshot(&r,"chooser");
    ink_reader_page(&r,1); assert(d.first==8); ink_reader_home(&r);
    assert(r.view==INK_READER_DEFINITION&&has(&d,"a red fruit"));
    ink_reader_home(&r); assert(r.view==INK_READER_PAGE&&r.page==page);
    ink_reader_tap(&r,87,20); assert(r.view==INK_READER_PAGE); /* comma */
    ink_reader_tap(&r,116,20); assert(!strcmp(r.word,"café")&&has(&d,"coffee"));
    ink_reader_home(&r); ink_reader_tap(&r,199,20); assert(!strcmp(r.word,"pomme")&&has(&d,"a red fruit"));
    choose(&r,"zip"); assert(!strcmp(d.name,"zip")&&has(&d,"Word not found")&&r.page==page);
    choose(&r,"plain"); assert(has(&d,"a red fruit"));
    ink_reader_tap(&r,400,730); assert(r.view==INK_READER_PAGE); ink_reader_close(&r);
    f=fopen(source,"wb"); assert(f); for(unsigned i=0;i<40;i++) assert(fputc('z',f)!=EOF); assert(!fclose(f));
    assert(!ink_reader_open(&r,source,argv[1],NULL,NULL)); r.dictionary=&d;
    ink_reader_tap(&r,20,50); assert(strlen(r.word)==40&&has(&d,"wrapped word definition")); ink_reader_close(&r);
    f=fopen(source,"wb"); assert(f); assert(fputs("word",f)>=0); assert(!fclose(f));
    snprintf(path,sizeof(path),"%s/dictionaries/invalid-text/invalid-text.ifo",argv[1]); assert(!ink_dict_select(&d,path));
    assert(!ink_reader_open(&r,source,argv[1],NULL,NULL)); r.dictionary=&d;
    ink_reader_tap(&r,20,20); assert(r.error[0]); ink_reader_tap(&r,40,730); ink_reader_home(&r); assert(r.error[0]);
    ink_reader_close(&r); ink_dict_close(&d);
    snprintf(path,sizeof(path),"%s/empty",argv[1]); assert(!mkdir(path,0700)); ink_dict_init(&d,path,NULL);
    assert(!ink_reader_open(&r,source,path,NULL,NULL)); r.dictionary=&d; ink_reader_tap(&r,20,20);
    assert(r.view==INK_READER_DEFINITION&&r.error[0]); ink_reader_tap(&r,40,730); assert(r.view==INK_READER_DICTIONARIES&&!d.total);
    ink_reader_home(&r); assert(r.view==INK_READER_DEFINITION); ink_reader_tap(&r,400,730); assert(r.view==INK_READER_PAGE);
    ink_reader_close(&r); ink_dict_close(&d);
    puts("PASS: styled-word hit testing, punctuation/UTF-8/alias taps, definition/chooser paging, dictionary switch and retry, modal Home/Close, page preservation, missing/error dictionary recovery");
}
