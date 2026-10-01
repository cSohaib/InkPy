#include "input.h"
#include <assert.h>
#include <stdio.h>

static ink_press_t at(ink_button_t *b, bool down, uint32_t ms, bool dbl)
{
    return ink_button_update(b, down, ms, dbl);
}
int main(void)
{
    ink_button_t b = {0};
    /* Contact bounce must not become an action. */
    assert(at(&b, true, 10, true) == INK_PRESS_NONE);
    assert(at(&b, false, 20, true) == INK_PRESS_NONE);
    assert(at(&b, false, 60, true) == INK_PRESS_NONE);
    /* Power single is delayed, then fires once. */
    at(&b, true, 100, true); at(&b, true, 130, true);
    at(&b, false, 180, true); assert(at(&b, false, 210, true) == INK_PRESS_NONE);
    assert(at(&b, false, 509, true) == INK_PRESS_NONE);
    assert(at(&b, false, 510, true) == INK_PRESS_SHORT);
    assert(at(&b, false, 600, true) == INK_PRESS_NONE);
    /* Double press never emits its component short presses. */
    b = (ink_button_t){0};
    at(&b, true, 10, true); at(&b, true, 40, true);
    at(&b, false, 80, true); at(&b, false, 110, true);
    assert(at(&b, true, 180, true) == INK_PRESS_NONE);
    assert(at(&b, true, 210, true) == INK_PRESS_NONE);
    assert(at(&b, false, 250, true) == INK_PRESS_NONE);
    assert(at(&b, false, 280, true) == INK_PRESS_DOUBLE);
    assert(at(&b, false, 900, true) == INK_PRESS_NONE);
    /* Motionless capacitive Home hold fires without new frames; release is silent. */
    b = (ink_button_t){0};
    at(&b, true, 10, false); at(&b, true, 40, false);
    assert(at(&b, true, 839, false) == INK_PRESS_NONE);
    assert(at(&b, true, 840, false) == INK_PRESS_LONG);
    assert(at(&b, true, 900, false) == INK_PRESS_NONE);
    at(&b, false, 950, false); assert(at(&b, false, 980, false) == INK_PRESS_NONE);
    /* A first click followed by a hold means long, not double or delayed short. */
    b = (ink_button_t){0};
    at(&b, true, 10, true); at(&b, true, 40, true);
    at(&b, false, 80, true); at(&b, false, 110, true);
    at(&b, true, 180, true); at(&b, true, 210, true);
    assert(at(&b, true, 1010, true) == INK_PRESS_LONG);
    at(&b, false, 1050, true); assert(at(&b, false, 1080, true) == INK_PRESS_NONE);
    assert(at(&b, false, 1500, true) == INK_PRESS_NONE);
    /* Two separate singles outside the double window remain separate. */
    b = (ink_button_t){0};
    at(&b, true, 10, true); at(&b, true, 40, true);
    at(&b, false, 80, true); at(&b, false, 110, true);
    at(&b, true, 500, true); assert(at(&b, true, 530, true) == INK_PRESS_SHORT);
    at(&b, false, 560, true); at(&b, false, 590, true);
    assert(at(&b, false, 890, true) == INK_PRESS_SHORT);
    /* Long-press timing remains correct across the millisecond counter wrap. */
    b = (ink_button_t){0};
    at(&b, true, UINT32_MAX - 99, false); at(&b, true, UINT32_MAX - 69, false);
    assert(at(&b, true, 730, false) == INK_PRESS_LONG);
    puts("input timing: all cases passed");
}
