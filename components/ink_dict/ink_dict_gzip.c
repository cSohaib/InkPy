#include "ink_dict.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GZIP_H
#include <limits.h>
#include <stdlib.h>
#include <string.h>
static void *alloc(FT_Memory m,long n) { (void)m; return n>0?malloc((size_t)n):NULL; }
static void release(FT_Memory m,void *p) { (void)m; free(p); }
static void *resize(FT_Memory m,long old,long n,void *p)
{ (void)m; (void)old; return n>0?realloc(p,(size_t)n):NULL; }
static unsigned long read_file(FT_Stream s,unsigned long offset,unsigned char *b,unsigned long n)
{
    FILE *f=s->descriptor.pointer;
    if(offset>LONG_MAX||fseek(f,(long)offset,SEEK_SET)) return n?0:1;
    return n?(unsigned long)fread(b,1,n,f):0;
}
static uint32_t little(const unsigned char *b)
{ return (uint32_t)b[0]|(uint32_t)b[1]<<8|(uint32_t)b[2]<<16|(uint32_t)b[3]<<24; }
int ink_dict_gunzip(const char *source,const char *dest,void (*progress)(void))
{
    FILE *in=fopen(source,"rb"),*out=NULL; int result=-1;
    struct FT_MemoryRec_ memory={NULL,alloc,release,resize};
    FT_StreamRec input={0},gzip={0}; unsigned char trailer[8],buffer[4096];
    if(!in||fseek(in,0,SEEK_END)) goto done;
    long length=ftell(in);
    if(length<18||fseek(in,length-8,SEEK_SET)||fread(trailer,1,8,in)!=8) goto done;
    input.size=(unsigned long)length; input.descriptor.pointer=in; input.read=read_file; input.memory=&memory;
    if(FT_Stream_OpenGzip(&gzip,&input)) goto done;
    out=fopen(dest,"wb"); if(!out) goto done;
    uint32_t crc=UINT32_MAX,table[256]; unsigned long offset=0;
    for(unsigned i=0;i<256;i++) { uint32_t value=i; for(unsigned j=0;j<8;j++) value=(value>>1)^((value&1)?0xedb88320u:0); table[i]=value; }
    for(;;) {
        unsigned long n;
        if(gzip.read) n=gzip.read(&gzip,offset,buffer,sizeof(buffer));
        else { n=offset<gzip.size?gzip.size-offset:0; if(n>sizeof(buffer)) n=sizeof(buffer); if(n) memcpy(buffer,gzip.base+offset,n); }
        if(!n) break;
        if(offset>(unsigned long)LONG_MAX-n||fwrite(buffer,1,n,out)!=n) goto done;
        for(unsigned long i=0;i<n;i++) crc=table[(crc^buffer[i])&255]^(crc>>8);
        offset+=n; if(progress) progress();
    }
    if(ferror(in)||offset!=little(trailer+4)||(crc^UINT32_MAX)!=little(trailer)) goto done;
    result=0;
done:
    if(gzip.close) gzip.close(&gzip);
    if(in) fclose(in);
    if(out&&fclose(out)) result=-1;
    if(result) remove(dest);
    return result;
}
