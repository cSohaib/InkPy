#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef struct {
    bool open,on,night;
    unsigned brightness,warmth;
    char time[32],battery[48],message[64];
    unsigned view,font_first,font_count,font_choice;
    char font_names[6][96];
    int calendar[5]; /* year, month, day, hour, minute */
} ink_power;
enum { INK_POWER_NONE,INK_POWER_LIGHT,INK_POWER_REFRESH,
    INK_POWER_FONTS,INK_POWER_FONT_SELECT,INK_POWER_TIME,INK_POWER_TIME_SAVE };
void ink_power_page(ink_power *p,int direction);
/* Modal state belongs to the application, independent of the current screen. */
int ink_power_tap(ink_power *p,unsigned x,unsigned y);
void ink_power_draw(const ink_power *p,uint8_t frame[48000]);
