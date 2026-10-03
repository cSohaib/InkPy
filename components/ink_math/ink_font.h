#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
/* UI-task ownership. One selected face, fixed cell geometry, UTF-8 glyphs. */
int ink_font_init(void);
unsigned ink_font_count(void);
const char *ink_font_name(unsigned index);
int ink_font_select(unsigned index);
unsigned ink_font_selected(void);
void ink_font_text(uint8_t *frame,unsigned x,unsigned y,const char *utf8,
    unsigned cell,unsigned height,unsigned style,unsigned limit,
    void (*pixel)(uint8_t *,unsigned,unsigned));

void ink_font_scan(void);
