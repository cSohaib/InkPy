#define _POSIX_C_SOURCE 200809L
#include "ink_text.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

static int error(ink_text_view *v,const char *message)
{ snprintf(v->error,sizeof(v->error),"%s",message); return -1; }
/* Validate one UTF-8 codepoint. The interim ASCII font uses '?' for others. */
static int codepoint(FILE *f)
{
    int c=fgetc(f); if(c==EOF || c<128) return c;
    unsigned n,min,value;
    if(c>=0xc2 && c<=0xdf) { n=1; min=0x80; value=c&31; }
    else if(c>=0xe0 && c<=0xef) { n=2; min=0x800; value=c&15; }
    else if(c>=0xf0 && c<=0xf4) { n=3; min=0x10000; value=c&7; }
    else return -2;
    for(unsigned i=0;i<n;++i) {
        c=fgetc(f); if(c==EOF || (c&0xc0)!=0x80) return -2;
        value=(value<<6)|(c&63);
    }
    if(value<min || value>0x10ffff || (value>=0xd800 && value<=0xdfff)) return -2;
    return '?';
}
static void after_cr(FILE *f)
{ int c=fgetc(f); if(c!=EOF && c!='\n') ungetc(c,f); }
static int load(ink_text_view *v,const char *path,uint64_t offset,unsigned page)
{
    memset(v,0,sizeof(*v)); v->offset=offset; v->page=page;
    FILE *f=fopen(path,"rb"); if(!f) return error(v,"Cannot open file");
    if(fseeko(f,(off_t)offset,SEEK_SET)) { fclose(f); return error(v,"Cannot seek file"); }
    if(!offset) {
        unsigned char bom[3]; size_t n=fread(bom,1,3,f);
        if(n!=3 || memcmp(bom,"\xef\xbb\xbf",3)) {
            if(fseeko(f,0,SEEK_SET)) { fclose(f); return error(v,"Cannot seek file"); }
        }
    }
    for(unsigned row=0;row<INK_TEXT_ROWS;++row) {
        int peek=fgetc(f); if(peek==EOF) break; ungetc(peek,f);
        unsigned col=0; v->rows=row+1;
        while(col<INK_TEXT_COLUMNS) {
            int cp=codepoint(f);
            if(cp==EOF) break;
            if(cp==-2 || cp==0 || (cp<32 && cp!='\r' && cp!='\n' && cp!='\t') || cp==127) {
                fclose(f); return error(v,"Invalid text or binary file");
            }
            if(cp=='\r') { after_cr(f); break; }
            if(cp=='\n') break;
            if(cp=='\t') {
                unsigned stop=col+4-col%4;
                while(col<stop) v->lines[row][col++]=' ';
            } else v->lines[row][col++]=(char)cp;
        }
        /* A newline following a full-width line is its terminator, not a blank row. */
        if(col==INK_TEXT_COLUMNS) {
            int c=fgetc(f);
            if(c=='\r') after_cr(f);
            else if(c!=EOF && c!='\n') ungetc(c,f);
        }
    }
    off_t next=ftello(f);
    int peek=fgetc(f); v->has_next=peek!=EOF;
    bool bad=ferror(f)!=0 || next<0;
    fclose(f);
    if(bad) return error(v,"File read failed");
    v->next_offset=(uint64_t)next; return 0;
}
int ink_text_open(ink_text_view *v,const char *path) { return load(v,path,0,1); }
int ink_text_turn(ink_text_view *v,const char *path,int direction)
{
    if(direction>0) {
        if(!v->has_next) return 0;
        return load(v,path,v->next_offset,v->page+1)?-1:1;
    }
    if(v->page<=1) return 0;
    /* No document-sized index in RAM. Optimize backward seeking in a later stage. */
    uint64_t target=v->offset;
    ink_text_view previous;
    if(load(&previous,path,0,1)) return error(v,previous.error);
    while(previous.has_next && previous.next_offset<target) {
        if(load(&previous,path,previous.next_offset,previous.page+1)) return error(v,previous.error);
    }
    *v=previous; return 1;
}
