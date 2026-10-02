#pragma once
#include <stddef.h>
#include <stdint.h>
#include "../ink_browser/ink_keyboard.h"
enum { INK_CONSOLE_INPUT=4096, INK_CONSOLE_LINES=128, INK_CONSOLE_COLUMNS=24,
    INK_CONSOLE_PAGE_ROWS=7 };
/* Adapter serializes access; VM worker owns execution, UI owns key events. */
typedef struct {
    ink_keyboard keyboard;
    char input[INK_CONSOLE_INPUT];
    size_t used;
    char lines[INK_CONSOLE_LINES][INK_CONSOLE_COLUMNS+1];
    unsigned first,count,column,page;
    bool busy,more,full,menu;
} ink_console;
void ink_console_init(ink_console *c);
/* True on Enter: adapter copies input to its single-slot command queue. */
bool ink_console_key(ink_console *c,int key);
void ink_console_output(ink_console *c,const char *bytes,size_t length);
void ink_console_completed(ink_console *c,bool more);
void ink_console_page(ink_console *c,int direction);
enum { INK_CONSOLE_NONE, INK_CONSOLE_STOP, INK_CONSOLE_CLOSE };
int ink_console_home(ink_console *c,bool long_press);
int ink_console_tap(ink_console *c,unsigned x,unsigned y);
static inline const char *ink_console_line(const ink_console *c,unsigned row)
{
    unsigned end=c->count-c->page*INK_CONSOLE_PAGE_ROWS;
    unsigned start=end>INK_CONSOLE_PAGE_ROWS?end-INK_CONSOLE_PAGE_ROWS:0;
    return start+row<end?c->lines[(c->first+start+row)%INK_CONSOLE_LINES]:"";
}
void ink_console_draw(const ink_console *c,uint8_t frame[48000]);
