#include "ink_power.h"
#include <stdio.h>
void ink_power_page(ink_power *p,int direction)
{
    if(p->view!=1) return;
    if(direction>0&&p->font_first+6<p->font_count) p->font_first+=6;
    else if(direction<0&&p->font_first>=6) p->font_first-=6;
}
int ink_power_tap(ink_power *p,unsigned x,unsigned y)
{
    if(p->view==1) {
        if(y>=128&&y<512&&x>=32&&x<448) {
            unsigned i=p->font_first+(y-128)/64;
            if(i<p->font_count) { p->font_choice=i; return INK_POWER_FONT_SELECT; }
        }
        if(y>=576&&y<640) p->view=0;
        return INK_POWER_NONE;
    }
    if(p->view==2) {
        if(y>=128&&y<448&&x>=288&&x<448) {
            unsigned field=(y-128)/64; const int min[]={2000,1,1,0,0},max[]={2099,12,31,23,59};
            int v=p->calendar[field]+(x<368?-1:1);
            p->calendar[field]=v<min[field]?max[field]:v>max[field]?min[field]:v;
        }
        if(y>=512&&y<576) return INK_POWER_TIME_SAVE;
        if(y>=576&&y<640) p->view=0;
        return INK_POWER_NONE;
    }
    if(x<32 || x>=448 || y<128 || y>=640) return INK_POWER_NONE;
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
    if(row==4) { p->view=2; return INK_POWER_TIME; }
    if(row==5) { p->view=1; p->font_first=0; return INK_POWER_FONTS; }
    if(row==6) { p->open=false; return INK_POWER_REFRESH; }
    if(row==7) p->open=false;
    return INK_POWER_NONE;
}
