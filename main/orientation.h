#pragma once
#include <stdint.h>
#include "pins.h"

/* Host drawing uses the opposite portrait rotation to this physical unit.
 * Correct only the device boundary; reader/editor coordinates stay 480x800. */
static inline uint8_t ink_reverse_bits(uint8_t b)
{
    b=(uint8_t)(((b&0x55)<<1)|((b>>1)&0x55));
    b=(uint8_t)(((b&0x33)<<2)|((b>>2)&0x33));
    return (uint8_t)((b<<4)|(b>>4));
}
static inline void ink_frame_rotate_180(uint8_t frame[PANEL_BYTES])
{
    for(unsigned i=0;i<PANEL_BYTES/2;i++) {
        uint8_t a=frame[i],b=frame[PANEL_BYTES-1-i];
        frame[i]=ink_reverse_bits(b);
        frame[PANEL_BYTES-1-i]=ink_reverse_bits(a);
    }
}
static inline void ink_panel_to_ui(unsigned x,unsigned y,unsigned *ui_x,unsigned *ui_y)
{ *ui_x=PANEL_HEIGHT-1-y; *ui_y=x; }
