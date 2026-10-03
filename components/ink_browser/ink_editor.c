#define _POSIX_C_SOURCE 200809L
#include "ink_editor.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
enum { GAP=4096 };
static int error(ink_editor *e,const char *message)
{ snprintf(e->error,sizeof(e->error),"%s",message); return -1; }
static int read_at(ink_editor *e,uint64_t at,void *data,size_t n)
{ return fseeko(e->work,(off_t)at,SEEK_SET) || fread(data,1,n,e->work)!=n?-1:0; }
static int write_at(ink_editor *e,uint64_t at,const void *data,size_t n)
{ return fseeko(e->work,(off_t)at,SEEK_SET) || fwrite(data,1,n,e->work)!=n?-1:0; }
static int grow_gap(ink_editor *e);
static int move_gap(ink_editor *e,uint64_t target)
{
    if(target!=e->gap_start && e->gap_end==e->gap_start && grow_gap(e)) return -1;
    unsigned char bytes[GAP];
    while(e->gap_start!=target) {
        bool right=target>e->gap_start;
        uint64_t distance=right?target-e->gap_start:e->gap_start-target;
        size_t n=(size_t)(distance<GAP?distance:GAP);
        uint64_t gap=e->gap_end-e->gap_start;
        if(n>gap) n=(size_t)gap;
        if(!n) return error(e,"Working gap is full");
        uint64_t from=right?e->gap_end:e->gap_start-n;
        uint64_t to=right?e->gap_start:e->gap_end-n;
        if(read_at(e,from,bytes,n) || write_at(e,to,bytes,n)) return error(e,"Working file I/O failed");
        if(right) { e->gap_start+=n; e->gap_end+=n; }
        else { e->gap_start-=n; e->gap_end-=n; }
    }
    return 0;
}
static int grow_gap(ink_editor *e)
{
    unsigned char bytes[GAP];
    uint64_t end=e->size+e->gap_end-e->gap_start;
    if(fflush(e->work) || ftruncate(fileno(e->work),(off_t)(end+GAP))) return error(e,"Cannot grow working file");
    while(end>e->gap_end) {
        size_t n=(size_t)(end-e->gap_end<GAP?end-e->gap_end:GAP);
        if(read_at(e,end-n,bytes,n) || write_at(e,end-n+GAP,bytes,n)) return error(e,"Working file I/O failed");
        end-=n;
    }
    e->gap_end+=GAP; return 0;
}
/* Buffered virtual reader skips the physical gap without a document-sized index. */
typedef struct { ink_editor *e; uint64_t base; size_t used; unsigned char bytes[GAP]; } reader;
static int byte(reader *r,uint64_t at)
{
    if(at>=r->e->size) return EOF;
    if(!r->used || at<r->base || at-r->base>=r->used) {
        r->base=at;
        uint64_t available=r->e->size-at;
        if(at<r->e->gap_start && available>r->e->gap_start-at) available=r->e->gap_start-at;
        r->used=(size_t)(available<GAP?available:GAP);
        uint64_t physical=at+(at>=r->e->gap_start?r->e->gap_end-r->e->gap_start:0);
        if(read_at(r->e,physical,r->bytes,r->used)) { r->used=0; return -2; }
    }
    return r->bytes[at-r->base];
}
static int layout(ink_editor *e,uint64_t start)
{
    reader r={.e=e}; uint64_t at=start;
    memset(e->lines,0,sizeof(e->lines)); e->rows=0;
    e->cursor_row=e->cursor_column=0;
    for(unsigned row=0;row<INK_EDITOR_ROWS;row++) {
        unsigned col=0,used=0; e->rows=row+1;
        while(col<INK_EDITOR_COLUMNS) {
            e->cells[row][col]=at;
            if(at==e->gap_start) { e->cursor_row=row; e->cursor_column=col; }
            int c=byte(&r,at); if(c==-2) return error(e,"Working file read failed");
            if(c==EOF) break;
            uint64_t before=at++;
            if(c=='\n' || c=='\r') { if(c=='\r' && byte(&r,at)=='\n') at++; break; }
            if(c>=128) { while((byte(&r,at)&0xc0)==0x80) at++; }
            unsigned count=c=='\t'?4-col%4:1;
            while(count-- && col<INK_EDITOR_COLUMNS) {
                e->cells[row][col++]=before;
                if(c=='\t') e->lines[row][used++]=' ';
                else for(uint64_t i=before;i<at;i++) e->lines[row][used++]=(char)byte(&r,i);
            }
        }
        e->cells[row][col]=at;
        /* Blank end-of-line area places cursor before its newline. */
        uint64_t end=at;
        if(at>start && byte(&r,at-1)=='\n') {
            end=at-1; if(end && byte(&r,end-1)=='\r') end--;
        } else if(at>start && byte(&r,at-1)=='\r') end=at-1;
        for(unsigned i=col;i<=INK_EDITOR_COLUMNS;i++) e->cells[row][i]=end;
        if(col==INK_EDITOR_COLUMNS && byte(&r,at)=='\r') { at++; if(byte(&r,at)=='\n') at++; }
        else if(col==INK_EDITOR_COLUMNS && byte(&r,at)=='\n') at++;
        if(at>=e->size) {
            if(end<at && row+1<INK_EDITOR_ROWS) {
                row++; e->rows=row+1;
                for(unsigned i=0;i<=INK_EDITOR_COLUMNS;i++) e->cells[row][i]=at;
                col=0;
            }
            if(e->gap_start==e->size) { e->cursor_row=row; e->cursor_column=col; }
            break;
        }
    }
    e->offset=start; e->next=at; e->has_next=at<e->size; return 0;
}
int ink_editor_open(ink_editor *e,const char *path)
{
    memset(e,0,sizeof(*e));
    if(strlen(path)>=sizeof(e->path)) return error(e,"Path is too long");
    snprintf(e->path,sizeof(e->path),"%s",path);
    snprintf(e->temporary,sizeof(e->temporary),"%s.inkpy-XXXXXX",path);
    FILE *source=fopen(path,"rb"); if(!source) return error(e,"Cannot open file");
    int fd=mkstemp(e->temporary);
    if(fd<0) { fclose(source); return error(e,"Cannot create working file"); }
    e->work=fdopen(fd,"w+b");
    if(!e->work) { close(fd); unlink(e->temporary); fclose(source); return error(e,"Cannot open working file"); }
    e->gap_end=GAP;
    bool bad=fseeko(e->work,GAP,SEEK_SET)!=0;
    unsigned char block[GAP]; size_t n; unsigned remaining=0,value=0,minimum=0;
    while(!bad && (n=fread(block,1,sizeof(block),source))) {
        /* Validate entire UTF-8 text during the initial streaming copy. */
        for(size_t i=0;i<n && !bad;i++) {
            unsigned c=block[i];
            if(remaining) {
                if((c&0xc0)!=0x80) bad=true;
                else { value=(value<<6)|(c&63); if(!--remaining && (value<minimum || value>0x10ffff || (value>=0xd800 && value<=0xdfff))) bad=true; }
            } else if(c>=128) {
                if(c>=0xc2 && c<=0xdf) { remaining=1; value=c&31; minimum=0x80; }
                else if(c>=0xe0 && c<=0xef) { remaining=2; value=c&15; minimum=0x800; }
                else if(c>=0xf0 && c<=0xf4) { remaining=3; value=c&7; minimum=0x10000; }
                else bad=true;
            } else if(!c || c==127 || (c<32 && c!='\n' && c!='\r' && c!='\t')) bad=true;
        }
        if(!bad && fwrite(block,1,n,e->work)!=n) bad=true;
        e->size+=n;
    }
    bad|=ferror(source)!=0 || remaining!=0; fclose(source);
    if(!bad && (fflush(e->work) || ftruncate(fd,(off_t)(e->size+GAP)))) bad=true;
    if(bad) { ink_editor_discard(e); return error(e,"Invalid text or copy failed"); }
    if(layout(e,0)) { ink_editor_discard(e); return -1; }
    return 0;
}
void ink_editor_cursor(ink_editor *e,unsigned row,unsigned column)
{
    e->error[0]=0;
    if(row>=e->rows) row=e->rows-1;
    if(column>INK_EDITOR_COLUMNS) column=INK_EDITOR_COLUMNS;
    if(!move_gap(e,e->cells[row][column])) layout(e,e->offset);
}
int ink_editor_key(ink_editor *e,int key)
{
    if(e->menu || !e->work) return 0;
    e->error[0]=0;
    if(key==INK_KEY_DELETE) {
        if(!e->gap_start) return 0;
        reader r={.e=e}; uint64_t at=e->gap_start-1;
        while(at && (byte(&r,at)&0xc0)==0x80) at--;
        /* Keep CRLF as one line break. */
        if(byte(&r,at)=='\n' && at && byte(&r,at-1)=='\r') at--;
        e->size-=e->gap_start-at; e->gap_start=at;
    } else {
        if(key!=INK_KEY_ENTER && key!=INK_KEY_INDENT && (key<32 || key>126)) return 0;
        char bytes[4]; size_t n=key==INK_KEY_INDENT?4:1;
        memset(bytes,key==INK_KEY_INDENT?' ':key==INK_KEY_ENTER?'\n':key,n);
        if(e->gap_end-e->gap_start<n && grow_gap(e)) return -1;
        if(write_at(e,e->gap_start,bytes,n)) return error(e,"Working file write failed");
        e->gap_start+=n; e->size+=n;
    }
    e->dirty=true;
    if(e->gap_start<e->offset) e->offset=e->gap_start;
    if(layout(e,e->offset)) return -1;
    if(e->gap_start>=e->next && e->has_next) return layout(e,e->next);
    return 0;
}
int ink_editor_page(ink_editor *e,int direction)
{
    if(e->menu) return 0;
    e->error[0]=0;
    uint64_t target=0;
    if(direction>0) { if(!e->has_next) return 0; target=e->next; }
    else {
        if(!e->offset) return 0;
        uint64_t current=e->offset;
        while(!layout(e,target) && e->has_next && e->next<current) target=e->next;
        if(e->error[0]) return -1;
    }
    if(move_gap(e,target)) return -1;
    return layout(e,target);
}
#ifdef ESP_PLATFORM
/* FatFs f_rename rejects existing destinations. Keep the original as a sibling
 * backup until replacement succeeds; never unlink it to make room for Save. */
static int install_save(ink_editor *e,const char *saved)
{
    char backup[INK_EDITOR_PATH]; snprintf(backup,sizeof(backup),"%s.inkpy-XXXXXX",e->path);
    int fd=mkstemp(backup); if(fd<0) return error(e,"Cannot reserve backup name");
    int closed=close(fd);
    if(unlink(backup) || closed) return error(e,"Cannot prepare backup name");
    if(rename(e->path,backup)) return error(e,"Cannot back up original");
    if(rename(saved,e->path)) {
        if(rename(backup,e->path)) return error(e,"Save failed; original in backup");
        return error(e,"Save failed; original restored");
    }
    /* A leftover backup is preferable to failing a completed save. */
    unlink(backup); return 0;
}
#else
static int install_save(ink_editor *e,const char *saved)
{ return rename(saved,e->path)?error(e,"Save failed"):0; }
#endif
int ink_editor_save(ink_editor *e)
{
    char path[INK_EDITOR_PATH]; snprintf(path,sizeof(path),"%s.inkpy-XXXXXX",e->path);
    int fd=mkstemp(path); if(fd<0) return error(e,"Cannot create save file");
    FILE *out=fdopen(fd,"wb");
    if(!out) { close(fd); unlink(path); return error(e,"Cannot open save file"); }
    reader r={.e=e}; bool bad=false;
    for(uint64_t at=0;at<e->size && !bad;) {
        int c=byte(&r,at); if(c<0) { bad=true; break; }
        size_t n=r.used-(size_t)(at-r.base);
        if(fwrite(r.bytes+(at-r.base),1,n,out)!=n) bad=true;
        at+=n;
    }
    if(fflush(out) || fsync(fd)) bad=true;
    if(fclose(out)) bad=true;
    if(bad) { unlink(path); return error(e,"Save failed"); }
    if(install_save(e,path)) { unlink(path); return -1; }
    ink_editor_discard(e); return 0;
}
void ink_editor_discard(ink_editor *e)
{
    if(e->work) { fclose(e->work); e->work=NULL; unlink(e->temporary); }
}
bool ink_editor_home(ink_editor *e,bool long_press)
{
    if(long_press) { ink_editor_discard(e); return true; }
    e->menu=!e->menu; return false;
}
bool ink_editor_tap(ink_editor *e,unsigned x,unsigned y)
{
    if(e->menu) {
        if(x<32 || x>=448 || y<180 || y>=372) return false;
        unsigned row=(y-180)/64;
        if(row==0 && ink_editor_save(e)) return false;
        if(row==1) ink_editor_discard(e);
        e->menu=false; return row<2;
    }
    if(x>=16 && x<464 && y>=64 && y<370)
        ink_editor_cursor(e,(y-64)/34,(x-16)/18);
    else if(y>=INK_KB_Y+INK_EDITOR_KEYBOARD_OFFSET)
        ink_editor_key(e,ink_keyboard_tap(&e->keyboard,x,y-INK_EDITOR_KEYBOARD_OFFSET));
    return false;
}
