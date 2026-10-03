#pragma once
#include <stdint.h>
/* Fixed 32px line icons; no fonts, resources, theme, or language dependency. */
enum { ICON_ADD,ICON_CONSOLE,ICON_ERROR,ICON_SUN,ICON_WARM,ICON_LIGHT,ICON_MOON,
 ICON_ROTATE,ICON_CLOCK,ICON_FONT,ICON_REFRESH,ICON_CLOSE,ICON_SAVE,ICON_BACK,
 ICON_PLAY,ICON_EDIT,ICON_CHAPTER,ICON_PAGE,ICON_DICT,ICON_STOP,ICON_ENTER,
 ICON_DELETE,ICON_SHIFT,ICON_SPACE,ICON_TAB,ICON_BATTERY };
typedef void (*ink_icon_pixel)(uint8_t *,unsigned,unsigned);
static inline void ink_icon_line(uint8_t *f,int x,int y,int xx,int yy,ink_icon_pixel p)
{
 int dx=xx>x?xx-x:x-xx,sx=x<xx?1:-1,dy=yy>y?y-yy:yy-y,sy=y<yy?1:-1,e=dx+dy;
 for(;;) { if(x>=0&&y>=0)p(f,(unsigned)x,(unsigned)y); if(x==xx&&y==yy)break;
 int n=2*e;if(n>=dy){e+=dy;x+=sx;}if(n<=dx){e+=dx;y+=sy;} }
}
static inline void ink_icon(uint8_t *f,unsigned x,unsigned y,unsigned kind,ink_icon_pixel p)
{
#define L(a,b,c,d) ink_icon_line(f,(int)x+(a),(int)y+(b),(int)x+(c),(int)y+(d),p)
#define R(a,b,c,d) do { L(a,b,c,b);L(c,b,c,d);L(c,d,a,d);L(a,d,a,b); } while(0)
#define C() do { for(int i=-12;i<=12;i++) for(int j=-12;j<=12;j++) {int v=i*i+j*j;if(v>=121&&v<=156)p(f,x+16+i,y+16+j);} } while(0)
 switch(kind) {
 case ICON_ERROR:C();L(10,10,22,22);L(10,22,22,10);break;
 case ICON_CLOSE:L(7,7,25,25);L(7,25,25,7);break;
 case ICON_ADD:R(5,3,25,29);L(10,16,20,16);L(15,11,15,21);break;
 case ICON_CONSOLE:R(2,5,30,27);L(7,11,13,16);L(13,16,7,21);L(17,22,25,22);break;
 case ICON_SUN:C();L(16,0,16,3);L(16,29,16,31);L(0,16,3,16);L(29,16,31,16);break;
 case ICON_WARM:C();L(16,6,16,26);for(int i=18;i<=23;i++)L(i,10,i,22);break;
 case ICON_LIGHT:R(8,3,24,20);L(12,21,12,27);L(20,21,20,27);L(12,27,20,27);break;
 case ICON_MOON:L(20,3,9,8);L(9,8,5,18);L(5,18,12,28);L(12,28,25,25);L(25,25,28,18);L(28,18,18,21);L(18,21,13,12);L(13,12,20,3);break;
 case ICON_ROTATE:R(5,3,17,23);L(22,8,29,8);L(29,8,29,26);L(24,21,29,26);L(29,26,31,21);break;
 case ICON_CLOCK:C();L(16,8,16,16);L(16,16,23,20);break;
 case ICON_FONT:L(4,26,13,5);L(13,5,22,26);L(8,18,18,18);L(22,26,30,26);break;
 case ICON_REFRESH:L(5,15,9,5);L(9,5,24,5);L(24,5,29,14);L(24,13,29,14);L(29,14,30,9);L(27,19,23,27);L(23,27,8,27);L(8,27,3,18);L(3,18,2,23);L(3,18,8,19);break;
 case ICON_SAVE:R(5,3,27,29);R(10,3,21,12);R(10,20,23,29);break;
 case ICON_BACK:L(25,16,5,16);L(5,16,13,8);L(5,16,13,24);break;
 case ICON_PLAY:L(9,4,26,16);L(26,16,9,28);L(9,28,9,4);break;
 case ICON_STOP:R(7,7,25,25);break;
 case ICON_EDIT:L(5,26,9,18);L(9,18,24,3);L(24,3,29,8);L(29,8,14,23);L(14,23,5,26);break;
 case ICON_CHAPTER:R(4,3,28,29);L(10,3,10,29);L(15,9,24,9);L(15,15,24,15);L(15,21,24,21);break;
 case ICON_PAGE:R(6,3,26,29);L(11,10,21,10);L(11,16,21,16);L(11,22,21,22);break;
 case ICON_DICT:R(3,5,16,28);R(16,5,29,28);L(8,11,12,11);L(20,11,24,11);break;
 case ICON_ENTER:L(26,6,26,20);L(26,20,5,20);L(5,20,13,12);L(5,20,13,28);break;
 case ICON_DELETE:L(2,16,10,6);L(10,6,29,6);L(29,6,29,26);L(29,26,10,26);L(10,26,2,16);L(15,11,23,21);L(15,21,23,11);break;
 case ICON_SHIFT:L(3,15,16,3);L(16,3,29,15);L(29,15,22,15);L(22,15,22,28);L(22,28,10,28);L(10,28,10,15);L(10,15,3,15);break;
 case ICON_SPACE:L(4,14,4,23);L(4,23,28,23);L(28,23,28,14);break;
 case ICON_TAB:L(3,16,26,16);L(26,16,19,9);L(26,16,19,23);L(29,7,29,25);break;
 case ICON_BATTERY:R(2,9,27,24);R(27,13,30,20);break;
 }
#undef C
#undef R
#undef L
}
