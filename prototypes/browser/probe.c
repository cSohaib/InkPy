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
    assert(!strcmp(b.rows[0].name,"Books")&&b.rows[0].directory);
    assert(!strcmp(b.rows[1].name,"z-folder"));
    assert(!strcmp(b.rows[2].name,"Notes-1.txt"));
    assert(!strcmp(b.rows[14].name,"Notes-3.txt"));
    assert(b.rows[1].directory);
    assert(!strcmp(b.rows[12].name,"notes-19.txt"));
    assert(!ink_browser_home(&b));
    assert(ink_browser_page(&b,1)); assert(b.count==21-INK_BROWSER_ROWS && !b.has_next);
    assert(!strcmp(b.rows[0].name,"Notes-4.txt"));
    assert(!strcmp(b.rows[5].name,"Notes-9.txt"));
    assert(!ink_browser_page(&b,1)); assert(ink_browser_page(&b,-1));
    /* Exercise routing through the first folder. */
    while(1) {
        for(unsigned i=0;i<b.count;++i) if(b.rows[i].directory) {
            assert(!ink_browser_long_press(&b,20,INK_BROWSER_LIST_Y+i*INK_BROWSER_ROW_HEIGHT));
            ink_browser_tap(&b,20,INK_BROWSER_LIST_Y+i*INK_BROWSER_ROW_HEIGHT); assert(b.count==4);
            unsigned n=row(&b,"book.epub"); ink_browser_tap(&b,20,INK_BROWSER_LIST_Y+n*INK_BROWSER_ROW_HEIGHT); assert(b.view==INK_OPEN_MARKDOWN);
            ink_browser_home(&b); n=row(&b,"book.md"); ink_browser_tap(&b,20,INK_BROWSER_LIST_Y+n*INK_BROWSER_ROW_HEIGHT); assert(b.view==INK_OPEN_MARKDOWN);
            ink_browser_home(&b); n=row(&b,"script.py"); ink_browser_tap(&b,20,INK_BROWSER_LIST_Y+n*INK_BROWSER_ROW_HEIGHT); assert(b.view==INK_OPEN_TEXT);
            ink_browser_home(&b); n=row(&b,"binary.bin"); ink_browser_tap(&b,20,INK_BROWSER_LIST_Y+n*INK_BROWSER_ROW_HEIGHT); assert(b.view==INK_NOTICE);
            ink_browser_home(&b); ink_browser_long_press(&b,20,INK_BROWSER_LIST_Y+n*INK_BROWSER_ROW_HEIGHT); assert(b.view==INK_FILE_MENU);
            ink_browser_tap(&b,60,200); assert(b.view==INK_NOTICE); /* binary Edit */
            ink_browser_home(&b); n=row(&b,"book.md"); ink_browser_long_press(&b,20,INK_BROWSER_LIST_Y+n*INK_BROWSER_ROW_HEIGHT);
            ink_browser_tap(&b,60,200); assert(b.view==INK_EDIT_TEXT); /* editor request */
            ink_browser_home(&b); ink_browser_long_press(&b,20,INK_BROWSER_LIST_Y+n*INK_BROWSER_ROW_HEIGHT);
            ink_browser_tap(&b,60,270); assert(b.view==INK_NOTICE && !strcmp(b.message,"not executable"));
            ink_browser_home(&b); n=row(&b,"script.py"); ink_browser_long_press(&b,20,INK_BROWSER_LIST_Y+n*INK_BROWSER_ROW_HEIGHT);
            ink_browser_tap(&b,60,270); assert(b.view==INK_EXECUTE_PYTHON);
            ink_browser_home(&b); n=row(&b,"script.py");ink_browser_long_press(&b,20,INK_BROWSER_LIST_Y+n*INK_BROWSER_ROW_HEIGHT);
            ink_browser_tap(&b,60,340);assert(b.view==INK_FILES&&b.count==3);
            for(unsigned j=0;j<b.count;j++)assert(strcmp(b.rows[j].name,"script.py"));
            ink_browser_root(&b); assert(!strcmp(b.folder,b.root)&&b.page==0&&b.view==INK_FILES);
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
    assert(!fclose(out)); puts("OK: paging, folders, Home, Markdown/text routing, file menu, Edit text/binary gate, Execute gate");
}
