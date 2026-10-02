#pragma once
#include <stddef.h>
void *ink_md_malloc(size_t n);
void *ink_md_realloc(void *p, size_t n);
void ink_md_free(void *p);
size_t ink_md_peak(void);
int ink_md_reset(void);
