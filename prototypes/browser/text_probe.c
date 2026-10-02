#include "ink_browser.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(int argc,char **argv)
{
    assert(argc==3);
    ink_browser b; memset(&b,0,sizeof(b)); b.view=INK_OPEN_TEXT;
    snprintf(b.selected,sizeof(b.selected),"%s",argv[1]);
    assert(!ink_text_open(&b.text,b.selected)); assert(b.text.has_next);
    ink_text_view first=b.text;
    assert(!ink_browser_tap(&b,100,100)); /* Reader taps do not close the file. */
    assert(ink_browser_page(&b,1)); assert(b.text.page==2);
    assert(ink_browser_page(&b,-1)); assert(!memcmp(&first,&b.text,sizeof(first)));
    assert(!ink_browser_page(&b,-1));
    uint8_t frame[48000]; ink_browser_draw(&b,frame);
    FILE *out=fopen(argv[2],"wb"); assert(out); fputs("P4\n800 480\n",out);
    for(unsigned i=0;i<sizeof(frame);++i) fputc(frame[i]^255,out);
    assert(!fclose(out));
    while(b.text.has_next) assert(ink_browser_page(&b,1));
    assert(!ink_browser_page(&b,1));
    assert(ink_browser_home(&b) && b.view==INK_FILES);
    puts("OK: open text, next/previous, final-page boundary, tap ignored, Home back");
}
