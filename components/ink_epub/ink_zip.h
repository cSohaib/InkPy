#pragma once
#include <stdio.h>
#include <stdint.h>
typedef struct { FILE *file;uint32_t central,size;unsigned entries;void (*progress)(void); } ink_zip;
int ink_zip_open(ink_zip *z,const char *path,void (*progress)(void));
int ink_zip_extract(ink_zip *z,const char *name,FILE *out);
