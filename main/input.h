#pragma once
#include <stdbool.h>
#include <stdint.h>

enum { INK_DEBOUNCE_MS = 30, INK_DOUBLE_MS = 300, INK_HOLD_MS = 800 };
typedef enum { INK_PRESS_NONE, INK_PRESS_SHORT, INK_PRESS_DOUBLE, INK_PRESS_LONG } ink_press_t;
typedef struct {
    bool raw, down, held, pending;
    uint32_t raw_at, down_at, up_at;
} ink_button_t;

/* Debounced monotonic milliseconds. Unsigned differences tolerate wraparound.
 * A double-enabled button defers SHORT until its double-click window expires. */
ink_press_t ink_button_update(ink_button_t *b, bool pressed, uint32_t now, bool double_enabled);
