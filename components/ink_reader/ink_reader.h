#pragma once
#include "ink_layout.h"
#include <stdbool.h>
#include "ink_dict.h"
enum { INK_READER_PAGE,INK_READER_MENU,INK_READER_GOTO,INK_READER_CHAPTERS,INK_READER_DEFINITION,INK_READER_DICTIONARIES };
typedef struct {
    FILE *draw,*pages,*chapters;
    FILE *input,*anchors,*bitmap,*table;
    void *layout;
    ink_layout_stats local_stats;
    char book_cache[512],source[512];
    unsigned document,documents;
    bool epub,eof;
    ink_layout_math math;
    void (*progress)(void);
    ink_layout_stats stats;
    unsigned page,chapter,view,chapter_first;
    char cache[512],title[INK_TITLE_BYTES],digits[11],error[160];
    ink_dict *dictionary; /* Application-owned, retained across book sessions. */
    char word[256];
    bool landscape; /* Book pages only; every menu stays portrait. */
} ink_reader;
/* One owner/session; source is never written. Disposable indexes live on SD. */
int ink_reader_open(ink_reader *r,const char *source,const char *root,ink_layout_math math,void (*progress)(void));
void ink_reader_close(ink_reader *r);
bool ink_reader_page(ink_reader *r,int direction);
void ink_reader_home(ink_reader *r);
enum { INK_READER_STAY,INK_READER_CLOSE,INK_READER_ROTATE };
int ink_reader_rotate(ink_reader *r,const char *source,const char *root,ink_layout_math math,void (*progress)(void));
/* Dictionary selection exists only in the lookup popup. */
int ink_reader_tap(ink_reader *r,unsigned x,unsigned y);
int ink_reader_draw(ink_reader *r,uint8_t frame[48000]);
