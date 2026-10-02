#pragma once
#include "ink_keyboard.h"
#include <stdint.h>
#include <stdio.h>
enum { INK_EDITOR_ROWS=9, INK_EDITOR_COLUMNS=24, INK_EDITOR_PATH=544 };
typedef struct {
    FILE *work;
    char path[512],temporary[INK_EDITOR_PATH],error[80];
    uint64_t size,gap_start,gap_end,offset,next;
    uint64_t cells[INK_EDITOR_ROWS][INK_EDITOR_COLUMNS+1];
    char lines[INK_EDITOR_ROWS][INK_EDITOR_COLUMNS+1];
    unsigned rows,cursor_row,cursor_column;
    bool menu,dirty,has_next;
    ink_keyboard keyboard;
} ink_editor;
int ink_editor_open(ink_editor *e,const char *path);
int ink_editor_key(ink_editor *e,int key);
int ink_editor_page(ink_editor *e,int direction);
void ink_editor_cursor(ink_editor *e,unsigned row,unsigned column);
/* Save closes only on success; failure leaves the working copy editable. */
int ink_editor_save(ink_editor *e);
void ink_editor_discard(ink_editor *e);
void ink_editor_draw(const ink_editor *e,uint8_t frame[48000]);
