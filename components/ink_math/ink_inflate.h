#pragma once
#include <stddef.h>
#include <stdint.h>
/* Streaming adapter to the zlib inflater already compiled inside FreeType. */
void *ink_inflate_open(int wrapped);
int ink_inflate_step(void *state,const void *input,size_t *input_size,void *output,size_t *output_size);
void ink_inflate_close(void *state);
uint32_t ink_crc32(uint32_t crc,const void *data,size_t size);
