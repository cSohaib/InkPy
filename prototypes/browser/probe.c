#include "ink_browser.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned row(ink_browser *b,const char *name)
{ for(unsigned i=0;i<b->count;++i) if(!strcmp(b->rows[i].name,name)) return i; assert(!"missing fixture"); return 0; }
int main(int argc,char **argv)
{
    assert(argc==3);
    ink_browser b; assert(!ink_browser_init(&b,argv[1]));
    assert(b.count==INK_BROWSER_ROWS && b.has_next);
    assert(!ink_browser_home(&b));
    assert(ink_browser_page(&b,1)); assert(b.count==6 && !b.has_next);
    assert(!ink_browser_page(&b,1)); assert(ink_browser_page(&b,-1));
    /* Locate entries without imposing sort order. */
    while(1) {
        for(unsigned i=0;i<b.count;++i) if(b.rows[i].directory) {
            ink_browser_tap(&b,20,148+i*42); assert(b.count==3);
            unsigned n=row(&b,"book.md"); ink_browser_tap(&b,20,148+n*42); assert(b.view==INK_OPEN_MARKDOWN);
            ink_browser_home(&b); n=row(&b,"script.py"); ink_browser_tap(&b,20,148+n*42); assert(b.view==INK_OPEN_TEXT);
            ink_browser_home(&b); n=row(&b,"binary.bin"); ink_browser_tap(&b,20,148+n*42); assert(b.view==INK_NOTICE);
            ink_browser_home(&b); ink_browser_home(&b); assert(!strcmp(b.folder,b.root));
            goto complete;
        }
        assert(ink_browser_page(&b,1));
    }
complete:
    uint8_t frame[48000]; ink_browser_draw(&b,frame);
    FILE *out=fopen(argv[2],"wb"); assert(out);
    fputs("P4\n800 480\n",out);
    /* PBM black=1, panel white=1. */
    for(unsigned i=0;i<sizeof(frame);++i) fputc(frame[i]^255,out);
    assert(!fclose(out)); puts("OK: paging, folders, Home, Markdown/text routing, binary notice");
}
