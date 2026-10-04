#define _POSIX_C_SOURCE 200809L
#include "ink_layout.h"
#include "md_budget.h"
#include "md4c.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>

enum { CELLS=128, MARGIN=8, TABLE_COLUMNS=16, TABLE_PAD=4 };
typedef struct { uint32_t value,min; unsigned need; uint64_t start; } Utf;
typedef struct { uint32_t cp; uint16_t style; uint64_t source, bitmap_offset; unsigned width,height,baseline; bool display; } Cell;
typedef struct {
    FILE *input,*draw,*pages,*chapters,*bitmaps,*table;
    ink_layout_config cfg;
    ink_layout_stats *stats;
    char block[INK_BLOCK_BYTES], physical[INK_BLOCK_BYTES];
    unsigned char read[1024];
    size_t block_n, read_n, read_at;
    uint64_t position,block_source,last_source,draw_bytes,page_begin,page_source;
    Utf validation,literal_utf;
    bool failed,literal,previous_cr,page_open,done;
    unsigned fence_char; uint64_t fence_length;
    unsigned y,head,bold,italic,code,math,image,chapter,page_chapter;
    Cell line[CELLS]; unsigned count;
    bool title_active,title_cut,title_located,image_rendered,next_chapter;
    uint64_t chapter_source;
    unsigned char chapter_record[202];
    unsigned title_n,title_page;
    uint64_t title_source;
    char title[INK_TITLE_BYTES];
    unsigned list_depth,list_order[16];
    char formula[4096]; unsigned formula_n; bool formula_cut,display_math;
    uint64_t formula_source;
    unsigned table_columns,table_column,table_above,table_below;
    bool table_active,table_collect,table_header,table_drawing;
    unsigned table_align[TABLE_COLUMNS];
    uint64_t table_write;
    uint64_t table_begin[TABLE_COLUMNS],table_end[TABLE_COLUMNS];
    uint8_t bitmap[48000];
} Layout;

static int fail(Layout *l,const char *why,uint64_t offset)
{
    if (!l->failed) { snprintf(l->stats->error,sizeof(l->stats->error),"%s",why); l->stats->error_offset=offset; }
    l->failed=true; return -1;
}
/* Decoder carries state over every I/O or literal-buffer boundary. */
static int utf(Utf *u,unsigned char b,uint64_t at,uint32_t *out)
{
    if (!u->need) {
        u->start=at;
        if (b<128) { *out=b; return 1; }
        if (b>=0xc2 && b<=0xdf) { u->value=b&31; u->min=0x80; u->need=1; }
        else if (b>=0xe0 && b<=0xef) { u->value=b&15; u->min=0x800; u->need=2; }
        else if (b>=0xf0 && b<=0xf4) { u->value=b&7; u->min=0x10000; u->need=3; }
        else return -1;
        return 0;
    }
    if ((b&0xc0)!=0x80) return -1;
    u->value=(u->value<<6)|(b&63);
    if (--u->need) return 0;
    if (u->value<u->min || u->value>0x10ffff || (u->value>=0xd800 && u->value<=0xdfff)) return -1;
    *out=u->value; return 1;
}
static unsigned encode(uint32_t c,char *out)
{
    if (c<0x80) { out[0]=(char)c; return 1; }
    if (c<0x800) { out[0]=(char)(0xc0|(c>>6)); out[1]=(char)(0x80|(c&63)); return 2; }
    if (c<0x10000) { out[0]=(char)(0xe0|(c>>12)); out[1]=(char)(0x80|((c>>6)&63)); out[2]=(char)(0x80|(c&63)); return 3; }
    out[0]=(char)(0xf0|(c>>18)); out[1]=(char)(0x80|((c>>12)&63)); out[2]=(char)(0x80|((c>>6)&63)); out[3]=(char)(0x80|(c&63)); return 4;
}
static void bytes(Layout *l,FILE *f,const void *p,size_t n)
{ if (fwrite(p,1,n,f)!=n) fail(l,"cache write failed",l->position); }
static void number(Layout *l,FILE *f,uint64_t v,unsigned n)
{ unsigned char p[8]; for (unsigned i=0;i<n;++i) { p[i]=(unsigned char)v; v>>=8; } bytes(l,f,p,n); }
static unsigned pixels(const Layout *l) { return l->cfg.font_pixels+(l->head?2*(7-l->head):0); }
static unsigned cell_width(const Layout *l) { return (pixels(l)*62+99)/100; }
static unsigned row_height(const Layout *l) { return pixels(l)+8; }
static unsigned style(const Layout *l)
{ return (l->bold?INK_BOLD:0)|(l->italic?INK_ITALIC:0)|(l->code?INK_CODE:0)|(l->literal?INK_LITERAL:0)|(l->math?INK_MATH:0)|(l->image?INK_IMAGE:0)|(l->head<<8); }
static void close_page(Layout *l)
{
    if (!l->page_open) return;
    number(l,l->pages,l->page_begin,8); number(l,l->pages,l->draw_bytes,8);
    number(l,l->pages,l->page_source,8);
    /* Chapter at the page's first run is captured in the reserved line below. */
    l->stats->pages++;
    l->page_open=false; l->y=MARGIN;
}
/* Keep page chapter separate from current chapter when an H2 is mid-page. */
static void end_page(Layout *l)
{
    if (!l->page_open) return;
    unsigned chapter=l->page_chapter;
    close_page(l); number(l,l->pages,chapter,4); number(l,l->pages,0,4);
}
static void room(Layout *l)
{ if (l->y+row_height(l)>l->cfg.height-MARGIN) end_page(l); }
static void open_page(Layout *l,uint64_t source)
{
    if (l->page_open) return;
    l->page_open=true; l->page_begin=l->draw_bytes; l->page_source=source;
    l->page_chapter=l->chapter;
}
static unsigned item_width(Layout *l,const Cell *c) { return c->width?c->width:cell_width(l); }
static unsigned line_width(Layout *l)
{ unsigned n=0; for (unsigned i=0;i<l->count;++i) n+=item_width(l,&l->line[i]); return n; }
static void output_line(Layout *l,unsigned n)
{
    if (!n || l->failed) return;
    unsigned above=pixels(l),below=0;
    for (unsigned i=0;i<n;++i) if (l->line[i].width) {
        Cell *c=&l->line[i]; if(c->baseline>above) above=c->baseline;
        if(c->height-c->baseline>below) below=c->height-c->baseline;
    }
    if(l->table_drawing) { above=l->table_above; below=l->table_below; }
    unsigned height=above+below+8;
    if (l->y+height>l->cfg.height-MARGIN) end_page(l);
    open_page(l,l->line[0].source);
    if (l->head==2 && l->title_active && !l->title_located) {
        l->title_located=true; l->title_page=(unsigned)l->stats->pages+1;
        l->title_source=l->line[0].source;
    }
    unsigned step=cell_width(l),x=MARGIN;
    if(l->table_drawing) {
        unsigned width=(l->cfg.width-2*MARGIN)/l->table_columns;
        unsigned used=line_width(l),extra=width-2*TABLE_PAD-used;
        x+=l->table_column*width+TABLE_PAD;
        if(l->table_align[l->table_column]==MD_ALIGN_CENTER) x+=extra/2;
        if(l->table_align[l->table_column]==MD_ALIGN_RIGHT) x+=extra;
    }
    for (unsigned i=0;i<n;) {
        Cell *c=&l->line[i]; unsigned j=i+1,len=0; char text[CELLS*4];
        unsigned y=l->y+above-pixels(l);
        if (c->width) {
            y=l->y+above-c->baseline;
            if(c->display) x=(l->cfg.width-c->width)/2;
            len=2+((c->width+7)/8)*c->height;
        } else {
            j=i;
            while (j<n && !l->line[j].width && l->line[j].style==c->style) { len+=encode(l->line[j].cp,text+len); ++j; }
        }
        number(l,l->draw,x,2); number(l,l->draw,y,2); number(l,l->draw,c->width?c->width:step,2);
        number(l,l->draw,c->width?INK_BITMAP:c->style,2); number(l,l->draw,len,4);
        number(l,l->draw,c->source,8);
        if(c->width) {
            number(l,l->draw,c->height,2);
            if(fseeko(l->bitmaps,(off_t)c->bitmap_offset,SEEK_SET)) fail(l,"bitmap seek failed",c->source);
            for(unsigned remaining=len-2;remaining && !l->failed;) {
                unsigned chunk=remaining>sizeof(text)?sizeof(text):remaining;
                if(fread(text,1,chunk,l->bitmaps)!=chunk) { fail(l,"bitmap read failed",c->source); break; }
                bytes(l,l->draw,text,chunk); remaining-=chunk;
            }
        } else bytes(l,l->draw,text,len);
        l->draw_bytes+=20+len; l->stats->runs++; x+=c->width?c->width:(j-i)*step; i=j;
    }
    l->y+=height;
}
static void finish_line(Layout *l)
{ output_line(l,l->count); l->count=0; }
static void paragraph(Layout *l)
{ finish_line(l); if (l->page_open) l->y+=6; }
static void table_store(Layout *l,const Cell *cell)
{
    bytes(l,l->table,cell,sizeof(*cell));l->table_write+=sizeof(*cell);
}
static void emit(Layout *l,uint32_t cp,uint64_t source,bool preserve)
{
    if (l->failed) return;
    if (source>l->last_source) l->last_source=source;
    if(l->table_collect) {
        if(cp=='\r'||cp=='\n'||cp=='\t') cp=' ';
        Cell c={.cp=cp,.style=(uint16_t)style(l),.source=source};
        table_store(l,&c); return;
    }
    if (cp=='\r') { finish_line(l); l->previous_cr=true; return; }
    if (cp=='\n') { if (!l->previous_cr) finish_line(l); l->previous_cr=false; return; }
    l->previous_cr=false;
    if (cp=='\t') cp=' ';
    if (!preserve && cp==' ' && (!l->count || l->line[l->count-1].cp==' ')) return;
    if (l->title_active) {
        char b[4]; unsigned n=encode(cp,b);
        if (!l->title_cut && l->title_n+n<INK_TITLE_BYTES) { memcpy(l->title+l->title_n,b,n); l->title_n+=n; }
        else l->title_cut=true;
    }
    unsigned max=l->cfg.width-2*MARGIN;
    if (line_width(l)+cell_width(l)>max || l->count==CELLS) {
        unsigned cut=l->count;
        if (!preserve) {
            for (unsigned i=l->count;i>0;--i) if (l->line[i-1].cp==' ') { cut=i; break; }
        }
        output_line(l,cut);
        memmove(l->line,l->line+cut,(l->count-cut)*sizeof(Cell)); l->count-=cut;
        if (!preserve && cp==' ' && !l->count) return;
    }
    l->line[l->count++]=(Cell){.cp=cp,.style=(uint16_t)style(l),.source=source};
}
static void literal_text(Layout *l,const char *s,size_t n,uint64_t source)
{
    for (size_t i=0;i<n;++i) {
        uint32_t cp; int r=utf(&l->literal_utf,(unsigned char)s[i],source+i,&cp);
        if (r<0) { fail(l,"invalid UTF-8",source+i); return; }
        if (r>0) emit(l,cp,l->literal_utf.start,true);
    }
}
static void synthetic(Layout *l,const char *s)
{ for (;*s;++s) emit(l,(unsigned char)*s,l->last_source,false); }
/* A row's cells use a reusable SD scratch file. Read one wrapped line at
 * a time, twice (measure then draw), so neither table size nor cell length sets
 * RAM use. Page breaks happen between line bands, even inside a tall row. */
static uint64_t table_line(Layout *l,unsigned col,uint64_t at)
{
    l->count=0;
    unsigned width=(l->cfg.width-2*MARGIN)/l->table_columns-2*TABLE_PAD,used=0,word_cut=0;
    uint64_t word_at=at;
    if(fseeko(l->table,(off_t)at,SEEK_SET)) { fail(l,"table read seek failed",l->last_source); return at; }
    while(at<l->table_end[col]&&l->count<CELLS) {
        Cell c;
        if(fread(&c,1,sizeof(c),l->table)!=sizeof(c)) { fail(l,"table read failed",l->last_source); break; }
        unsigned w=item_width(l,&c);
        if(used+w>width) {
            if(word_cut) { l->count=word_cut;at=word_at; }
            break;
        }
        l->line[l->count++]=c;used+=w;at+=sizeof(c);
        if(c.cp==' '&&!(c.style&INK_CODE)) { word_cut=l->count;word_at=at; }
    }
    while(l->count&&l->line[l->count-1].cp==' ') l->count--;
    return at;
}
static void rule(Layout *l,unsigned x,unsigned y,unsigned w,unsigned h)
{
    number(l,l->draw,x,2);number(l,l->draw,y,2);number(l,l->draw,w,2);
    number(l,l->draw,INK_RULE,2);number(l,l->draw,2,4);number(l,l->draw,l->last_source,8);
    number(l,l->draw,h,2);l->draw_bytes+=22;l->stats->runs++;
}
static void table_row(Layout *l)
{
    uint64_t at[TABLE_COLUMNS];memcpy(at,l->table_begin,sizeof(at));
    bool first=true,more=true;
    while(more&&!l->failed) {
        if(l->cfg.progress) l->cfg.progress();
        more=false;l->table_above=pixels(l);l->table_below=0;
        for(unsigned col=0;col<l->table_columns;col++) {
            table_line(l,col,at[col]);
            for(unsigned i=0;i<l->count;i++) if(l->line[i].width) {
                Cell *c=&l->line[i];
                if(c->baseline>l->table_above) l->table_above=c->baseline;
                if(c->height-c->baseline>l->table_below) l->table_below=c->height-c->baseline;
            }
        }
        unsigned height=l->table_above+l->table_below+8;
        if(l->y+height>l->cfg.height-MARGIN) {
            if(l->page_open&&!first) rule(l,MARGIN,l->y-1,((l->cfg.width-2*MARGIN)/l->table_columns)*l->table_columns+1,1);
            end_page(l);first=true;
        }
        unsigned top=l->y;
        l->table_drawing=true;
        for(unsigned col=0;col<l->table_columns;col++) {
            l->table_column=col;
            uint64_t next=table_line(l,col,at[col]);
            if(next==at[col]&&at[col]<l->table_end[col]) { fail(l,"table cell exceeds column width",l->last_source);break; }
            at[col]=next;more|=next<l->table_end[col];
            l->y=top;finish_line(l);
        }
        l->table_drawing=false;
        open_page(l,l->last_source);l->y=top+height;
        unsigned width=(l->cfg.width-2*MARGIN)/l->table_columns;
        if(first) rule(l,MARGIN,top,width*l->table_columns+1,1);
        for(unsigned col=0;col<=l->table_columns;col++) rule(l,MARGIN+col*width,top,1,height);
        if(!more) rule(l,MARGIN,top+height-1,width*l->table_columns+1,1);
        first=false;
    }
    l->count=0;
}
static int block_enter(MD_BLOCKTYPE t,void *detail,void *user)
{
    Layout *l=user;
    if(t==MD_BLOCK_TABLE) {
        paragraph(l);l->table_columns=((MD_BLOCK_TABLE_DETAIL*)detail)->col_count;
        l->table_active=l->table_columns&&l->table_columns<=TABLE_COLUMNS&&
            (l->cfg.width-2*MARGIN)/l->table_columns>=cell_width(l)+2*TABLE_PAD;
        if(l->table_active&&!l->table) l->table=l->cfg.table_spool?l->cfg.table_spool:tmpfile();
        if(l->table_active&&!l->table) return fail(l,"table spool creation failed",l->block_source);
    }
    if(t==MD_BLOCK_TR&&l->table_active) {
        l->table_column=0;l->table_write=0;
        if(fseeko(l->table,0,SEEK_SET)) return fail(l,"table row seek failed",l->last_source);
    }
    if((t==MD_BLOCK_TD||t==MD_BLOCK_TH)&&l->table_active) {
        l->table_collect=true;l->table_header=t==MD_BLOCK_TH;
        l->table_align[l->table_column]=((MD_BLOCK_TD_DETAIL*)detail)->align;
        if(l->table_header) l->bold++;
        l->table_begin[l->table_column]=l->table_write;
    }
    if (t==MD_BLOCK_H || t==MD_BLOCK_CODE || t==MD_BLOCK_LI) paragraph(l);
    if (t==MD_BLOCK_H) {
        l->head=((MD_BLOCK_H_DETAIL*)detail)->level;
        if (l->head==2 && !l->cfg.chapter_spool) {
            /* Fixed chapter behavior: an H2 begins a new page. */
            end_page(l);
            ++l->chapter; l->title_active=true; l->title_cut=false; l->title_located=false;
            l->title_n=0; memset(l->title,0,sizeof(l->title));
        }
    }
    if (t==MD_BLOCK_CODE) ++l->code;
    if (t==MD_BLOCK_OL || t==MD_BLOCK_UL) {
        if (l->list_depth==16) return fail(l,"list nesting exceeds prototype budget",l->block_source);
        l->list_order[l->list_depth++]=t==MD_BLOCK_OL?((MD_BLOCK_OL_DETAIL*)detail)->start:0;
    }
    if (t==MD_BLOCK_LI) {
        if (l->list_depth && l->list_order[l->list_depth-1]) {
            char b[32]; snprintf(b,sizeof(b),"%u. ",l->list_order[l->list_depth-1]++); synthetic(l,b);
        } else synthetic(l,"- ");
    }
    if (t==MD_BLOCK_HR) { paragraph(l); synthetic(l,"----------------"); paragraph(l); }
    return l->failed?-1:0;
}
static int block_leave(MD_BLOCKTYPE t,void *detail,void *user)
{
    (void)detail; Layout *l=user;
    if((t==MD_BLOCK_TD||t==MD_BLOCK_TH)&&l->table_active) {
        l->table_end[l->table_column++]=l->table_write;
        if(l->table_header) l->bold--;
        l->table_collect=false;return l->failed?-1:0;
    }
    if(t==MD_BLOCK_TR&&l->table_active) { table_row(l);return l->failed?-1:0; }
    if(t==MD_BLOCK_TABLE) { l->table_active=false;paragraph(l); }
    if (t==MD_BLOCK_P || t==MD_BLOCK_H || t==MD_BLOCK_CODE || t==MD_BLOCK_LI || t==MD_BLOCK_TR) paragraph(l);
    if (t==MD_BLOCK_H) {
        if (l->head==2 && !l->cfg.chapter_spool) {
            l->title_active=false;
            if (!l->title_located) { room(l); open_page(l,l->block_source); l->title_page=(unsigned)l->stats->pages+1; l->title_source=l->block_source; }
            number(l,l->chapters,l->title_source,8); number(l,l->chapters,l->title_page,4);
            number(l,l->chapters,l->chapter,4); number(l,l->chapters,l->title_n,2);
            number(l,l->chapters,l->title_cut,2); bytes(l,l->chapters,l->title,INK_TITLE_BYTES);
            l->stats->chapters++;
        }
        l->head=0;
    }
    if (t==MD_BLOCK_CODE) --l->code;
    if (t==MD_BLOCK_OL || t==MD_BLOCK_UL) --l->list_depth;
    if (t==MD_BLOCK_TD || t==MD_BLOCK_TH) synthetic(l," | ");
    return l->failed?-1:0;
}
static void formula(Layout *l)
{
    unsigned w=0,h=0,base=0;
    l->formula[l->formula_n]=0;
    int bad=l->formula_cut || l->cfg.render_math(l->formula,l->display_math,pixels(l),l->bitmap,&w,&h,&base);
    unsigned available=l->table_collect?(l->cfg.width-2*MARGIN)/l->table_columns-2*TABLE_PAD:l->cfg.width-2*MARGIN;
    if(!bad&&w>available&&l->table_collect) {
        unsigned smaller=pixels(l)*available/w;
        if(smaller>=12) bad=l->cfg.render_math(l->formula,0,smaller,l->bitmap,&w,&h,&base);
    }
    if (bad || !w || w>480 || w>available || !h || h>800 ||
        base>h || (base>pixels(l)?base:pixels(l))+h-base+8>l->cfg.height-2*MARGIN) {
        ++l->stats->math_fallbacks;
        synthetic(l,l->display_math?"$$":"$");
        Utf state={0};
        for(unsigned i=0;i<l->formula_n;++i) { uint32_t cp; int r=utf(&state,(unsigned char)l->formula[i],l->formula_source+i,&cp); if(r>0) emit(l,cp,state.start,true); }
        if(l->formula_cut) synthetic(l,"...");
        synthetic(l,l->display_math?"$$":"$");
        return;
    }
    if(l->display_math&&!l->table_collect) paragraph(l);
    if(!l->table_collect&&(line_width(l)+w>available || l->count==CELLS)) finish_line(l);
    if(fseeko(l->bitmaps,0,SEEK_END)) { fail(l,"bitmap spool seek failed",l->formula_source); return; }
    off_t offset=ftello(l->bitmaps);
    if(offset<0) { fail(l,"bitmap spool position failed",l->formula_source); return; }
    unsigned stride=(w+7)/8;
    for(unsigned y=0;y<h;++y) bytes(l,l->bitmaps,l->bitmap+y*60,stride);
    Cell c={.source=l->formula_source,.bitmap_offset=(uint64_t)offset,
        .width=w,.height=h,.baseline=base,.display=l->display_math&&!l->table_collect};
    if(l->table_collect) table_store(l,&c); else l->line[l->count++]=c;
    ++l->stats->formulas;
    if(l->display_math&&!l->table_collect) paragraph(l);
}
static bool picture(Layout *l,const MD_ATTRIBUTE *src)
{
    if(!l->cfg.render_image||src->size>=512) return false;
    char resource[512];memcpy(resource,src->text,src->size);resource[src->size]=0;
    unsigned w=0,h=0,available=l->table_collect?(l->cfg.width-2*MARGIN)/l->table_columns-2*TABLE_PAD:l->cfg.width-2*MARGIN;
    if(l->cfg.render_image(resource,available,l->cfg.height-2*MARGIN-8,l->bitmap,&w,&h)||!w||w>available||!h||h>l->cfg.height-2*MARGIN-8) return false;
    bool display=!l->table_collect&&(w>pixels(l)*3||h>pixels(l)*2);
    if(display)paragraph(l);
    if(!l->table_collect&&(line_width(l)+w>available||l->count==CELLS))finish_line(l);
    if(fseeko(l->bitmaps,0,SEEK_END)) {fail(l,"image spool seek failed",l->last_source);return false;}
    off_t offset=ftello(l->bitmaps);if(offset<0){fail(l,"image spool offset failed",l->last_source);return false;}
    unsigned stride=(w+7)/8;bytes(l,l->bitmaps,l->bitmap,(size_t)stride*h);
    Cell c={.source=l->last_source,.bitmap_offset=(uint64_t)offset,.width=w,.height=h,.baseline=h,.display=display};
    if(l->table_collect)table_store(l,&c);else l->line[l->count++]=c;
    if(display)paragraph(l);
    return true;
}
static int span(MD_SPANTYPE t,void *user,bool enter)
{
    Layout *l=user; int delta=enter?1:-1;
    if (t==MD_SPAN_STRONG) l->bold+=delta;
    if (t==MD_SPAN_EM) l->italic+=delta;
    if (t==MD_SPAN_CODE) l->code+=delta;
    if (t==MD_SPAN_LATEXMATH || t==MD_SPAN_LATEXMATH_DISPLAY) {
        if(l->cfg.render_math) {
            if(enter) { ++l->math; l->formula_n=0; l->formula_cut=false;
                l->display_math=t==MD_SPAN_LATEXMATH_DISPLAY; l->formula_source=l->last_source; }
            else { formula(l); --l->math; }
        } else {
            if (enter) ++l->math;
            synthetic(l,t==MD_SPAN_LATEXMATH?"$":"$$");
            if (!enter) --l->math;
        }
    }
    if (t==MD_SPAN_IMG) {
        if (enter) { ++l->image; synthetic(l,"[image: "); }
        else { synthetic(l,"]"); --l->image; }
    }
    return l->failed?-1:0;
}
static int span_enter(MD_SPANTYPE t,void *d,void *u)
{
    Layout *l=u;
    if(t==MD_SPAN_IMG) {
        l->image_rendered=picture(l,&((MD_SPAN_IMG_DETAIL*)d)->src);
        if(l->image_rendered){l->image++;return l->failed?-1:0;}
    }
    return span(t,u,true);
}
static int span_leave(MD_SPANTYPE t,void *d,void *u)
{
    (void)d;Layout *l=u;
    if(t==MD_SPAN_IMG&&l->image_rendered){l->image--;l->image_rendered=false;return l->failed?-1:0;}
    return span(t,u,false);
}
static uint32_t entity(const char *s,size_t n)
{
    const struct { const char *s; uint32_t cp; } names[]={{"&amp;",38},{"&lt;",60},{"&gt;",62},{"&quot;",34},{"&apos;",39},{"&nbsp;",160}};
    for (size_t i=0;i<sizeof(names)/sizeof(names[0]);++i) if (strlen(names[i].s)==n && !memcmp(s,names[i].s,n)) return names[i].cp;
    if (n>=4 && s[0]=='&' && s[1]=='#' && s[n-1]==';') {
        size_t i=2; unsigned base=10; uint32_t value=0;
        if (s[i]=='x'||s[i]=='X') { base=16; ++i; }
        if (i==n-1) return 0;
        for (;i<n-1;++i) {
            unsigned digit=(s[i]>='0'&&s[i]<='9')?(unsigned)(s[i]-'0'):
                (s[i]>='a'&&s[i]<='f')?(unsigned)(s[i]-'a'+10):
                (s[i]>='A'&&s[i]<='F')?(unsigned)(s[i]-'A'+10):99;
            if (digit>=base || value>(0x10ffff-digit)/base) return 0;
            value=value*base+digit;
        }
        if (value>=32 && value<=0x10ffff && !(value>=0xd800 && value<=0xdfff)) return value;
    }
    return 0;
}
static int text(MD_TEXTTYPE t,const MD_CHAR *s,MD_SIZE n,void *user)
{
    Layout *l=user; if(l->image_rendered)return 0; uint64_t at=l->last_source;
    uintptr_t p=(uintptr_t)s,base=(uintptr_t)l->block;
    if (p>=base && p-base<l->block_n) at=l->block_source+(p-base);
    if(l->math && l->cfg.render_math) {
        if(!l->formula_n) l->formula_source=at;
        for(unsigned i=0;i<n;++i) {
            if(l->formula_n+1<sizeof(l->formula)) l->formula[l->formula_n++]=s[i];
            else l->formula_cut=true;
        }
        if(at+n>l->last_source) l->last_source=at+n;
        return 0;
    }
    bool preserve=l->code || l->math;
    if (t==MD_TEXT_SOFTBR) { emit(l,' ',at,false); return l->failed?-1:0; }
    if (t==MD_TEXT_BR) { finish_line(l); return l->failed?-1:0; }
    if (t==MD_TEXT_ENTITY) { uint32_t cp=entity(s,n); if (cp) { emit(l,cp,at,preserve); return l->failed?-1:0; } }
    Utf state={0};
    for (unsigned i=0;i<n;++i) {
        uint32_t cp; int r=utf(&state,(unsigned char)s[i],at+i,&cp);
        if (r<0) return fail(l,"invalid callback UTF-8",at+i);
        if (r) emit(l,cp,state.start,preserve);
    }
    if (state.need) return fail(l,"incomplete callback UTF-8",state.start);
    return l->failed?-1:0;
}
static void parse(Layout *l)
{
    if (!l->block_n || l->failed) return;
    MD_PARSER parser={0}; parser.flags=MD_FLAG_TABLES|MD_FLAG_LATEXMATHSPANS|MD_FLAG_NOHTML;
    parser.enter_block=block_enter; parser.leave_block=block_leave;
    parser.enter_span=span_enter; parser.leave_span=span_leave; parser.text=text;
    if (md_parse(l->block,(MD_SIZE)l->block_n,&parser,l)!=0 && !l->failed)
        fail(l,"Markdown parser complexity/allocation budget exceeded",l->block_source);
    l->block_n=0;
}
static void start_literal(Layout *l)
{
    if (l->literal) return;
    paragraph(l); l->literal=true; l->stats->literal_blocks++;
    literal_text(l,l->block,l->block_n,l->block_source); l->block_n=0;
}
static int peek(Layout *l)
{
    if (l->read_at==l->read_n) {
        if(l->cfg.progress) l->cfg.progress();
        if(l->cfg.cancelled&&l->cfg.cancelled()){fail(l,"Reader cancelled",l->position);return EOF;}
        l->read_n=fread(l->read,1,l->cfg.read_bytes,l->input); l->read_at=0;
        if (!l->read_n) { if (ferror(l->input)) fail(l,"source read failed",l->position); return EOF; }
    }
    return l->read[l->read_at];
}
static int get(Layout *l)
{
    int c=peek(l); if (c==EOF) return EOF;
    ++l->read_at; uint32_t cp; int r=utf(&l->validation,(unsigned char)c,l->position,&cp);
    if (r<0) fail(l,"invalid UTF-8",l->validation.start);
    if (r>0 && (cp==0 || (cp<32 && cp!='\n' && cp!='\r' && cp!='\t') || cp==127)) fail(l,"NUL/control byte in text",l->validation.start);
    ++l->position; return c;
}
typedef struct { bool blank,tail_space,tail_tick,in_run; unsigned indent,mark; uint64_t run; } Line;
static void classify(Line *m,int c)
{
    if (c=='\r'||c=='\n') return;
    if (c!=' '&&c!='\t') m->blank=false;
    if (!m->mark) {
        if (c==' ' && m->indent<4) { ++m->indent; return; }
        if (m->indent<=3 && (c=='`'||c=='~')) { m->mark=(unsigned)c; m->run=1; m->in_run=true; return; }
        m->mark=1;
    } else if (m->in_run && c==(int)m->mark) { ++m->run; return; }
    m->in_run=false;
    if (c!=' '&&c!='\t') m->tail_space=false;
    if (c=='`') m->tail_tick=true;
}
static void next_chapter(Layout *l)
{
    size_t n=fread(l->chapter_record,1,sizeof(l->chapter_record),l->cfg.chapter_spool);
    l->next_chapter=n==sizeof(l->chapter_record);l->chapter_source=0;
    if(n&&!l->next_chapter)fail(l,"Invalid EPUB chapter anchor",l->position);
    if(l->next_chapter)for(unsigned i=0;i<8;i++)l->chapter_source|=(uint64_t)l->chapter_record[i]<<(i*8);
}
static void book_chapter(Layout *l)
{
    parse(l);paragraph(l);end_page(l);l->chapter++;
    unsigned n=l->chapter_record[8]|(unsigned)l->chapter_record[9]<<8;
    if(n>=INK_TITLE_BYTES){fail(l,"Invalid EPUB chapter title",l->position);return;}
    open_page(l,l->position);
    number(l,l->chapters,l->position,8);number(l,l->chapters,l->stats->pages+1,4);
    number(l,l->chapters,l->chapter,4);number(l,l->chapters,n,2);number(l,l->chapters,0,2);
    bytes(l,l->chapters,l->chapter_record+10,INK_TITLE_BYTES);l->stats->chapters++;
    next_chapter(l);
}
static void scan(Layout *l,uint64_t target)
{
    while (!l->failed && peek(l)!=EOF) {
        if(l->next_chapter&&l->position==l->chapter_source)book_chapter(l);
        uint64_t first=l->position; size_t n=0; bool long_line=false;
        Line m={.blank=true,.tail_space=true};
        for (;;) {
            int c=get(l); if (c==EOF || l->failed) break;
            classify(&m,c);
            if (!long_line && n==INK_BLOCK_BYTES) {
                start_literal(l); literal_text(l,l->physical,n,first); n=0; long_line=true;
            }
            if (long_line) { char b=(char)c; literal_text(l,&b,1,l->position-1); }
            else l->physical[n++]=(char)c;
            if (c=='\r' && peek(l)=='\n') {
                c=get(l);
                if (!long_line && n==INK_BLOCK_BYTES) { start_literal(l); literal_text(l,l->physical,n,first); n=0; long_line=true; }
                if (long_line) { char b='\n'; literal_text(l,&b,1,l->position-1); }
                else l->physical[n++]='\n';
            }
            if (c=='\n'||c=='\r') break;
        }
        if (l->failed) break;
        if (!long_line) {
            if (!l->literal && l->block_n+n>INK_BLOCK_BYTES) start_literal(l);
            if (l->literal) literal_text(l,l->physical,n,first);
            else { if (!l->block_n) l->block_source=first; memcpy(l->block+l->block_n,l->physical,n); l->block_n+=n; }
        }
        bool closed=false;
        if (l->fence_char) {
            if (m.mark==l->fence_char && m.run>=l->fence_length && m.tail_space) { l->fence_char=0; closed=true; }
        } else if (m.run>=3 && (m.mark=='~'||(m.mark=='`'&&!m.tail_tick))) {
            l->fence_char=m.mark; l->fence_length=m.run;
        }
        if ((!l->fence_char && m.blank)||closed) {
            if (l->literal) { paragraph(l); l->literal=false; }
            else parse(l);
        }
        if(target && l->stats->pages>=target) return;
    }
    l->done=true;
    if (l->validation.need) fail(l,"incomplete UTF-8 at EOF",l->validation.start);
    if (l->literal) paragraph(l); else parse(l);
    finish_line(l); end_page(l);
}
void *ink_layout_begin(FILE *source,FILE *draw,FILE *pages,FILE *chapters,
                   const ink_layout_config *config,ink_layout_stats *stats)
{
    memset(stats,0,sizeof(*stats));
    Layout *l=calloc(1,sizeof(*l));
    if (!l) { snprintf(stats->error,sizeof(stats->error),"layout allocation failed"); return NULL; }
    l->input=source; l->draw=draw; l->pages=pages; l->chapters=chapters;
    l->cfg=*config; l->stats=stats; l->y=MARGIN; stats->context_bytes=sizeof(*l);
    if (config->width<120 || config->width>800 || config->height<100 || config->height>800 ||
        config->font_pixels<16 || config->font_pixels>28 || !config->read_bytes || config->read_bytes>sizeof(l->read)) fail(l,"invalid prototype geometry/read size",0);
    if (ink_md_reset()) fail(l,"parser allocator still in use",0);
    unsigned char bom[3]; size_t n=fread(bom,1,3,source);
    l->position=n==3 && !memcmp(bom,"\xef\xbb\xbf",3)?3:0;
    if (fseeko(source,(off_t)l->position,SEEK_SET)) fail(l,"source must be seekable",0);
    if((config->render_math||config->render_image) && !(l->bitmaps=config->bitmap_spool?config->bitmap_spool:tmpfile())) fail(l,"bitmap spool creation failed",0);
    if(config->chapter_spool)next_chapter(l);

    if(l->failed) { ink_layout_end(l); return NULL; }
    return l;
}
void ink_layout_end(void *context)
{
    Layout *l=context;if(!l)return;
    if(l->bitmaps&&!l->cfg.bitmap_spool) fclose(l->bitmaps);
    if(l->table&&!l->cfg.table_spool) fclose(l->table);
    free(l);
}
int ink_layout_step(void *context)
{
    Layout *l=context;
    if(!l->failed&&!l->done) scan(l,l->stats->pages+1);
    if(fflush(l->draw)||fflush(l->pages)||fflush(l->chapters)||
       (l->bitmaps&&fflush(l->bitmaps))||(l->table&&fflush(l->table)))
        fail(l,"cache flush failed",l->position);
    l->stats->source_bytes=l->position;l->stats->parser_peak_bytes=ink_md_peak();
    return l->failed?-1:l->done?1:0;
}
/* Only the pointer-free scanner state is persisted, and only between parser
 * calls. Header includes ABI size; book cache key includes firmware/layout version.
 * Caller supplies fresh streams/callbacks on restore, never restored addresses. */
int ink_layout_save(void *context,FILE *state)
{
    Layout *l=context;
    uint64_t h[4]={0x383350594b4e49ULL,sizeof(*l),
        l->cfg.chapter_spool?(uint64_t)ftello(l->cfg.chapter_spool):0,l->table!=NULL};
    size_t offset=offsetof(Layout,block),size=offsetof(Layout,bitmap)-offset;
    if(l->failed||fwrite(h,1,sizeof(h),state)!=sizeof(h)||
       fwrite(l->stats,1,sizeof(*l->stats),state)!=sizeof(*l->stats)||
       fwrite(l->block,1,size,state)!=size||fflush(state))return -1;
    return 0;
}
void *ink_layout_restore(FILE *state,FILE *source,FILE *draw,FILE *pages,FILE *chapters,
    const ink_layout_config *config,ink_layout_stats *stats)
{
    uint64_t h[4];
    if(fread(h,1,sizeof(h),state)!=sizeof(h)||h[0]!=0x383350594b4e49ULL||h[1]!=sizeof(Layout))return NULL;
    Layout *l=calloc(1,sizeof(*l));if(!l)return NULL;
    size_t offset=offsetof(Layout,block),size=offsetof(Layout,bitmap)-offset;
    if(fread(stats,1,sizeof(*stats),state)!=sizeof(*stats)||
       fread(l->block,1,size,state)!=size)goto bad;
    l->input=source;l->draw=draw;l->pages=pages;l->chapters=chapters;l->stats=stats;l->cfg=*config;
    l->bitmaps=config->bitmap_spool;l->table=h[3]?config->table_spool:NULL;
    if(l->read_n>sizeof(l->read)||l->read_at>l->read_n||l->count>CELLS||l->block_n>INK_BLOCK_BYTES||l->formula_n>sizeof(l->formula)||l->failed||
       fseeko(source,(off_t)(l->position+l->read_n-l->read_at),SEEK_SET)||
       ftruncate(fileno(draw),(off_t)l->draw_bytes)||fseeko(draw,(off_t)l->draw_bytes,SEEK_SET)||
       ftruncate(fileno(pages),(off_t)(stats->pages*32))||fseeko(pages,(off_t)(stats->pages*32),SEEK_SET)||
       ftruncate(fileno(chapters),(off_t)(stats->chapters*212))||fseeko(chapters,(off_t)(stats->chapters*212),SEEK_SET)||
       (config->chapter_spool&&fseeko(config->chapter_spool,(off_t)h[2],SEEK_SET))||ink_md_reset())goto bad;
    return l;
bad:free(l);return NULL;
}
int ink_layout_run(FILE *source,FILE *draw,FILE *pages,FILE *chapters,
    const ink_layout_config *config,ink_layout_stats *stats)
{
    void *l=ink_layout_begin(source,draw,pages,chapters,config,stats);if(!l)return -1;
    int result;do{result=ink_layout_step(l);}while(!result);
    ink_layout_end(l);return result<0?-1:0;
}
