#define _POSIX_C_SOURCE 200809L
#include "ink_editor.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static unsigned rng=19, renames, fail_rename;
static unsigned random_number(void) { rng=rng*1664525u+1013904223u; return rng; }
int __real_rename(const char *,const char *);
int __wrap_rename(const char *a,const char *b)
{ if(++renames==fail_rename) return -1; return __real_rename(a,b); }
static void put(const char *path,const void *data,size_t n)
{ FILE *f=fopen(path,"wb"); assert(f); assert(fwrite(data,1,n,f)==n); assert(!fclose(f)); }
static void same(const char *path,const void *data,size_t n)
{ FILE *f=fopen(path,"rb"); assert(f); unsigned char b[32768]; assert(n<sizeof(b)); assert(fread(b,1,sizeof(b),f)==n); assert(!memcmp(b,data,n)); assert(!fclose(f)); }
int main(void)
{
    char path[]="audit-XXXXXX"; int fd=mkstemp(path); assert(fd>=0); close(fd);
    ink_editor e; char expected[32768]; size_t n=10000; memset(expected,'a',n);
    put(path,expected,n); assert(!ink_editor_open(&e,path));
    for(unsigned step=0;step<6000;step++) {
        unsigned action=random_number()%6;
        if(action==0) assert(!ink_editor_page(&e,random_number()%2?1:-1));
        else if(action==1) ink_editor_cursor(&e,random_number()%e.rows,random_number()%(INK_EDITOR_COLUMNS+1));
        else {
            size_t at=e.gap_start; assert(at<=n);
            if(action==2) {
                assert(!ink_editor_key(&e,INK_KEY_DELETE));
                if(at) { memmove(expected+at-1,expected+at,n-at); n--; }
            } else {
                char c=(char)('a'+random_number()%26); assert(n+1<sizeof(expected));
                assert(!ink_editor_key(&e,c));
                memmove(expected+at+1,expected+at,n-at); expected[at]=c; n++;
            }
            assert(e.size==n);
        }
        assert(!e.error[0]);
    }
    assert(!ink_editor_save(&e)); same(path,expected,n);
    const char utf[]="\xef\xbb\xbf\xc3\xa9\r\n\xf0\x9f\x98\x80\tend\n";
    put(path,utf,sizeof(utf)-1); assert(!ink_editor_open(&e,path));
    assert(!ink_editor_save(&e)); same(path,utf,sizeof(utf)-1);
    const unsigned char invalid[][4]={{0xc0,0xaf,0,0},{0xed,0xa0,0x80,0},{0xf4,0x90,0x80,0x80},{0xe2,0x82,0,0},{'a',0,'b',0}};
    const size_t lengths[]={2,3,4,2,3};
    for(unsigned i=0;i<5;i++) {
        put(path,invalid[i],lengths[i]); assert(ink_editor_open(&e,path));
        assert(!e.work); same(path,invalid[i],lengths[i]);
    }
#ifdef ESP_PLATFORM
    put(path,"original",8); assert(!ink_editor_open(&e,path)); assert(!ink_editor_key(&e,'X'));
    renames=0; fail_rename=2; assert(ink_editor_save(&e)); same(path,"original",8);
    assert(e.work); renames=0; fail_rename=0; assert(!ink_editor_save(&e)); same(path,"Xoriginal",9);
#endif
    assert(!unlink(path));
    puts("PASS: 6000 deterministic edit/navigation operations, exact save, UTF-8 preservation, invalid-text rejection; ESP branch also tests failed replacement rollback");
}
