#include "ink_dict.h"
#include <assert.h>
#include <string.h>
static unsigned progress_calls;
static void progress(void){progress_calls++;}
static void select_dictionary(ink_dict *d,const char *root,const char *name)
{ char p[512]; snprintf(p,sizeof(p),"%s/dictionaries/%s/%s.ifo",root,name,name); assert(!ink_dict_select(d,p)); }
static bool has(ink_dict *d,const char *text)
{ for(unsigned i=0;i<INK_DICT_LINES;i++) if(strstr(d->lines[i],text)) return true; return false; }
int main(int argc,char **argv)
{
    assert(argc==2); ink_dict d; ink_dict_init(&d,argv[1],progress);
    assert(!ink_dict_catalog(&d,0)&&d.total>=10&&d.count==8);
    assert(!ink_dict_catalog(&d,8)&&d.count>=2);
    select_dictionary(&d,argv[1],"plain");
    progress_calls=0;select_dictionary(&d,argv[1],"plain");assert(!progress_calls);
    char cache[560];snprintf(cache,sizeof(cache),"%s/ord",d.cache);
    ink_dict_close(&d);FILE *broken=fopen(cache,"wb");assert(broken);fclose(broken);
    progress_calls=0;select_dictionary(&d,argv[1],"plain");assert(progress_calls);
    snprintf(cache,sizeof(cache),"%s/dictionaries/plain/plain.dict",argv[1]);
    FILE *changed=fopen(cache,"ab");assert(changed);fputc('\n',changed);fclose(changed);
    progress_calls=0;select_dictionary(&d,argv[1],"plain");assert(progress_calls);
    assert(!ink_dict_lookup(&d,"APPLE")&&has(&d,"a red fruit")&&has(&d,"second meaning"));
    assert(!ink_dict_lookup(&d,"pomme")&&has(&d,"a red fruit")&&has(&d,"second meaning"));
    assert(ink_dict_lookup(&d,"absent")==1&&has(&d,"Word not found"));
    assert(!ink_dict_lookup(&d,"café")&&has(&d,"coffee"));
    select_dictionary(&d,argv[1],"zip");
    progress_calls=0;select_dictionary(&d,argv[1],"zip");assert(!progress_calls);
    assert(!ink_dict_lookup(&d,"large")&&d.pages>100);
    assert(!ink_dict_page(&d,1)&&d.page==1); assert(!ink_dict_page(&d,-1)&&d.page==0);
    assert(!ink_dict_lookup(&d,"markup")&&has(&d,"fish & chips")&&has(&d,"café"));
    select_dictionary(&d,argv[1],"xdxf"); assert(!ink_dict_lookup(&d,"word")&&has(&d,"XML meaning"));
    select_dictionary(&d,argv[1],"binary-last"); assert(!ink_dict_lookup(&d,"word")&&has(&d,"plain text")&&has(&d,"Media omitted"));
    select_dictionary(&d,argv[1],"typed");
    assert(!ink_dict_lookup(&d,"word")&&has(&d,"phonetic")&&has(&d,"meaning"));
    assert(!ink_dict_lookup(&d,"empty")&&has(&d,"No textual definition"));
    select_dictionary(&d,argv[1],"mixed");
    assert(!ink_dict_lookup(&d,"word")&&has(&d,"first")&&has(&d,"Media omitted")&&has(&d,"last"));
    select_dictionary(&d,argv[1],"wide");
    assert(d.offset_bytes==8&&!ink_dict_lookup(&d,"word")&&has(&d,"64 bit offset"));
    select_dictionary(&d,argv[1],"invalid-text"); assert(ink_dict_lookup(&d,"word")==-1);
    char p[512]; const char *bad[]={"bad-index","bad-gzip","bad-size"};
    for(unsigned i=0;i<3;i++) { snprintf(p,sizeof(p),"%s/dictionaries/%s/%s.ifo",argv[1],bad[i],bad[i]); assert(ink_dict_select(&d,p)==-1&&!d.ready); }
    select_dictionary(&d,argv[1],"plain"); ink_dict_close(&d);
    ink_dict_init(&d,argv[1],NULL); assert(strstr(d.selected,"plain.ifo"));
    assert(!ink_dict_lookup(&d,"apple")&&has(&d,"a red fruit")); ink_dict_close(&d);
    puts("PASS: discovery/paging, plain/gzip/dictzip, 32/64-bit offsets, homographs, aliases, UTF-8, HTML/entities, typed fields/media omission, large paged definitions, choice persistence, invalid input rejection");
}
