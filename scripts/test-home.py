#!/usr/bin/env python3
"""Exercise the actual application's Home dispatch with lightweight owner stubs."""
from pathlib import Path
import subprocess
import tempfile
source = Path('main/browser_app.c').read_text()
home = source[source.index('static void home('):source.index('static bool dispatch(')]
prefix = r'''
#include <assert.h>
#include <stdbool.h>
static int reader, editor, python, browser;
static bool reader_active, editor_active, console_active, home_requested;
static struct { bool open; int view; } power_menu;
static unsigned readers, discards, closes, roots, ordinary;
static void ink_reader_close(int *p) {(void)p;readers++;}
static void ink_editor_discard(int *p) {(void)p;discards++;}
static void ink_python_worker_close(int *p) {(void)p;closes++;}
static void ink_browser_root(int *p) {(void)p;roots++;}
static void ink_reader_home(int *p) {(void)p;ordinary++;}
static void ink_python_worker_home(int *p,bool l) {(void)p;(void)l;ordinary++;}
static bool ink_editor_home(int *p,bool l) {(void)p;(void)l;ordinary++;return false;}
static void ink_browser_home(int *p) {(void)p;ordinary++;}
'''
tests = r'''
int main(void) {
    for(unsigned context=0;context<4;context++) for(unsigned overlay=0;overlay<3;overlay++) {
        reader_active=context==1; editor_active=context==2; console_active=context==3;
        power_menu.open=overlay!=0;power_menu.view=overlay==2;
        readers=discards=closes=roots=ordinary=0;home_requested=false;
        home(true);
        assert(!power_menu.open&&!power_menu.view&&!reader_active&&!editor_active);
        assert(readers==(context==1)&&discards==(context==2)&&closes==(context==3));
        assert(roots==(context!=3)&&home_requested==(context==3)&&!ordinary);
        /* Python ownership remains live until the existing cleanup acknowledgment. */
        assert(console_active==(context==3));
    }
    console_active=false;power_menu.open=true;power_menu.view=1;
    home(false);assert(power_menu.open&&!power_menu.view);
    home(false);assert(!power_menu.open);
}
'''
with tempfile.TemporaryDirectory() as directory:
    c = Path(directory)/'home.c'; exe=Path(directory)/'home'
    c.write_text(prefix+home+tests)
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',str(c),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS: global long Home across browser/reader/editor/Python and Power/font overlays; cleanup ownership preserved')
