#include "ink_power.h"
int ink_power_tap(ink_power *p,unsigned x,unsigned y)
{
    if(x<8||x>=472) return INK_POWER_NONE;
    p->message[0]=0;
    if(y>=96&&y<288) {
        unsigned *value=y<192?&p->brightness:&p->warmth;
        if(x<104) *value=*value>=5?*value-5:0;
        else if(x>=376) *value=*value<=95?*value+5:100;
        else if(y<192) p->on=!p->on;
        else return INK_POWER_NONE;
        return INK_POWER_LIGHT;
    }
    if(y>=304&&y<456) {
        unsigned col=(x-8)/155;
        if(col==0) { p->open=false;return INK_POWER_DICTIONARY; }
        if(col==1) { p->open=false;return INK_POWER_REFRESH; }
        p->night=!p->night;
    }
    return INK_POWER_NONE;
}
