#define _POSIX_C_SOURCE 200809L
#include "ink_power.h"
#include "ink_editor.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
static void capture(const char *path,const uint8_t *frame)
{
    FILE *f=fopen(path,"wb"); assert(f); fputs("P4\n800 480\n",f);
    for(unsigned i=0;i<48000;i++) fputc(frame[i]^255,f);
    assert(!fclose(f));
}
int main(void)
{
    ink_power p={.open=true,.brightness=20,.warmth=50};
    assert(ink_power_tap(&p,400,150)==INK_POWER_LIGHT && p.brightness==25);
    assert(ink_power_tap(&p,320,214)==INK_POWER_LIGHT && p.warmth==45);
    assert(ink_power_tap(&p,60,280)==INK_POWER_LIGHT && p.on);
    ink_power_tap(&p,60,344); assert(p.night);
    uint8_t frame[48000]; ink_power_draw(&p,frame); capture("build/power.pbm",frame);
    assert(ink_power_tap(&p,60,600)==INK_POWER_REFRESH && !p.open);
    char path[]="build/ui-editor-XXXXXX"; int fd=mkstemp(path); assert(fd>=0); close(fd);
    ink_editor e; assert(!ink_editor_open(&e,path));
    ink_editor_tap(&e,18,522); assert(e.size==1); /* bottom keyboard q */
    ink_editor_draw(&e,frame); capture("build/editor-clean.pbm",frame);
    assert(ink_editor_home(&e,false)==false && e.menu);
    ink_editor_draw(&e,frame); capture("build/editor-menu.pbm",frame);
    ink_editor_discard(&e); unlink(path);
    puts("PASS: Power +/-/light/night/refresh model; bottom editor keyboard tap; clean editor and filename menu renders");
}
