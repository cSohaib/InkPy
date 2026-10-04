#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef struct {
    bool open,on,night;
    unsigned brightness,warmth;
    char time[32],battery[8],message[64];
    unsigned view,font_first,font_count,font_choice;
    char font_names[8][96];
} ink_power;
enum { INK_POWER_NONE,INK_POWER_LIGHT,INK_POWER_REFRESH,INK_POWER_FONTS,INK_POWER_FONT_SELECT };
void ink_power_page(ink_power *p,int direction);
int ink_power_tap(ink_power *p,unsigned x,unsigned y);
void ink_power_draw(const ink_power *p,uint8_t frame[48000]);
