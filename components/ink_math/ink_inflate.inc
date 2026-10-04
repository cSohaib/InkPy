/* InkPy adapter appended to pinned ftgzip.c; no changes to zlib internals. */
#include <stdint.h>
#include <stdlib.h>
static voidpf ink_zalloc(voidpf opaque,uInt n,uInt size)
{ (void)opaque; return calloc(n,size); }
static void ink_zfree(voidpf opaque,voidpf p) { (void)opaque;free(p); }
void *ink_inflate_open(int wrapped)
{
    z_stream *s=calloc(1,sizeof(*s));if(!s)return NULL;
    s->zalloc=ink_zalloc;s->zfree=ink_zfree;
    if(inflateInit2(s,wrapped?MAX_WBITS:-MAX_WBITS)!=Z_OK) { free(s);return NULL; }
    return s;
}
int ink_inflate_step(void *state,const void *input,size_t *input_size,void *output,size_t *output_size)
{
    z_stream *s=state;s->next_in=(Bytef*)input;s->avail_in=(uInt)*input_size;
    s->next_out=output;s->avail_out=(uInt)*output_size;
    int r=inflate(s,Z_NO_FLUSH);
    *input_size-=s->avail_in;*output_size-=s->avail_out;
    return r==Z_STREAM_END?1:r==Z_OK||r==Z_BUF_ERROR?0:-1;
}
void ink_inflate_close(void *state)
{ if(state) { inflateEnd(state);free(state); } }
uint32_t ink_crc32(uint32_t crc,const void *data,size_t size)
{ return (uint32_t)crc32(crc,data,(uInt)size); }
