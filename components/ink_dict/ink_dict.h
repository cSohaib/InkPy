#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
enum { INK_DICT_ROWS=8,INK_DICT_LINES=16,INK_DICT_COLUMNS=24 };
typedef struct { char path[512],name[96]; } ink_dict_choice;
typedef struct {
    char root[512],cache[512],selected[512],name[96],types[64],error[160];
    FILE *idx,*ord,*data,*syn,*sord,*definition,*defpages;
    uint32_t words,synonyms;
    unsigned offset_bytes,first,count,total,page,pages;
    bool ready;
    void (*progress)(void);
    ink_dict_choice rows[INK_DICT_ROWS];
    char lines[INK_DICT_LINES][INK_DICT_COLUMNS*4+2];
} ink_dict;
void ink_dict_init(ink_dict *d,const char *root,void (*progress)(void));
void ink_dict_close(ink_dict *d);
int ink_dict_catalog(ink_dict *d,unsigned first);
int ink_dict_select(ink_dict *d,const char *ifo);
/* 0=found, 1=not found, -1=error; all outcomes provide popup text. */
int ink_dict_lookup(ink_dict *d,const char *word);
int ink_dict_page(ink_dict *d,int direction);
int ink_dict_gunzip(const char *source,const char *dest,void (*progress)(void));
