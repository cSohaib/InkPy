#pragma once
#include <stdbool.h>
#include <stdint.h>
extern bool ink_view_landscape;
/* Packed native frame before the physical-device 180-degree correction. */
static inline void ink_view_pixel(uint8_t *f,unsigned x,unsigned y,unsigned width,unsigned height)
{
    if(x>=width||y>=height) return;
    if(!ink_view_landscape) {
        unsigned dx=799-y,dy=x; f[dy*100+dx/8]&=(uint8_t)~(0x80>>(dx%8)); return;
    }
    unsigned x0=x*800/width,x1=(x+1)*800/width,y0=y*480/height,y1=(y+1)*480/height;
    if(x1==x0) x1++;
    if(y1==y0) y1++;
    for(unsigned py=y0;py<y1&&py<480;py++) for(unsigned px=x0;px<x1&&px<800;px++) {
        unsigned dx=799-px,dy=479-py; f[dy*100+dx/8]&=(uint8_t)~(0x80>>(dx%8));
    }
}
