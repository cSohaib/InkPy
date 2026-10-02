#include "md_budget.h"
#include "ink_layout.h"
#include <stdlib.h>
#include <stdint.h>
typedef union { max_align_t alignment; size_t bytes; } Header;
static size_t used, peak;
void *ink_md_malloc(size_t n)
{
    if (n > INK_MD_HEAP_BYTES-sizeof(Header) || used > INK_MD_HEAP_BYTES-n-sizeof(Header)) return NULL;
    Header *h=malloc(sizeof(*h)+n);
    if (!h) return NULL;
    h->bytes=sizeof(*h)+n; used+=h->bytes;
    if (used>peak) peak=used;
    return h+1;
}
void ink_md_free(void *p)
{
    if (!p) return;
    Header *h=(Header*)p-1; used-=h->bytes; free(h);
}
void *ink_md_realloc(void *p,size_t n)
{
    if (!p) return ink_md_malloc(n);
    if (!n) { ink_md_free(p); return NULL; }
    Header *old=(Header*)p-1;
    size_t previous=old->bytes;
    if (n>INK_MD_HEAP_BYTES-sizeof(Header) || used-previous>INK_MD_HEAP_BYTES-n-sizeof(Header)) return NULL;
    Header *h=realloc(old,sizeof(*h)+n);
    if (!h) return NULL;
    h->bytes=sizeof(*h)+n; used=used-previous+h->bytes;
    if (used>peak) peak=used;
    return h+1;
}
size_t ink_md_peak(void) { return peak; }
int ink_md_reset(void) { if (used) return -1; peak=0; return 0; }
