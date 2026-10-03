#pragma once
#include "ink_layout.h"
#include <stdbool.h>
#include "ink_dict.h"
enum { INK_READER_PAGE,INK_READER_MENU,INK_READER_GOTO,INK_READER_CHAPTERS,INK_READER_DEFINITION,INK_READER_DICTIONARIES };
typedef struct {
    FILE *draw,*pages,*chapters;
    ink_layout_stats stats;
    unsigned page,chapter,view,chapter_first;
    char cache[512],title[INK_TITLE_BYTES],digits[11],error[160];
    ink_dict *dictionary; /* Application-owned, retained across book sessions. */
    char word[256];
} ink_reader;
/* One owner/session; source is never written. Disposable indexes live on SD. */
int ink_reader_open(ink_reader *r,const char *source,const char *root,ink_layout_math math,void (*progress)(void));
void ink_reader_close(ink_reader *r);
bool ink_reader_page(ink_reader *r,int direction);
void ink_reader_home(ink_reader *r);
/* True requests Close. Dictionary selection exists only in the lookup popup. */
bool ink_reader_tap(ink_reader *r,unsigned x,unsigned y);
int ink_reader_draw(ink_reader *r,uint8_t frame[48000]);
