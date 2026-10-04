#pragma once
#include <stdio.h>
/* Converts supported XHTML to a disposable Markdown stream and chapter anchors.
 * Archives are read, never modified. Scratch files are confined to owned cache. */
int ink_epub_import(const char *source,const char *cache,void (*progress)(void),char *error,size_t capacity);
void ink_epub_cleanup(const char *cache);
