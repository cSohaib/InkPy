#include "ink_browser.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
static void key(ink_browser *b,int character)
{
    for(unsigned layer=0;layer<2;++layer) {
        for(unsigned row=0;row<4;++row) for(unsigned col=0;col<10;++col)
            if(ink_keyboard_key(&b->keyboard,row,col)==character) {
                assert(ink_browser_tap(b,INK_KB_X+col*INK_KB_WIDTH+INK_KB_WIDTH/2,INK_KB_Y+row*56+28)); return;
            }
        ink_browser_tap(b,INK_KB_X+3*INK_KB_WIDTH,INK_KB_Y+4*56+28);
    }
    assert(!"character missing from keyboard");
}
static void type(ink_browser *b,const char *s)
{ for(;*s;++s) key(b,(unsigned char)*s); }
int main(int argc,char **argv)
{
    assert(argc==3);
    ink_browser b; assert(!ink_browser_init(&b,argv[1]));
    assert(ink_keyboard_key(&b.keyboard,3,0)=='\'');
    assert(ink_keyboard_key(&b.keyboard,3,2)=='(');
    assert(ink_keyboard_key(&b.keyboard,3,4)=='[');
    assert(ink_keyboard_key(&b.keyboard,3,9)==INK_KEY_INDENT);
    const char *names[]={"notes.py","chapter.md","plain.txt","scratch"};
    for(unsigned i=0;i<4;++i) {
        ink_browser_tap(&b,50,INK_BROWSER_ACTION_Y+20); assert(b.view==INK_NEW_FILE);
        type(&b,names[i]); assert(!strcmp(b.new_name,names[i]));
        ink_browser_tap(&b,350,745); assert(b.view==INK_EDIT_TEXT);
        struct stat st; assert(!stat(b.selected,&st) && st.st_size==0);
        ink_browser_home(&b); assert(b.count==i+1);
    }
    b.view=INK_OPEN_CONSOLE;
    char path[600]; snprintf(path,sizeof(path),"%s/from-python.py",b.folder);
    FILE *created=fopen(path,"wb"); assert(created); assert(!fclose(created));
    ink_browser_home(&b); assert(b.count==5);
    FILE *f=fopen(b.selected,"wb"); assert(f); fputs("keep me",f); assert(!fclose(f));
    ink_browser_tap(&b,50,INK_BROWSER_ACTION_Y+20); type(&b,"scratch"); ink_browser_tap(&b,350,745);
    assert(b.view==INK_NEW_FILE && !strcmp(b.message,"Already exists"));
    f=fopen(b.selected,"rb"); assert(f); char saved[8]={0}; assert(fread(saved,1,7,f)==7); fclose(f);
    assert(!strcmp(saved,"keep me"));
    ink_browser_home(&b); ink_browser_tap(&b,50,INK_BROWSER_ACTION_Y+20); type(&b,"../bad");
    ink_browser_tap(&b,350,745); assert(b.view==INK_NEW_FILE && !strcmp(b.message,"Invalid filename"));
    ink_browser_home(&b); ink_browser_tap(&b,50,INK_BROWSER_ACTION_Y+20);
    ink_browser_tap(&b,INK_KB_X+INK_KB_WIDTH,INK_KB_Y+4*56+28); /* Shift */
    type(&b,"P");
    ink_browser_tap(&b,INK_KB_X+INK_KB_WIDTH,INK_KB_Y+4*56+28);
    type(&b,"ython.pyx");
    ink_browser_tap(&b,INK_KB_X+7*INK_KB_WIDTH,INK_KB_Y+4*56+28); /* Delete */
    assert(!strcmp(b.new_name,"Python.py"));
    uint8_t frame[48000]; ink_browser_draw(&b,frame);
    f=fopen(argv[2],"wb"); assert(f); fputs("P4\n800 480\n",f);
    for(unsigned i=0;i<sizeof(frame);++i) fputc(frame[i]^255,f);
    assert(!fclose(f));
    ink_browser_tap(&b,80,745); assert(b.view==INK_FILES);
    puts("OK: keyboard, exact extensions, extensionless file, no overwrite, invalid name, cancel");
}
