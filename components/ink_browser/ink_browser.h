#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "ink_text.h"
#include "ink_keyboard.h"
enum { INK_BROWSER_LIST_Y=32, INK_BROWSER_ROW_HEIGHT=44, INK_BROWSER_ACTION_Y=712, INK_BROWSER_ROWS=15, INK_BROWSER_PATH=512 };
typedef enum { INK_FILES, INK_OPEN_MARKDOWN, INK_OPEN_TEXT, INK_NEW_FILE, INK_NOTICE,
    INK_FILE_MENU, INK_OPEN_CONSOLE, INK_EXECUTE_PYTHON, INK_EDIT_TEXT } ink_browser_view;
typedef struct { char name[256]; bool directory; } ink_browser_entry;
typedef struct {
    char root[INK_BROWSER_PATH], folder[INK_BROWSER_PATH], selected[INK_BROWSER_PATH];
    char message[96];
    ink_browser_entry rows[INK_BROWSER_ROWS];
    unsigned page, count;
    bool has_next;
    ink_browser_view view;
    ink_text_view text;
    ink_keyboard keyboard;
    char new_name[256];
} ink_browser;
int ink_browser_init(ink_browser *b,const char *root);
int ink_browser_reload(ink_browser *b);
bool ink_browser_page(ink_browser *b,int direction);
bool ink_browser_home(ink_browser *b);
void ink_browser_root(ink_browser *b);
bool ink_browser_tap(ink_browser *b,unsigned portrait_x,unsigned portrait_y);
bool ink_browser_long_press(ink_browser *b,unsigned portrait_x,unsigned portrait_y);
/* Native panel: 800x480 landscape, white=1. UI is rotated 480x800 portrait. */
void ink_browser_draw(const ink_browser *b,uint8_t frame[48000]);
