#define _POSIX_C_SOURCE 200809L
#include "ink_reader.h"
#include "ink_epub.h"
#include "ink_image.h"
#include <strings.h>
#include "../ink_browser/ink_ui_font.h"
#include "../ink_browser/ink_ui.h"
#include "../ink_browser/ink_icons.h"
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef INK_USE_FONTS
#include "ink_font.h"
#endif
static unsigned page_width(const ink_reader *r) { return r->landscape?800:480; }
static unsigned page_height(const ink_reader *r) { return r->landscape?480:800; }
/* Drawing callbacks are synchronous and owned by the UI task. */
static const ink_reader *drawing;

static int error(ink_reader *r,const char *s)
{ snprintf(r->error,sizeof(r->error),"%s",s);return -1; }
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
    if(r->epub) {
        ink_epub_chapter c;
        if(!id){title[0]=0;*page=1;
    return 0;}
        if(ink_epub_get_chapter(r->book_cache,id,&c))return error(r,"Cannot read EPUB contents");
        snprintf(title,INK_TITLE_BYTES,"%s",c.title);*page=1;
    return 0;
    }
    if(!id) { strcpy(title,""); *page=1; return 0; }
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
    if(r->epub) {
        uint64_t at=number(b+16,8);r->chapter=0;r->title[0]=0;
        char p[560];path(r,p,"epub-positions");FILE *f=fopen(p,"rb");
        uint64_t record[2],best=0;
        if(f) {
            while(fread(record,1,sizeof(record),f)==sizeof(record))if(record[1]<=at&&record[1]>=best){r->chapter=(unsigned)record[0];best=record[1];}
            fclose(f);
        }
        if(r->chapter){ink_epub_chapter c;if(!ink_epub_get_chapter(r->book_cache,r->chapter,&c))snprintf(r->title,sizeof(r->title),"%s",c.title);}
        return 0;
    }
    r->chapter=(unsigned)number(b+24,4); unsigned ignored;
    return chapter(r,r->chapter,r->title,&ignored);
}
static void resume_save(ink_reader *r);
static void document_close(ink_reader *r)
{
    ink_layout_end(r->layout);r->layout=NULL;
    FILE **files[]={&r->draw,&r->pages,&r->chapters,&r->input,&r->anchors,&r->bitmap,&r->table};
    for(unsigned i=0;i<7;i++)if(*files[i]){fclose(*files[i]);*files[i]=NULL;}
}
void ink_reader_close(ink_reader *r)
{
    resume_save(r);document_close(r);
    if(r->epub)return; /* Successful EPUB caches survive Close and reboot. */
    if(r->cache[0])ink_epub_cleanup(r->cache);
    if(r->cache[0]) for(unsigned i=0;i<5;i++) {
        char p[560]; const char *names[]={"draw","pages","chapters","bitmap","table"};
        if(!path(r,p,names[i])) unlink(p);
    }
}
static char image_folder[512];
static void (*image_progress)(void);
static ink_reader *image_reader;
static int render_image(const char *resource,unsigned mw,unsigned mh,uint8_t *bits,unsigned *w,unsigned *h)
{
    if(strstr(resource,":")||!resource[0])return -1;
    char file[1024];int n=resource[0]=='/'?snprintf(file,sizeof(file),"%s",resource):snprintf(file,sizeof(file),"%s/%s",image_folder,resource);
    if(n<0||(size_t)n>=sizeof(file))return -1;
    char *out=file;
    for(char *in=file;*in;in++) {
        if(*in=='%'&&in[1]&&in[2]) {
            char hex[3]={in[1],in[2],0},*tail;unsigned long value=strtoul(hex,&tail,16);
            if(!*tail&&value){*out++=(char)value;in+=2;continue;}
        }*out++=*in;
    }*out=0;
    if(image_reader&&image_reader->epub) {
        if(ink_epub_image(image_reader->source,image_reader->cache,resource,file,sizeof(file),image_progress))return -1;
    }
    int status=ink_image_render(file,mw,mh,bits,w,h,image_progress);
    return status;
}

static int save_layout(ink_reader *r)
{
    char p[560],temp[560];path(r,p,"state");path(r,temp,"state-next");
    FILE *f=fopen(temp,"wb");
    if(!f)return error(r,"Cannot save reader continuation");
    int result=ink_layout_save(r->layout,f);
    if(fclose(f))result=-1;
    if(!result&&rename(temp,p)) {
        /* FatFS rename cannot replace an existing destination. The complete
         * state-next remains recoverable if power is lost between these calls. */
        if(errno!=EEXIST||unlink(p)||rename(temp,p))result=-1;
    }
    if(result){unlink(temp);
    return error(r,"Cannot save reader continuation");}
    return 0;
}
static int extend(ink_reader *r,unsigned target)
{
    while(!r->eof&&r->local_stats.pages<target) {
        /* UI reads seek the same streams. Restore the writer's append position. */
        if(fseek(r->draw,0,SEEK_END)||fseek(r->pages,0,SEEK_END)||fseek(r->chapters,0,SEEK_END))return error(r,"Cannot append reader cache");
        int result=ink_layout_step(r->layout);
        if(result<0)return error(r,r->local_stats.error);
        r->eof=result==1;
    }
    r->stats=r->local_stats;r->stats.chapters=ink_epub_chapters(r->book_cache);
    if(save_layout(r))return -1;
    return 0;
}
static int reach_anchor(ink_reader *r,uint64_t anchor)
{
    while(!r->eof) {
        unsigned char record[32];
        if(r->stats.pages&&!seek_record(r->pages,r->stats.pages-1,32)&&fread(record,1,32,r->pages)==32&&number(record+16,8)>=anchor)break;
        if(extend(r,(unsigned)r->stats.pages+1))return -1;
    }
    return 0;
}
static int load_document(ink_reader *r,unsigned document)
{
    document_close(r);r->eof=false;r->document=document;r->page=0;
    int n=snprintf(r->cache,sizeof(r->cache),"%s/d%u",r->book_cache,document);
    if(n<0||(size_t)n>=sizeof(r->cache))return error(r,"EPUB cache path too long");
    if(mkdir(r->cache,0700)&&errno!=EEXIST)return error(r,"Cannot create EPUB document cache");
    char p[560];path(r,p,"ready");bool ready=access(p,F_OK)==0;
    if(!ready) {
        ink_epub_cleanup(r->cache);
        if(ink_epub_document(r->source,r->book_cache,document,r->cache,r->progress,r->error,sizeof(r->error)))return -1;
        path(r,p,"ready");FILE *f=fopen(p,"wb");
    if(!f)return error(r,"Cannot mark EPUB document cache");
    fclose(f);
    }
    path(r,p,"epub-body");r->input=fopen(p,"rb");
    path(r,p,"epub-chapters");r->anchors=fopen(p,"rb");
    path(r,p,"state");FILE *state=fopen(p,"rb");
    if(!state){path(r,p,"state-next");state=fopen(p,"rb");}
    bool cached=state!=NULL;
    path(r,p,"draw");r->draw=fopen(p,cached?"r+b":"w+b");
    path(r,p,"pages");r->pages=fopen(p,cached?"r+b":"w+b");
    path(r,p,"chapters");r->chapters=fopen(p,cached?"r+b":"w+b");
    path(r,p,"bitmap");r->bitmap=fopen(p,cached?"r+b":"w+b");
    path(r,p,"table");r->table=fopen(p,cached?"r+b":"w+b");
    snprintf(image_folder,sizeof(image_folder),"%s",r->cache);image_progress=r->progress;image_reader=r;
    ink_layout_config cfg={.width=page_width(r),.height=page_height(r),.font_pixels=INK_UI_FONT,.read_bytes=1024,
        .render_math=r->math,.render_image=render_image,.chapter_spool=r->anchors,
        .bitmap_spool=r->bitmap,.table_spool=r->table,.cancelled=ink_epub_cancelled,.progress=r->progress};
    if(!r->input||!r->anchors||!r->draw||!r->pages||!r->chapters||!r->bitmap||!r->table) {
        if(state)fclose(state);
    return error(r,"Cannot open EPUB layout cache");
    }
    if(state) {
        r->layout=ink_layout_restore(state,r->input,r->draw,r->pages,r->chapters,&cfg,&r->local_stats);
    fclose(state);
        if(!r->layout) {

            FILE *streams[]={r->draw,r->pages,r->chapters,r->bitmap,r->table};
            for(unsigned i=0;i<5;i++){if(ftruncate(fileno(streams[i]),0)||fseek(streams[i],0,SEEK_SET))return error(r,"Cannot reset invalid cache");}
            rewind(r->input);rewind(r->anchors);cached=false;
        }
    }
    if(!r->layout)r->layout=ink_layout_begin(r->input,r->draw,r->pages,r->chapters,&cfg,&r->local_stats);
    if(!r->layout)return error(r,r->local_stats.error);
    /* A restored continuation reports EOF through step without reprocessing. */
    if(cached) {
        r->stats=r->local_stats;r->stats.chapters=ink_epub_chapters(r->book_cache);
        r->eof=false;
    }
    if(!r->local_stats.pages&&extend(r,1))return -1;
    r->stats=r->local_stats;r->stats.chapters=ink_epub_chapters(r->book_cache);
    r->page=r->stats.pages?1:0;r->view=INK_READER_PAGE;
    return 0;
}
static uint64_t hash_bytes(uint64_t value,const void *data,size_t size)
{
    const unsigned char *p=data;while(size--)value=(value^*p++)*1099511628211ULL;
    return value;
}
static int epub_open(ink_reader *r,const char *source,ink_layout_math math,void (*progress)(void))
{
    r->epub=true;r->math=math;r->progress=progress;
    if(strlen(source)>=sizeof(r->source))return error(r,"EPUB path too long");
    strcpy(r->source,source);
    struct stat st;
    if(stat(source,&st))return error(r,"Cannot stat EPUB");
    uint64_t key=hash_bytes(1469598103934665603ULL,source,strlen(source));
    key=hash_bytes(key,&st.st_size,sizeof(st.st_size));key=hash_bytes(key,&st.st_mtime,sizeof(st.st_mtime));
    key=hash_bytes(key,&r->landscape,sizeof(r->landscape));
#ifdef INK_USE_FONTS
    const char *font=ink_font_name(ink_font_selected());key=hash_bytes(key,font,strlen(font));
    char font_path[640];snprintf(font_path,sizeof(font_path),"/sd/fonts/%s",font);
    if(!stat(font_path,&st)){key=hash_bytes(key,&st.st_size,sizeof(st.st_size));key=hash_bytes(key,&st.st_mtime,sizeof(st.st_mtime));}
#endif
    int n=snprintf(r->book_cache,sizeof(r->book_cache),"%s/e39-%016llx",r->cache,(unsigned long long)key);
    if(n<0||(size_t)n>=sizeof(r->book_cache))return error(r,"EPUB cache path too long");
    if(mkdir(r->book_cache,0700)&&errno!=EEXIST)return error(r,"Cannot create EPUB cache");
    char p[640];snprintf(p,sizeof(p),"%s/ready",r->book_cache);
    if(access(p,F_OK)) {
        ink_epub_cleanup(r->book_cache);
        if(ink_epub_prepare(source,r->book_cache,progress,r->error,sizeof(r->error)))return -1;
        FILE *f=fopen(p,"wb");
    if(!f)return error(r,"Cannot mark EPUB cache");
    fclose(f);
    }
    r->documents=ink_epub_documents(r->book_cache);
    if(!r->documents)return error(r,"EPUB has no documents");
    unsigned document=1,screen=1;
    snprintf(p,sizeof(p),"%s/resume",r->book_cache);FILE *f=fopen(p,"rb");
    if(f){if(fscanf(f,"%u %u",&document,&screen)!=2||!document||document>r->documents){document=screen=1;}fclose(f);}
    if(load_document(r,document)){document_close(r);
    return -1;}
    if(screen&&screen<=r->stats.pages)r->page=screen;
    return 0;
}
static void resume_save(ink_reader *r)
{
    if(!r->epub)return;
    char p[640];snprintf(p,sizeof(p),"%s/resume",r->book_cache);FILE *f=fopen(p,"wb");
    if(f){fprintf(f,"%u %u",r->document,r->page);
    fclose(f);}
}
static int open_book(ink_reader *r,const char *source,const char *root,ink_layout_math math,void (*progress)(void),bool landscape)
{
    ink_epub_reset_cancel();memset(r,0,sizeof(*r)); r->landscape=landscape; char p[560];
    int n=snprintf(r->cache,sizeof(r->cache),"%s/.inkpy-reader",root);
    if(n<0||(size_t)n>=sizeof(r->cache)) { r->cache[0]=0; return error(r,"Cache path too long"); }
    struct stat st; bool exists=stat(r->cache,&st)==0;
    if(exists&&!S_ISDIR(st.st_mode)) { r->cache[0]=0; return error(r,"Reader cache path is not a directory"); }
    bool created=!exists&&mkdir(r->cache,0700)==0;
    if(!exists&&!created) {
        int code=errno; r->cache[0]=0;
        snprintf(r->error,sizeof(r->error),"Cannot create reader cache: errno %d (%s)",code,strerror(code)); return -1;
    }
    path(r,p,"owner"); FILE *owner=fopen(p,created?"wb":"rb");
    const char marker[]="InkPy reader cache v2\n"; char check[sizeof(marker)]={0};
    bool valid=false;
    if(owner) {
        if(created) valid=fwrite(marker,1,sizeof(marker)-1,owner)==sizeof(marker)-1;
        else valid=fread(check,1,sizeof(marker)-1,owner)==sizeof(marker)-1&&!memcmp(check,marker,sizeof(marker)-1);
        if(fclose(owner)) valid=false;
    }
    if(!valid) { r->cache[0]=0; return error(r,"Reader cache owner mismatch"); }
    const char *extension=strrchr(source,'.');bool epub=extension&&!strcasecmp(extension,".epub");
    if(epub)return epub_open(r,source,math,progress);
    FILE *anchors=NULL;
    const char *input_path=source;
    const char *slash=strrchr(source,'/');size_t folder=slash?(size_t)(slash-source):0;
    if(folder>=sizeof(image_folder)){if(anchors)fclose(anchors);ink_reader_close(r);
    return error(r,"Image folder too long");}
    if(folder){memcpy(image_folder,source,folder);image_folder[folder]=0;}else strcpy(image_folder,".");
    if(epub)snprintf(image_folder,sizeof(image_folder),"%s",r->cache);
    image_progress=progress;image_reader=r;
    FILE *input=fopen(input_path,"rb"),*spool=NULL,*table=NULL;
    if(!input){if(anchors)fclose(anchors);ink_reader_close(r);
    return error(r,"Cannot open book");}
    path(r,p,"draw"); r->draw=fopen(p,"w+b");
    path(r,p,"pages"); r->pages=fopen(p,"w+b");
    path(r,p,"chapters"); r->chapters=fopen(p,"w+b");
    path(r,p,"bitmap"); spool=fopen(p,"w+b");
    path(r,p,"table"); table=fopen(p,"w+b");
    int status=-1;
    if(!r->draw||!r->pages||!r->chapters||!table||!spool) error(r,"Cannot open reader cache");
    else {
        ink_layout_config cfg={.width=page_width(r),.height=page_height(r),.font_pixels=INK_UI_FONT,.read_bytes=1024,
            .render_math=math,.render_image=render_image,.chapter_spool=anchors,.bitmap_spool=spool,.table_spool=table,.progress=progress};
        status=ink_layout_run(input,r->draw,r->pages,r->chapters,&cfg,&r->stats);
        if(status) error(r,r->stats.error);
        else if(r->stats.pages>UINT_MAX||r->stats.chapters>UINT_MAX) status=error(r,"Reader index too large");
    }
    if(fclose(input)) status=error(r,"Book close failed");
    if(anchors&&fclose(anchors))status=error(r,"Chapter anchor close failed");
    if(epub)ink_epub_cleanup(r->cache);
    if(spool&&fclose(spool)) status=error(r,"Bitmap cache close failed");
    if(table&&fclose(table)) status=error(r,"Table cache close failed");
    if(status) { ink_reader_close(r); return -1; }
    path(r,p,"bitmap"); unlink(p);
    path(r,p,"table"); unlink(p);
    r->page=r->stats.pages?1:0; strcpy(r->title,"");
    if(r->page) { unsigned char b[32]; if(page_record(r,b)) { ink_reader_close(r); return -1; } }
    return 0;
}
int ink_reader_open(ink_reader *r,const char *source,const char *root,ink_layout_math math,void (*progress)(void))
{ return open_book(r,source,root,math,progress,false); }
int ink_reader_rotate(ink_reader *r,const char *source,const char *root,ink_layout_math math,void (*progress)(void))
{
    unsigned document=r->document;bool epub=r->epub;
    unsigned char record[32]; uint64_t anchor=0;
    if(r->page) { if(page_record(r,record)) return -1; anchor=number(record+16,8); }
    bool landscape=!r->landscape; ink_dict *dictionary=r->dictionary;
    ink_reader_close(r);
    if(open_book(r,source,root,math,progress,landscape)) return -1;
    r->dictionary=dictionary;
    if(epub) {
        if(load_document(r,document))return -1;
        if(reach_anchor(r,anchor))return -1;
    }
    /* Keep the source position, not a page number from the previous layout. */
    uint64_t first=1,last=r->stats.pages;
    while(first<=last) {
        uint64_t page=first+(last-first)/2;
        if(seek_record(r->pages,page-1,32)||fread(record,1,32,r->pages)!=32)
            return error(r,"Cannot locate reading position");
        if(number(record+16,8)>anchor) last=page-1;
        else { r->page=(unsigned)page; first=page+1; }
    }
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
        if(direction>0&&r->chapter_first+10<=r->stats.chapters) r->chapter_first+=10;
        if(direction<0&&r->chapter_first>10) r->chapter_first-=10;
    } else if(r->view==INK_READER_PAGE) {
        if(r->epub) {
            if(!r->layout&&load_document(r,r->document))return true;
            if(direction>0&&r->page>=r->stats.pages) {
                if(extend(r,r->page+1))return true;
                if(r->page>=r->stats.pages&&r->eof&&r->document<r->documents) {
                    if(load_document(r,r->document+1))return true;
                    while(!r->page&&r->document<r->documents)if(load_document(r,r->document+1))return true;
                    resume_save(r);
    return true;
                }
            }
            if(direction<0&&r->page<=1&&r->document>1) {
                if(load_document(r,r->document-1))return true;
                /* Returning to an unseen previous document must find its end. */
                while(!r->eof)if(extend(r,(unsigned)r->stats.pages+1))return true;
                r->page=(unsigned)r->stats.pages;resume_save(r);
    return true;
            }
        }
        if(direction>0&&r->page<r->stats.pages) r->page++;
        if(direction<0&&r->page>1) r->page--;
        resume_save(r);
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
        unsigned level=(style>>8)&7,height=INK_UI_FONT+(level?2*(7-level):0);
        if(size>end-at||!cell||cell>page_width(r)) return error(r,"Invalid word geometry");
        bool continuation=(x==prev_end&&y==prev_y)||(x==8&&prev_end>=page_width(r)-8-prev_cell&&y==prev_y+prev_height+8);
        if(used&&!continuation) { if(hit) goto found; used=0; overflow=false; }
        if(style==INK_BITMAP||style==INK_RULE||(style&INK_MATH)) {
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
int ink_reader_tap(ink_reader *r,unsigned x,unsigned y)
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
    } else if(r->view==INK_READER_MENU&&x>=8&&x<472&&y>=180&&y<500) {
        unsigned row=(y-180)/64;
        if(r->epub) {
            if(row==1)return INK_READER_ROTATE;
            if(row==2)return INK_READER_CLOSE;
            if(row>=3){r->view=INK_READER_PAGE;
    return INK_READER_STAY;}
            r->view=INK_READER_CHAPTERS;r->chapter_first=1;
    return INK_READER_STAY;
        }
        if(row==3) return INK_READER_CLOSE;
        if(row==2) return INK_READER_ROTATE;
        if(row==4) {r->view=INK_READER_PAGE;
    return INK_READER_STAY;}
        r->view=row?INK_READER_GOTO:INK_READER_CHAPTERS; r->digits[0]=0; r->chapter_first=1;
    } else if(r->view==INK_READER_CHAPTERS&&x>=8&&x<472&&y>=104&&y<744) {
        unsigned id=r->chapter_first+(y-104)/64,page; char title[INK_TITLE_BYTES];
        if(id<=r->stats.chapters&&!chapter(r,id,title,&page)) {
            if(r->epub) {
                ink_epub_chapter c;uint64_t anchor=0;
                if(ink_epub_get_chapter(r->book_cache,id,&c)||!c.document)return error(r,"Chapter target missing from spine");
                if(load_document(r,c.document))return -1;
                if(c.fragment[0]&&ink_epub_anchor(r->cache,id,&anchor)) {
                    anchor=0;
                }else ink_epub_anchor(r->cache,id,&anchor);
                if(reach_anchor(r,anchor))return -1;
                unsigned char record[32];r->page=1;
                for(unsigned i=1;i<=r->stats.pages;i++) {
                    if(seek_record(r->pages,i-1,32)||fread(record,1,32,r->pages)!=32)break;
                    r->page=i;
                    if(number(record+16,8)>=anchor)break;
                }
                resume_save(r);
            }else r->page=page;
            r->view=INK_READER_PAGE;
        }
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
    bool landscape=drawing&&drawing->view==INK_READER_PAGE&&drawing->landscape;
    if(x>=(landscape?800u:480u)||y>=(landscape?480u:800u)) return;
    unsigned dx=landscape?799-x:799-y,dy=landscape?479-y:x;
    f[dy*100+dx/8]&=(uint8_t)~(0x80>>(dx%8));
}
static void text(uint8_t *f,unsigned x,unsigned y,const char *s,unsigned cell,unsigned height,unsigned style)
{
#ifdef INK_USE_FONTS
    ink_font_text(f,x,y,s,cell,height,style,drawing->view==INK_READER_PAGE?page_width(drawing)-8:464,pixel); return;
#endif
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
        x+=cell; if(x>=page_width(drawing)-8) break;
    }
}
static void label(uint8_t *f,unsigned x,unsigned y,const char *s)
{ text(f,x,y,s,INK_UI_CELL,INK_UI_FONT,0); }
static void box(uint8_t *f,unsigned x,unsigned y,unsigned w,unsigned h)
{
    for(unsigned i=0;i<w;i++) { pixel(f,x+i,y); pixel(f,x+i,y+h-1); }
    for(unsigned i=0;i<h;i++) { pixel(f,x,y+i); pixel(f,x+w-1,y+i); }
}
int ink_reader_draw(ink_reader *r,uint8_t frame[48000])
{
    drawing=r;
    memset(frame,255,48000);
    if(r->view==INK_READER_PAGE&&r->epub&&r->error[0]){ink_icon(frame,224,352,ICON_ERROR,pixel);return 0;}
    if(r->page&&r->view==INK_READER_PAGE) {
        unsigned char p[32]; if(page_record(r,p)) return -1;
        uint64_t begin=number(p,8),end=number(p+8,8);
        if(number(p+28,4)||begin>end||end>LONG_MAX||fseek(r->draw,(long)begin,SEEK_SET)) return error(r,"Invalid page offsets");
        for(uint64_t at=begin;at<end;) {
            unsigned char h[20];
            if(end-at<20||fread(h,1,20,r->draw)!=20) return error(r,"Truncated page run");
            at+=20;
            unsigned x=(unsigned)number(h,2),y=(unsigned)number(h+2,2),cell=(unsigned)number(h+4,2),style=(unsigned)number(h+6,2);
            uint64_t size=number(h+8,4);
            if(!size||size>end-at||x>=page_width(r)||y>=page_height(r)||!cell) return error(r,"Invalid page run");
            if(style==INK_RULE) {
                unsigned char b[2];
                if(size!=2||fread(b,1,2,r->draw)!=2) return error(r,"Invalid table rule");
                unsigned height=(unsigned)number(b,2);
                if(!height||height>page_height(r)-y||cell>page_width(r)-x) return error(r,"Table rule outside page");
                for(unsigned dy=0;dy<height;dy++) for(unsigned dx=0;dx<cell;dx++) pixel(frame,x+dx,y+dy);
            } else if(style==INK_BITMAP) {
                unsigned char b[2],row[100];
                if(size<2||cell>800||fread(b,1,2,r->draw)!=2) return error(r,"Invalid formula bitmap");
                unsigned height=(unsigned)number(b,2),stride=(cell+7)/8;
                if(!height||height>page_height(r)-y||cell>page_width(r)-x||size!=2+(uint64_t)stride*height) return error(r,"Formula outside page");
                for(unsigned dy=0;dy<height;dy++) {
                    if(fread(row,1,stride,r->draw)!=stride) return error(r,"Truncated formula bitmap");
                    for(unsigned dx=0;dx<cell;dx++) if(row[dx/8]&(0x80>>(dx%8))) pixel(frame,x+dx,y+dy);
                }
            } else {
                char payload[513];
                if(size>512||cell>100||fread(payload,1,(size_t)size,r->draw)!=size) return error(r,"Invalid text run");
                payload[size]=0; unsigned level=(style>>8)&7,height=INK_UI_FONT+(level?2*(7-level):0);
                text(frame,x,y,payload,cell,height,style|(level?INK_BOLD:0));
            }
            at+=size;
        }
    }
    if(r->view!=INK_READER_PAGE) {
        char line[64];
        if(r->page) { unsigned char p[32]; if(page_record(r,p)) return -1; }
        if(r->view==INK_READER_DEFINITION||r->view==INK_READER_DICTIONARIES) {
            ink_dict *d=r->dictionary; label(frame,16,16,r->word);
            if(r->view==INK_READER_DICTIONARIES) {
                ink_icon(frame,224,64,ICON_DICT,pixel);
                if(!d||!d->total) ink_icon(frame,224,120,ICON_ERROR,pixel);
                if(d) for(unsigned i=0;i<d->count;i++) { box(frame,16,120+i*64,448,64); label(frame,24,136+i*64,d->rows[i].name); }
            } else {
                if(d&&d->name[0]) label(frame,16,64,d->name);
                if(r->error[0]||(d&&d->error[0])) ink_icon(frame,224,144,ICON_ERROR,pixel);
                else if(d) for(unsigned row=0;row<INK_DICT_LINES;row++) label(frame,16,144+row*32,d->lines[row]);
                if(d&&d->pages&&!r->error[0]&&!d->error[0]) { snprintf(line,sizeof(line),"%u / %u",d->page+1,d->pages); label(frame,16,664,line); }
                box(frame,16,704,312,64); ink_icon(frame,152,720,ICON_DICT,pixel);
                box(frame,328,704,136,64); ink_icon(frame,380,720,ICON_CLOSE,pixel);
            }
            return 0;
        }
        if(!r->epub){snprintf(line,sizeof(line),"%u / %u",r->page,(unsigned)r->stats.pages);label(frame,16,16,line);}
        label(frame,16,64,r->title);
        if(r->view==INK_READER_MENU) {
            const unsigned icons[]={ICON_CHAPTER,ICON_PAGE,ICON_ROTATE,ICON_EXIT,ICON_BACK};
            const unsigned epub_icons[]={ICON_CHAPTER,ICON_ROTATE,ICON_EXIT,ICON_BACK};
            unsigned count=r->epub?4:5;
            for(unsigned i=0;i<count;i++){box(frame,8,180+i*64,464,64);ink_icon(frame,224,196+i*64,r->epub?epub_icons[i]:icons[i],pixel);}
        } else if(r->view==INK_READER_CHAPTERS) {
            if(!r->stats.chapters) ink_icon(frame,224,120,ICON_ERROR,pixel);
            for(unsigned i=0;i<10&&r->chapter_first+i<=r->stats.chapters;i++) {
                char title[INK_TITLE_BYTES]; unsigned page;
                if(chapter(r,r->chapter_first+i,title,&page)) return -1;
                box(frame,8,104+i*64,464,64); label(frame,16,122+i*64,title);
            }
        } else {
            label(frame,32,140,r->digits);
            for(unsigned row=0;row<4;row++) for(unsigned col=0;col<3;col++) {
                char key[8];
                if(row<3) snprintf(key,sizeof(key),"%u",1+row*3+col);
                else snprintf(key,sizeof(key),"%s",col==0?"0":"");
                box(frame,32+col*138,220+row*80,138,80);
                if(row==3&&col) ink_icon(frame,80+col*138,244+row*80,col==1?ICON_DELETE:ICON_PLAY,pixel);
                else label(frame,72+col*138,244+row*80,key);
            }
        }
        if(r->error[0]) ink_icon(frame,224,680,ICON_ERROR,pixel);
    }
    return 0;
}
