#pragma once
#include <stdint.h>
/* Packed black=1 bitmap, stride ceil(width/8); <=48000 output bytes. */
int ink_image_render(const char *path,unsigned max_width,unsigned max_height,uint8_t *bits,unsigned *width,unsigned *height,void (*progress)(void));
