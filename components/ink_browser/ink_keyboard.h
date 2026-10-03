#pragma once
#include <stdbool.h>
enum { INK_KB_X=0, INK_KB_Y=400, INK_KB_WIDTH=48, INK_KB_HEIGHT=56 };
enum { INK_KEY_SHIFT=256, INK_KEY_SYMBOLS, INK_KEY_DELETE, INK_KEY_ENTER, INK_KEY_CHANGED, INK_KEY_INDENT };
typedef struct { bool shift,symbols; } ink_keyboard;
/* ASCII character or control code, zero for unused cells. One fixed English layout. */
int ink_keyboard_key(const ink_keyboard *k,unsigned row,unsigned column);
int ink_keyboard_tap(ink_keyboard *k,unsigned x,unsigned y);
