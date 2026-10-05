#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef struct {
    bool open,on,night;
    unsigned brightness,warmth;
    char time[32],battery[8],message[64];
} ink_power;
enum { INK_POWER_NONE,INK_POWER_LIGHT,INK_POWER_REFRESH,INK_POWER_DICTIONARY };
int ink_power_tap(ink_power *p,unsigned x,unsigned y);
void ink_power_draw(const ink_power *p,uint8_t frame[48000]);
