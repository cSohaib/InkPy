#pragma once
#include <stdbool.h>
#include <stdint.h>
enum { INK_BIDI_CELLS=128 };
typedef struct {
    uint32_t visual[INK_BIDI_CELLS];
    int map[INK_BIDI_CELLS];
    bool rtl;
} ink_bidi;
bool ink_bidi_arabic(uint32_t cp);
bool ink_bidi_mark(uint32_t cp);
bool ink_bidi_hidden(uint32_t cp);
bool ink_bidi_word(uint32_t cp);
/* Fixed line budget; logical text is never modified. Caller owns result. */
int ink_bidi_shape(const uint32_t *logical,unsigned count,ink_bidi *out);
