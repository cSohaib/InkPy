#define _POSIX_C_SOURCE 200809L
#include "ink_editor.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
int main(void)
{
    char path[]="build/editor-large-XXXXXX"; int fd=mkstemp(path); assert(fd>=0);
    FILE *file=fdopen(fd,"wb"); assert(file);
    char block[4096]; memset(block,'a',sizeof(block));
    for(unsigned i=0;i<2048;i++) assert(fwrite(block,1,sizeof(block),file)==sizeof(block));
    assert(!fclose(file));
    ink_editor e; assert(!ink_editor_open(&e,path)); assert(e.size==8*1024*1024);
    assert(!ink_editor_key(&e,'X')); assert(!ink_editor_save(&e));
    file=fopen(path,"rb"); assert(file && fgetc(file)=='X');
    assert(!fseeko(file,-1,SEEK_END) && fgetc(file)=='a');
    assert(ftello(file)==8*1024*1024+1); fclose(file); unlink(path);
    char small[]="build/editor-small-XXXXXX"; fd=mkstemp(small); assert(fd>=0); close(fd);
    assert(!ink_editor_open(&e,small));
    for(unsigned i=0;i<4097;i++) assert(!ink_editor_key(&e,'z')); /* gap expansion */
    assert(e.size==4097);
    while(e.offset) assert(!ink_editor_page(&e,-1));
    ink_editor_cursor(&e,0,0); assert(!ink_editor_key(&e,INK_KEY_INDENT));
    assert(!ink_editor_save(&e));
    file=fopen(small,"rb"); assert(file); char first[5]={0}; assert(fread(first,1,4,file)==4);
    assert(!strcmp(first,"    ")); fclose(file); unlink(small);
    puts("OK: 8 MiB edit preserves tail, fixed-size editor, gap growth, pages, four-space Tab");
    return 0;
}
