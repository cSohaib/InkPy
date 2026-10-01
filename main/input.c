#include "input.h"

ink_press_t ink_button_update(ink_button_t *b, bool pressed, uint32_t now, bool double_enabled)
{
    ink_press_t event = INK_PRESS_NONE;
    if (pressed != b->raw) {
        b->raw = pressed;
        b->raw_at = now;
    }
    /* Expire a first click even when a new press just started outside its window. */
    if (b->pending && !b->down && !b->raw && now - b->up_at >= INK_DOUBLE_MS) {
        b->pending = false;
        event = INK_PRESS_SHORT;
    }
    if (b->down != b->raw && now - b->raw_at >= INK_DEBOUNCE_MS) {
        b->down = b->raw;
        if (b->down) {
            if (b->pending && now - b->up_at >= INK_DOUBLE_MS) {
                b->pending = false;
                event = INK_PRESS_SHORT;
            }
            b->down_at = now;
            b->held = false;
        } else if (!b->held) {
            if (!double_enabled) event = INK_PRESS_SHORT;
            else if (b->pending) {
                b->pending = false;
                event = INK_PRESS_DOUBLE;
            } else {
                b->pending = true;
                b->up_at = now;
            }
        }
    }
    if (b->down && !b->held && now - b->down_at >= INK_HOLD_MS) {
        b->held = true;
        b->pending = false;
        event = INK_PRESS_LONG;
    }
    return event;
}
