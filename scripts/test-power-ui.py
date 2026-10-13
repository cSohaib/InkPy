"""Check actual loading pixels/theme and device landscape draw/tap transforms."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
app=(root/'main/browser_app.c').read_text()
reader=(root/'components/ink_reader/ink_reader.c').read_text()
loading=app[app.index('static void loading_pixel('):app.index('static void loading_begin(')]
pixel=reader[reader.index('static void pixel('):reader.index('static void text(')]
tap=app[app.index('            x=799-event->x;'):app.index('            x=799-event->x;')+len('            x=799-event->x; y=479-event->y;')]
prefix=r'''
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "orientation.h"
#include "ink_icons.h"
static uint8_t a[48000],b[48000],*loading_frame=a;
static bool loading=true,loading_painted,restoring;
static struct {bool night;} power_menu;
static unsigned updates;
static int esp_timer_get_time(void){return 1000000;}
typedef struct {int kind,press;unsigned x,y;} ink_event;
enum {INK_EVENT_HOME,INK_PRESS_LONG};
static bool ink_capture_next(ink_event *e,int wait){(void)e;(void)wait;return false;}
static void ink_epub_cancel(void){}
static int ink_display_update(const uint8_t *f,bool full){(void)f;assert(!full);updates++;return 0;}
static void vTaskDelay(int n){(void)n;}
#define taskYIELD() ((void)0)
enum {INK_READER_PAGE};
static struct {unsigned view;bool landscape;} context={INK_READER_PAGE,true},*drawing=&context;
'''
suffix=r'''
int main(void){
    indexing_progress();assert(updates==1);memcpy(b,a,sizeof(a));
    indexing_progress();assert(updates==1);
    power_menu.night=true;loading_painted=false;indexing_progress();assert(updates==2);
    for(unsigned i=0;i<sizeof(a);i++)assert(a[i]==(uint8_t)(b[i]^255));
    restoring=true;loading_painted=false;indexing_progress();assert(updates==2);
    for(unsigned xx=0;xx<800;xx+=113)for(unsigned yy=0;yy<480;yy+=79){
        memset(a,255,sizeof(a));pixel(a,xx,yy);ink_frame_rotate_180(a);
        unsigned px=799-xx,py=479-yy;
        assert(!(a[py*100+px/8]&(0x80>>(px%8))));
        ink_event e={.x=px,.y=py},*event=&e;unsigned x,y;
'''+tap+r'''
        assert(x==xx&&y==yy);
    }
    puts("PASS: dark hourglass is exact inverse; restore never paints loading; anticlockwise pixels and physical taps agree");
}
'''
with tempfile.TemporaryDirectory() as tmp:
    source=Path(tmp)/'test.c';binary=Path(tmp)/'test'
    source.write_text(prefix+loading+pixel+suffix)
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-I'+str(root/'main'),'-I'+str(root/'components/ink_browser'),str(source),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
