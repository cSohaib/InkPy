#define _POSIX_C_SOURCE 200809L
#include "ink_reader.h"
#include "../ink_browser/ink_ui_font.h"
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int error(ink_reader *r,const char *s)
{ snprintf(r->error,sizeof(r->error),"%s",s); return -1; }
static int path(ink_reader *r,char out[560],const char *name)
{ int n=snprintf(out,560,"%s/%s",r->cache,name); return n<0||n>=560?-1:0; }
static uint64_t number(const unsigned char *b,unsigned n)
{ uint64_t v=0; for(unsigned i=0;i<n;i++) v|=(uint64_t)b[i]<<(8*i); return v; }
static int seek_record(FILE *f,uint64_t index,unsigned size)
{
    if(index>(uint64_t)LONG_MAX/size) return -1;
    return fseek(f,(long)(index*size),SEEK_SET);
}
static int chapter(ink_reader *r,unsigned id,char *title,unsigned *page)
{
    if(!id) { strcpy(title,"No chapter"); *page=1; return 0; }
    unsigned char b[212];
    if(id>r->stats.chapters||seek_record(r->chapters,id-1,212)||fread(b,1,sizeof(b),r->chapters)!=sizeof(b))
        return error(r,"Cannot read chapter");
    unsigned n=(unsigned)number(b+16,2); *page=(unsigned)number(b+8,4);
    if(n>=INK_TITLE_BYTES||number(b+12,4)!=id||!*page||*page>r->stats.pages)
        return error(r,"Invalid chapter index");
    memcpy(title,b+20,n); title[n]=0; if(!n) strcpy(title,"Untitled"); return 0;
}
static int page_record(ink_reader *r,unsigned char b[32])
{
    if(!r->page||r->page>r->stats.pages||seek_record(r->pages,r->page-1,32)||fread(b,1,32,r->pages)!=32)
        return error(r,"Cannot read page");
    r->chapter=(unsigned)number(b+24,4); unsigned ignored;
    return chapter(r,r->chapter,r->title,&ignored);
}
void ink_reader_close(ink_reader *r)
{
    if(r->draw) fclose(r->draw);
    if(r->pages) fclose(r->pages);
    if(r->chapters) fclose(r->chapters);
    r->draw=r->pages=r->chapters=NULL;
    if(r->cache[0]) for(unsigned i=0;i<4;i++) {
        char p[560]; const char *names[]={"draw","pages","chapters","bitmap"};
        if(!path(r,p,names[i])) unlink(p);
    }
}
int ink_reader_open(ink_reader *r,const char *source,const char *root,ink_layout_math math,void (*progress)(void))
{
    memset(r,0,sizeof(*r)); char p[560];
    int n=snprintf(r->cache,sizeof(r->cache),"%s/.inkpy-reader",root);
    if(n<0||(size_t)n>=sizeof(r->cache)) { r->cache[0]=0; return error(r,"Cache path too long"); }
    bool created=mkdir(r->cache,0700)==0;
    if(!created&&errno!=EEXIST) { r->cache[0]=0; return error(r,"Cannot create reader cache"); }
    path(r,p,"owner"); FILE *owner=fopen(p,created?"wb":"rb");
    const char marker[]="InkPy reader cache v2\n"; char check[sizeof(marker)]={0};
    bool valid=false;
    if(owner) {
        if(created) valid=fwrite(marker,1,sizeof(marker)-1,owner)==sizeof(marker)-1;
        else valid=fread(check,1,sizeof(marker)-1,owner)==sizeof(marker)-1&&!memcmp(check,marker,sizeof(marker)-1);
        if(fclose(owner)) valid=false;
    }
    if(!valid) { r->cache[0]=0; return error(r,"Reader cache owner mismatch"); }
    FILE *input=fopen(source,"rb"),*spool=NULL;
    if(!input) return error(r,"Cannot open Markdown");
    path(r,p,"draw"); r->draw=fopen(p,"w+b");
    path(r,p,"pages"); r->pages=fopen(p,"w+b");
    path(r,p,"chapters"); r->chapters=fopen(p,"w+b");
    if(math) { path(r,p,"bitmap"); spool=fopen(p,"w+b"); }
    int status=-1;
    if(!r->draw||!r->pages||!r->chapters||(math&&!spool)) error(r,"Cannot open reader cache");
    else {
        ink_layout_config cfg={.width=480,.height=800,.font_pixels=22,.read_bytes=1024,
            .render_math=math,.bitmap_spool=spool,.progress=progress};
        status=ink_layout_run(input,r->draw,r->pages,r->chapters,&cfg,&r->stats);
        if(status) error(r,r->stats.error);
        else if(r->stats.pages>UINT_MAX||r->stats.chapters>UINT_MAX) status=error(r,"Reader index too large");
    }
    if(fclose(input)) status=error(r,"Markdown close failed");
    if(spool&&fclose(spool)) status=error(r,"Bitmap cache close failed");
    if(status) { ink_reader_close(r); return -1; }
    path(r,p,"bitmap"); unlink(p);
    r->page=r->stats.pages?1:0; strcpy(r->title,"No chapter");
    if(r->page) { unsigned char b[32]; if(page_record(r,b)) { ink_reader_close(r); return -1; } }
    return 0;
}
bool ink_reader_page(ink_reader *r,int direction)
{
    if(r->view!=INK_READER_DEFINITION&&r->view!=INK_READER_DICTIONARIES) r->error[0]=0;
    if(r->view==INK_READER_DEFINITION) {
        if(r->dictionary) ink_dict_page(r->dictionary,direction);
    } else if(r->view==INK_READER_DICTIONARIES) {
        ink_dict *d=r->dictionary;
        if(d) {
            unsigned first=d->first;
            if(direction>0&&first+INK_DICT_ROWS<d->total) first+=INK_DICT_ROWS;
            if(direction<0) first=first>=INK_DICT_ROWS?first-INK_DICT_ROWS:0;
            ink_dict_catalog(d,first);
        }
    } else if(r->view==INK_READER_CHAPTERS) {
        if(direction>0&&r->chapter_first+8<=r->stats.chapters) r->chapter_first+=8;
        if(direction<0&&r->chapter_first>8) r->chapter_first-=8;
    } else if(r->view==INK_READER_PAGE) {
        if(direction>0&&r->page<r->stats.pages) r->page++;
        if(direction<0&&r->page>1) r->page--;
    }
    return true;
}
void ink_reader_home(ink_reader *r)
{
    if(r->view==INK_READER_DICTIONARIES) { r->view=INK_READER_DEFINITION; return; }
    r->view=r->view==INK_READER_PAGE?INK_READER_MENU:INK_READER_PAGE; r->error[0]=0;
}
static bool word_character(uint32_t cp)
{
    return (cp>='a'&&cp<='z')||(cp>='A'&&cp<='Z')||(cp>='0'&&cp<='9')||cp=='_'||cp=='\''||cp=='-'||
        (cp>=128&&cp!=160&&!(cp>=0x2000&&cp<=0x206f));
}
static int lookup_word(ink_reader *r,unsigned tap_x,unsigned tap_y)
{
    if(!r->page) return 0;
    unsigned char page[32]; if(page_record(r,page)) return -1;
    uint64_t at=number(page,8),end=number(page+8,8);
    if(at>end||seek_record(r->draw,at,1)) return error(r,"Cannot read tapped word");
    char word[256]; unsigned used=0,prev_end=0,prev_y=0,prev_height=0,prev_cell=0;
    bool hit=false,overflow=false;
    while(at<end) {
        unsigned char h[20];
        if(end-at<20||fread(h,1,20,r->draw)!=20) return error(r,"Invalid word run");
        at+=20; uint64_t size=number(h+8,4);
        unsigned x=(unsigned)number(h,2),y=(unsigned)number(h+2,2),cell=(unsigned)number(h+4,2),style=(unsigned)number(h+6,2);
        unsigned level=(style>>8)&7,height=22+(level?2*(7-level):0);
        if(size>end-at||!cell||cell>480) return error(r,"Invalid word geometry");
        bool continuation=(x==prev_end&&y==prev_y)||(x==16&&prev_end>=464-prev_cell&&y==prev_y+prev_height+8);
        if(used&&!continuation) { if(hit) goto found; used=0; overflow=false; }
        if(style==INK_BITMAP||(style&INK_MATH)) {
            if(hit) goto found;
            used=0; overflow=false;
            if(size>LONG_MAX||fseek(r->draw,(long)size,SEEK_CUR)) return error(r,"Cannot skip formula");
        } else {
            char payload[513];
            if(size>512||fread(payload,1,(size_t)size,r->draw)!=size) return error(r,"Cannot read tapped word");
            unsigned i=0;
            while(i<size) {
                unsigned start=i; unsigned char c=(unsigned char)payload[i++]; uint32_t cp=c;
                unsigned n=c<128?0:(c&0xe0)==0xc0?1:(c&0xf0)==0xe0?2:3;
                if(n) { cp=c&((1u<<(6-n))-1); if(n>size-i) return error(r,"Invalid word UTF-8");
                    while(n--) cp=(cp<<6)|((unsigned char)payload[i++]&63); }
                bool punctuation=cp=='\''||cp=='-';
                if(word_character(cp)&&(!punctuation||used)) {
                    if(used+i-start<sizeof(word)) { memcpy(word+used,payload+start,i-start); used+=i-start; }
                    else overflow=true;
                    hit|=tap_x>=x&&tap_x<x+cell&&tap_y>=y&&tap_y<y+height;
                } else { if(hit) goto found; used=0; overflow=false; }
                x+=cell;
            }
        }
        prev_end=x; prev_y=y; prev_height=height; prev_cell=cell; at+=size;
    }
    if(!hit) return 0;
found:
    while(used&&(word[used-1]=='\''||word[used-1]=='-')) used--;
    if(!used) return 0;
    word[used]=0; r->view=INK_READER_DEFINITION;
    snprintf(r->word,sizeof(r->word),"%s",word);
    if(overflow) { r->word[0]=0; return error(r,"Tapped word exceeds 255 bytes"); }
    if(r->dictionary) {
        if(ink_dict_lookup(r->dictionary,word)<0) error(r,r->dictionary->error);
    }
    else error(r,"No dictionaries found");
    return 1;
}
bool ink_reader_tap(ink_reader *r,unsigned x,unsigned y)
{
    if(r->view==INK_READER_PAGE) { r->error[0]=0; lookup_word(r,x,y); }
    else if(r->view==INK_READER_DEFINITION&&y>=704&&y<768) {
        if(x>=16&&x<328) { r->view=INK_READER_DICTIONARIES; if(r->dictionary) ink_dict_catalog(r->dictionary,0); }
        else if(x>=328&&x<464) r->view=INK_READER_PAGE;
    } else if(r->view==INK_READER_DICTIONARIES&&r->dictionary&&x>=16&&x<464&&y>=120&&y<632) {
        ink_dict *d=r->dictionary; unsigned row=(y-120)/64;
        if(row<d->count) {
            if(ink_dict_select(d,d->rows[row].path)) error(r,d->error);
            else if(r->word[0]) { r->error[0]=0; if(ink_dict_lookup(d,r->word)<0) error(r,d->error); }
            r->view=INK_READER_DEFINITION;
        }
    } else if(r->view==INK_READER_MENU&&x>=32&&x<448&&y>=180&&y<372) {
        unsigned row=(y-180)/64;
        if(row==2) return true;
        r->view=row?INK_READER_GOTO:INK_READER_CHAPTERS; r->digits[0]=0; r->chapter_first=1;
    } else if(r->view==INK_READER_CHAPTERS&&x>=16&&x<464&&y>=120&&y<632) {
        unsigned id=r->chapter_first+(y-120)/64,page; char title[INK_TITLE_BYTES];
        if(id<=r->stats.chapters&&!chapter(r,id,title,&page)) { r->page=page; r->view=INK_READER_PAGE; }
    } else if(r->view==INK_READER_GOTO&&x>=32&&x<448&&y>=220&&y<540) {
        unsigned col=(x-32)/138,row=(y-220)/80,key=row*3+col;
        size_t n=strlen(r->digits); r->error[0]=0;
        if(row<3&&n<10) { r->digits[n]=(char)('1'+key); r->digits[n+1]=0; }
        else if(row==3&&col==0&&n<10) { r->digits[n]='0'; r->digits[n+1]=0; }
        else if(row==3&&col==1&&n) r->digits[n-1]=0;
        else if(row==3&&col==2) {
            unsigned long value=strtoul(r->digits,NULL,10);
            if(!n||!value||value>r->stats.pages) error(r,"Page out of range");
            else { r->page=(unsigned)value; r->view=INK_READER_PAGE; }
        }
    }
    return false;
}
static void pixel(uint8_t *f,unsigned x,unsigned y)
{
    if(x>=480||y>=800) return;
    unsigned dx=799-y,dy=x; f[dy*100+dx/8]&=(uint8_t)~(0x80>>(dx%8));
}
static void text(uint8_t *f,unsigned x,unsigned y,const char *s,unsigned cell,unsigned height,unsigned style)
{
    unsigned shift=0;
    for(;*s;s++) {
        unsigned char c=(unsigned char)*s;
        if((c&0xc0)==0x80) continue;
        if(c<32||c>126) c='?';
        for(unsigned dy=0;dy<height;dy++) {
            shift=(style&INK_ITALIC)?(height-dy)/8:0;
            for(unsigned dx=0;dx+1<cell;dx++) if(ink_ui_font[c-32][dy*16/height]&(1u<<(dx*9/(cell-1)))) {
                pixel(f,x+dx+shift,y+dy);
                if(style&INK_BOLD) pixel(f,x+dx+shift+1,y+dy);
            }
        }
        x+=cell; if(x>=464) break;
    }
}
static void label(uint8_t *f,unsigned x,unsigned y,const char *s)
{ text(f,x,y,s,18,32,0); }
static void box(uint8_t *f,unsigned x,unsigned y,unsigned w,unsigned h)
{
    for(unsigned i=0;i<w;i++) { pixel(f,x+i,y); pixel(f,x+i,y+h-1); }
    for(unsigned i=0;i<h;i++) { pixel(f,x,y+i); pixel(f,x+w-1,y+i); }
}
int ink_reader_draw(ink_reader *r,uint8_t frame[48000])
{
    memset(frame,255,48000);
    if(r->page) {
        unsigned char p[32]; if(page_record(r,p)) return -1;
        uint64_t begin=number(p,8),end=number(p+8,8);
        if(number(p+28,4)||begin>end||end>LONG_MAX||fseek(r->draw,(long)begin,SEEK_SET)) return error(r,"Invalid page offsets");
        for(uint64_t at=begin;at<end;) {
            unsigned char h[20];
            if(end-at<20||fread(h,1,20,r->draw)!=20) return error(r,"Truncated page run");
            at+=20;
            unsigned x=(unsigned)number(h,2),y=(unsigned)number(h+2,2),cell=(unsigned)number(h+4,2),style=(unsigned)number(h+6,2);
            uint64_t size=number(h+8,4);
            if(!size||size>end-at||x>=480||y>=800||!cell) return error(r,"Invalid page run");
            if(style==INK_BITMAP) {
                unsigned char b[2],row[60];
                if(size<2||cell>480||fread(b,1,2,r->draw)!=2) return error(r,"Invalid formula bitmap");
                unsigned height=(unsigned)number(b,2),stride=(cell+7)/8;
                if(!height||height>800-y||cell>480-x||size!=2+(uint64_t)stride*height) return error(r,"Formula outside page");
                for(unsigned dy=0;dy<height;dy++) {
                    if(fread(row,1,stride,r->draw)!=stride) return error(r,"Truncated formula bitmap");
                    for(unsigned dx=0;dx<cell;dx++) if(row[dx/8]&(0x80>>(dx%8))) pixel(frame,x+dx,y+dy);
                }
            } else {
                char payload[513];
                if(size>512||cell>100||fread(payload,1,(size_t)size,r->draw)!=size) return error(r,"Invalid text run");
                payload[size]=0; unsigned level=(style>>8)&7,height=22+(level?2*(7-level):0);
                text(frame,x,y,payload,cell,height,style|(level?INK_BOLD:0));
            }
            at+=size;
        }
    }
    if(r->view!=INK_READER_PAGE) {
        memset(frame,255,48000); char line[64];
        if(r->view==INK_READER_DEFINITION||r->view==INK_READER_DICTIONARIES) {
            ink_dict *d=r->dictionary; label(frame,16,16,r->word);
            if(r->view==INK_READER_DICTIONARIES) {
                label(frame,16,64,"Choose dictionary");
                if(!d||!d->total) label(frame,16,120,"No dictionaries found");
                if(d) for(unsigned i=0;i<d->count;i++) { box(frame,16,120+i*64,448,64); label(frame,24,136+i*64,d->rows[i].name); }
            } else {
                label(frame,16,64,d&&d->name[0]?d->name:"No dictionary selected");
                if(r->error[0]) label(frame,16,144,r->error);
                else if(d&&d->error[0]) {
                    for(unsigned row=0;row<5;row++) { char part[25]; size_t at=(size_t)row*24,n=strlen(d->error);
                        if(at>=n) break;
                        snprintf(part,sizeof(part),"%.*s",24,d->error+at); label(frame,16,144+row*36,part); }
                } else if(d) for(unsigned row=0;row<INK_DICT_LINES;row++) label(frame,16,144+row*32,d->lines[row]);
                if(d&&d->pages&&!r->error[0]&&!d->error[0]) { snprintf(line,sizeof(line),"%u / %u",d->page+1,d->pages); label(frame,16,664,line); }
                box(frame,16,704,312,64); text(frame,24,722,"Change dictionary",16,28,0);
                box(frame,328,704,136,64); label(frame,344,720,"Close");
            }
            return 0;
        }
        snprintf(line,sizeof(line),"Page %u / %u",r->page,(unsigned)r->stats.pages); label(frame,16,16,line);
        label(frame,16,64,r->title);
        if(r->view==INK_READER_MENU) {
            const char *names[]={"Go to chapter","Go to page","Close"};
            for(unsigned i=0;i<3;i++) { box(frame,32,180+i*64,416,64); label(frame,48,196+i*64,names[i]); }
        } else if(r->view==INK_READER_CHAPTERS) {
            if(!r->stats.chapters) label(frame,16,120,"No H2 chapters");
            for(unsigned i=0;i<8&&r->chapter_first+i<=r->stats.chapters;i++) {
                char title[INK_TITLE_BYTES]; unsigned page;
                if(chapter(r,r->chapter_first+i,title,&page)) return -1;
                box(frame,16,120+i*64,448,64); label(frame,24,136+i*64,title);
            }
        } else {
            label(frame,32,140,r->digits[0]?r->digits:"Page number");
            for(unsigned row=0;row<4;row++) for(unsigned col=0;col<3;col++) {
                char key[8];
                if(row<3) snprintf(key,sizeof(key),"%u",1+row*3+col);
                else snprintf(key,sizeof(key),"%s",col==0?"0":col==1?"Del":"Go");
                box(frame,32+col*138,220+row*80,138,80); label(frame,72+col*138,244+row*80,key);
            }
        }
        if(r->error[0]) label(frame,16,680,r->error);
    }
    return 0;
}
