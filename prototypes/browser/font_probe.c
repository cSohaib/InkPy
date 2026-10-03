#include "ink_font.h"
#include "ink_view.h"
#include "ink_power.h"
#include "ink_console.h"
#include "ink_browser.h"
#include "ink_editor.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t frame[48000];
static void capture(const char *name) { FILE *f=fopen(name,"wb"); assert(f); assert(fwrite(frame,1,sizeof(frame),f)==sizeof(frame)); assert(!fclose(f)); }
int main(void)
{
    assert(!ink_font_init()); assert(ink_font_count()>=1); assert(ink_font_select(99));
    ink_power p={.open=true,.brightness=20,.warmth=50};
    strcpy(p.time,"2026-10-03 12:34"); strcpy(p.battery,"75%");
    ink_power_draw(&p,frame); capture("power-font.bin");
    assert(ink_power_tap(&p,48,400)==INK_POWER_ORIENTATION&&p.landscape);
    ink_view_landscape=true; ink_power_draw(&p,frame); capture("power-landscape.bin");
    assert(ink_power_tap(&p,48,460)==INK_POWER_TIME&&p.view==2);
    int date[]={2026,10,3,12,34}; memcpy(p.calendar,date,sizeof(date));
    ink_power_draw(&p,frame); capture("time-font.bin");
    ink_power_tap(&p,400,150); assert(p.calendar[0]==2027);
    assert(ink_power_tap(&p,48,540)==INK_POWER_TIME_SAVE);
    ink_console c; ink_console_init(&c);
    ink_console_output(&c,"caf\xc3",4); ink_console_output(&c,"\xa9 \xce\xb1\n\xd0\xbf\xd1\x80\xd0\xb8\xd0\xb2\xd0\xb5\xd1\x82",17);
    assert(!strncmp(c.lines[0],"café α",8));
    ink_view_landscape=false; ink_console_draw(&c,frame); capture("unicode-console.bin");
    ink_browser b={.view=INK_FILES,.count=3};
    strcpy(b.rows[0].name,"Notes.md"); strcpy(b.rows[1].name,"calculate.py"); strcpy(b.rows[2].name,"Books"); b.rows[2].directory=true;
    ink_browser_draw(&b,frame); capture("home-icons.bin");
    b.view=INK_NOTICE; strcpy(b.message,"Binary file or read error"); ink_browser_draw(&b,frame); capture("error-icon.bin");
    ink_editor e={.rows=2}; strcpy(e.lines[0],"from math import sqrt"); strcpy(e.lines[1],"print(sqrt(16))");
    ink_editor_draw(&e,frame); capture("editor-icons.bin");
    puts("PASS: bundled Unicode face, selection validation, Power submenus/orientation, split UTF-8 console output");
}
