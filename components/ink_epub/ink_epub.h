#pragma once
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
typedef struct { char path[512],fragment[192],title[192]; uint32_t document,id; } ink_epub_chapter;
int ink_epub_prepare(const char *source,const char *cache,void (*progress)(void),char *error,size_t capacity);
int ink_epub_document(const char *source,const char *cache,unsigned document,const char *dest,
    void (*progress)(void),char *error,size_t capacity);
unsigned ink_epub_documents(const char *cache);
unsigned ink_epub_chapters(const char *cache);
int ink_epub_get_chapter(const char *cache,unsigned id,ink_epub_chapter *chapter);
int ink_epub_image(const char *source,const char *cache,const char *resource,char *out,size_t size,
    void (*progress)(void));
void ink_epub_cleanup(const char *cache);
int ink_epub_anchor(const char *cache,unsigned chapter,uint64_t *offset);
void ink_epub_cancel(void);
void ink_epub_reset_cancel(void);
bool ink_epub_cancelled(void);
