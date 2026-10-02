#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef struct {
    bool open,on,night;
    unsigned brightness,warmth;
    char time[32],message[64];
} ink_power;
enum { INK_POWER_NONE,INK_POWER_LIGHT,INK_POWER_REFRESH };
/* Modal state belongs to the application, independent of the current screen. */
int ink_power_tap(ink_power *p,unsigned x,unsigned y);
void ink_power_draw(const ink_power *p,uint8_t frame[48000]);
