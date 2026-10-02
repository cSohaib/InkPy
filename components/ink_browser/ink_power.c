#include "ink_power.h"
#include <stdio.h>
int ink_power_tap(ink_power *p,unsigned x,unsigned y)
{
    if(x<32 || x>=448 || y<128 || y>=704) return INK_POWER_NONE;
    unsigned row=(y-128)/64; p->message[0]=0;
    if(row<2) {
        if(x<288) return INK_POWER_NONE;
        unsigned *value=row==0?&p->brightness:&p->warmth;
        if(x<368) *value=*value>=5?*value-5:0;
        else *value=*value<=95?*value+5:100;
        return INK_POWER_LIGHT;
    }
    if(row==2) { p->on=!p->on; return INK_POWER_LIGHT; }
    if(row==3) { p->night=!p->night; return INK_POWER_NONE; }
    if(row==7) { p->open=false; return INK_POWER_REFRESH; }
    if(row==8) p->open=false;
    else snprintf(p->message,sizeof(p->message),"%s: pending",row==4?"Orientation":row==5?"Time settings":"Font selector");
    return INK_POWER_NONE;
}
