#pragma once
#include <stdint.h>
/* Monochrome line icons in a 32-unit grid. Sizes are fixed at their call sites. */
enum { ICON_ADD,ICON_CONSOLE,ICON_ERROR,ICON_BULB,ICON_BULB_ON,ICON_WARM,ICON_CONTRAST,
 ICON_ROTATE,ICON_FONT,ICON_REFRESH,ICON_CLOSE,ICON_SAVE,ICON_DISCARD,ICON_BACK,
 ICON_PLAY,ICON_EDIT,ICON_CHAPTER,ICON_PAGE,ICON_DICT,ICON_STOP,ICON_ENTER,
 ICON_DELETE,ICON_SHIFT,ICON_SPACE,ICON_TAB,ICON_EXIT,ICON_FOLDER,ICON_FILE,ICON_TRASH,ICON_WAIT };
typedef void (*ink_icon_pixel)(uint8_t *,unsigned,unsigned);
static inline void ink_icon_line(uint8_t *f,int x,int y,int xx,int yy,unsigned stroke,ink_icon_pixel p)
{
 int dx=xx>x?xx-x:x-xx,sx=x<xx?1:-1,dy=yy>y?y-yy:yy-y,sy=y<yy?1:-1,e=dx+dy;
 for(;;) { for(unsigned i=0;i<stroke;i++)for(unsigned j=0;j<stroke;j++)
 if(x+(int)i>=0&&y+(int)j>=0)p(f,(unsigned)x+i,(unsigned)y+j);
 if(x==xx&&y==yy)break;
 int n=2*e;if(n>=dy){e+=dy;x+=sx;}if(n<=dx){e+=dx;y+=sy;} }
}
static inline void ink_icon_size(uint8_t *f,unsigned x,unsigned y,unsigned kind,unsigned size,ink_icon_pixel p)
{
 unsigned stroke=size/32;if(!stroke)stroke=1;
#define L(a,b,c,d) ink_icon_line(f,(int)x+(a)*(int)size/32,(int)y+(b)*(int)size/32,(int)x+(c)*(int)size/32,(int)y+(d)*(int)size/32,stroke,p)
#define R(a,b,c,d) do { L(a,b,c,b);L(c,b,c,d);L(c,d,a,d);L(a,d,a,b); } while(0)
#define C(radius,filled) do { int r=(radius)*(int)size/32,inner=r-(int)stroke; for(unsigned i=0;i<size;i++)for(unsigned j=0;j<size;j++) { int dx=(int)i-(int)size/2,dy=(int)j-(int)size/2,v=dx*dx+dy*dy; if(v<=r*r&&((filled)||v>=inner*inner))p(f,x+i,y+j); } } while(0)
 switch(kind) {
 case ICON_ERROR:C(16,0);L(9,9,23,23);L(9,23,23,9);break;
 case ICON_CLOSE:L(7,7,25,25);L(7,25,25,7);break;
 case ICON_ADD:L(4,16,28,16);L(16,4,16,28);break;
 case ICON_CONSOLE:L(3,7,13,16);L(13,16,3,25);L(17,26,29,26);break;
 case ICON_BULB_ON:L(16,0,16,3);L(1,12,4,12);L(28,12,31,12);L(3,2,6,5);L(26,5,29,2); /* fall through */
 case ICON_BULB:L(9,5,23,5);L(9,5,6,12);L(6,12,11,20);L(23,5,26,12);L(26,12,21,20);R(11,20,21,26);L(13,29,19,29);break;
 case ICON_WARM:R(13,2,19,23);L(16,7,16,24);R(10,23,22,29);L(22,5,26,5);L(22,11,26,11);L(22,17,26,17);break;
 case ICON_CONTRAST:C(13,0);for(unsigned i=0;i<size/2;i++)for(unsigned j=0;j<size;j++){int dx=(int)i-(int)size/2,dy=(int)j-(int)size/2,r=13*(int)size/32;if(dx*dx+dy*dy<=r*r)p(f,x+i,y+j);}break;
 case ICON_ROTATE:R(5,3,17,23);L(22,8,29,8);L(29,8,29,26);L(24,21,29,26);L(29,26,31,21);break;
 case ICON_FONT: {
  /* Aa glyphs from the bundled bitmap font; see FONT-LICENSE.txt. */
  static const uint16_t letters[2][16]={{0x0,0x0,0x0,0x10,0x28,0x28,0x28,0x28,0x44,0x7c,0x44,0x82,0x82,0x0,0x0,0x0},{0x0,0x0,0x0,0x0,0x0,0x38,0x44,0x40,0x7c,0x42,0x42,0x62,0x5c,0x0,0x0,0x0}};
  for(unsigned row=0;row<24;row++)for(unsigned col=0;col<13;col++) {
   if(letters[0][row*16/24]&(1u<<(col*9/13)))L(2+(int)col,4+(int)row,2+(int)col,4+(int)row);
   if(letters[1][row*16/24]&(1u<<(col*9/13)))L(17+(int)col,4+(int)row,17+(int)col,4+(int)row);
  }break; }
 case ICON_WAIT:L(7,3,25,3);L(7,29,25,29);L(8,4,8,9);L(24,4,24,9);L(8,9,24,23);L(24,9,8,23);L(8,23,8,28);L(24,23,24,28);L(12,26,20,26);break;
 case ICON_REFRESH:L(5,15,9,5);L(9,5,24,5);L(24,5,29,14);L(24,13,29,14);L(29,14,30,9);L(27,19,23,27);L(23,27,8,27);L(8,27,3,18);L(3,18,2,23);L(3,18,8,19);break;
 case ICON_DISCARD:L(2,2,30,30);L(2,30,30,2); /* fall through */
 case ICON_SAVE:R(5,3,27,29);R(10,3,21,12);R(10,20,23,29);break;
 case ICON_BACK:L(25,16,5,16);L(5,16,13,8);L(5,16,13,24);break;
 case ICON_PLAY:L(9,4,26,16);L(26,16,9,28);L(9,28,9,4);break;
 case ICON_STOP:R(7,7,25,25);break;
 case ICON_EDIT:L(5,26,9,18);L(9,18,24,3);L(24,3,29,8);L(29,8,14,23);L(14,23,5,26);break;
 case ICON_CHAPTER:L(2,5,16,9);L(16,9,30,5);L(2,5,2,25);L(30,5,30,25);L(2,25,16,29);L(16,29,30,25);L(16,9,16,29);break;
 case ICON_PAGE:R(6,2,26,30);L(13,10,11,24);L(20,10,18,24);L(9,14,23,14);L(8,20,22,20);break;
 case ICON_DICT:R(2,3,30,29);L(6,3,6,29);L(9,23,13,10);L(13,10,17,23);L(11,18,15,18);L(20,10,27,10);L(27,10,20,23);L(20,23,27,23);break;
 case ICON_ENTER:L(26,6,26,20);L(26,20,5,20);L(5,20,13,12);L(5,20,13,28);break;
 case ICON_DELETE:L(2,16,10,6);L(10,6,29,6);L(29,6,29,26);L(29,26,10,26);L(10,26,2,16);L(15,11,23,21);L(15,21,23,11);break;
 case ICON_SHIFT:L(3,15,16,3);L(16,3,29,15);L(29,15,22,15);L(22,15,22,28);L(22,28,10,28);L(10,28,10,15);L(10,15,3,15);break;
 case ICON_SPACE:L(4,14,4,23);L(4,23,28,23);L(28,23,28,14);break;
 case ICON_TAB:L(3,16,26,16);L(26,16,19,9);L(26,16,19,23);L(29,7,29,25);break;
 case ICON_EXIT:R(3,3,17,29);L(12,16,30,16);L(30,16,23,9);L(30,16,23,23);break;
 case ICON_FOLDER:L(2,9,2,27);L(2,27,30,27);L(30,27,30,9);L(30,9,15,9);L(15,9,12,5);L(12,5,2,5);L(2,5,2,9);break;
 case ICON_FILE:L(6,2,20,2);L(20,2,26,8);L(26,8,26,30);L(26,30,6,30);L(6,30,6,2);L(20,2,20,8);L(20,8,26,8);break;
 case ICON_TRASH:R(8,8,24,29);L(5,8,27,8);R(12,3,20,8);L(13,12,13,25);L(19,12,19,25);break;
 }
#undef C
#undef R
#undef L
}
static inline void ink_icon(uint8_t *f,unsigned x,unsigned y,unsigned kind,ink_icon_pixel p)
{
 if(kind==ICON_ERROR) ink_icon_size(f,x>=34?x-34:0,y>=34?y-34:0,kind,100,p);
 else ink_icon_size(f,x,y,kind,32,p);
}
