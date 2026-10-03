#include "ink_keyboard.h"
#include <string.h>
int ink_keyboard_key(const ink_keyboard *k,unsigned row,unsigned column)
{
    static const char *letters[]={"qwertyuiop","asdfghjkl:","zxcvbnm,._","'\"()[]{}\\"};
    static const char *symbols[]={"1234567890","!@#$%^&*()","+-=/\\[]{};","'\"<>?~`|"};
    static const int controls[]={INK_KEY_SHIFT,INK_KEY_SYMBOLS,' ',INK_KEY_DELETE,INK_KEY_ENTER};
    if(row==4) return column<5?controls[column]:0;
    if(row>=4) return 0;
    if(!k->symbols && row==3 && column==9) return INK_KEY_INDENT;
    const char *keys=k->symbols?symbols[row]:letters[row];
    if(column>=strlen(keys)) return 0;
    int key=(unsigned char)keys[column];
    if(k->shift && key>='a' && key<='z') key-=32;
    return key;
}
int ink_keyboard_tap(ink_keyboard *k,unsigned x,unsigned y)
{
    if(x>=INK_KB_X+10*INK_KB_WIDTH || y<INK_KB_Y || y>=INK_KB_Y+5*INK_KB_HEIGHT) return 0;
    unsigned row=(y-INK_KB_Y)/INK_KB_HEIGHT;
    unsigned col=(x-INK_KB_X)/(INK_KB_WIDTH*(row==4?2:1));
    int key=ink_keyboard_key(k,row,col);
    if(key==INK_KEY_SHIFT) { k->shift=!k->shift; return INK_KEY_CHANGED; }
    if(key==INK_KEY_SYMBOLS) { k->symbols=!k->symbols; return INK_KEY_CHANGED; }
    return key;
}
