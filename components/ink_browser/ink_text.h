#pragma once
#include "ink_ui.h"
#include <stdbool.h>
#include <stdint.h>
enum { INK_TEXT_ROWS=24, INK_TEXT_COLUMNS=INK_UI_COLUMNS };
typedef struct {
    char lines[INK_TEXT_ROWS][INK_TEXT_COLUMNS*4+1];
    uint64_t offset,next_offset;
    unsigned rows,page;
    bool has_next;
    char error[80];
} ink_text_view;
int ink_text_open(ink_text_view *v,const char *path);
/* 1 = changed, 0 = boundary, -1 = read/text error. Previous rescans from start. */
int ink_text_turn(ink_text_view *v,const char *path,int direction);
