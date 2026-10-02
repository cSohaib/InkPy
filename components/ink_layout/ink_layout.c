#define _POSIX_C_SOURCE 200809L
#include "ink_layout.h"
#include "md_budget.h"
#include "md4c.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

enum { CELLS=128, MARGIN=16 };
typedef struct { uint32_t value,min; unsigned need; uint64_t start; } Utf;
typedef struct { uint32_t cp; uint16_t style; uint64_t source; } Cell;
typedef struct {
    FILE *input,*draw,*pages,*chapters;
    ink_layout_config cfg;
    ink_layout_stats *stats;
    char block[INK_BLOCK_BYTES], physical[INK_BLOCK_BYTES];
    unsigned char read[1024];
    size_t block_n, read_n, read_at;
    uint64_t position,block_source,last_source,draw_bytes,page_begin,page_source;
    Utf validation,literal_utf;
    bool failed,literal,previous_cr,page_open;
    unsigned fence_char; uint64_t fence_length;
    unsigned y,head,bold,italic,code,math,image,chapter,page_chapter;
    Cell line[CELLS]; unsigned count;
    bool title_active,title_cut,title_located;
    unsigned title_n,title_page;
    uint64_t title_source;
    char title[INK_TITLE_BYTES];
    unsigned list_depth,list_order[16];
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
static void output_line(Layout *l,unsigned n)
{
    if (!n || l->failed) return;
    room(l); open_page(l,l->line[0].source);
    if (l->head==2 && !l->title_located) {
        l->title_located=true; l->title_page=(unsigned)l->stats->pages+1;
        l->title_source=l->line[0].source;
    }
    unsigned step=cell_width(l),x=MARGIN;
    for (unsigned i=0;i<n;) {
        unsigned j=i; char text[CELLS*4]; unsigned len=0;
        while (j<n && l->line[j].style==l->line[i].style) { len+=encode(l->line[j].cp,text+len); ++j; }
        number(l,l->draw,x,2); number(l,l->draw,l->y,2); number(l,l->draw,step,2);
        number(l,l->draw,l->line[i].style,2); number(l,l->draw,len,4);
        number(l,l->draw,l->line[i].source,8); bytes(l,l->draw,text,len);
        l->draw_bytes+=20+len; l->stats->runs++; x+=(j-i)*step; i=j;
    }
    l->y+=row_height(l);
}
static void finish_line(Layout *l)
{ output_line(l,l->count); l->count=0; }
static void paragraph(Layout *l)
{ finish_line(l); if (l->page_open) l->y+=6; }
static void emit(Layout *l,uint32_t cp,uint64_t source,bool preserve)
{
    if (l->failed) return;
    if (source>l->last_source) l->last_source=source;
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
    unsigned max=(l->cfg.width-2*MARGIN)/cell_width(l);
    if (l->count==max) {
        unsigned cut=l->count;
        if (!preserve) {
            for (unsigned i=l->count;i>0;--i) if (l->line[i-1].cp==' ') { cut=i; break; }
        }
        output_line(l,cut);
        memmove(l->line,l->line+cut,(l->count-cut)*sizeof(Cell)); l->count-=cut;
        if (!preserve && cp==' ' && !l->count) return;
    }
    l->line[l->count++]=(Cell){cp,(uint16_t)style(l),source};
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
static int block_enter(MD_BLOCKTYPE t,void *detail,void *user)
{
    Layout *l=user;
    if (t==MD_BLOCK_H || t==MD_BLOCK_CODE || t==MD_BLOCK_LI) paragraph(l);
    if (t==MD_BLOCK_H) {
        l->head=((MD_BLOCK_H_DETAIL*)detail)->level;
        if (l->head==2) {
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
    if (t==MD_BLOCK_P || t==MD_BLOCK_H || t==MD_BLOCK_CODE || t==MD_BLOCK_LI || t==MD_BLOCK_TR) paragraph(l);
    if (t==MD_BLOCK_H) {
        if (l->head==2) {
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
static int span(MD_SPANTYPE t,void *user,bool enter)
{
    Layout *l=user; int delta=enter?1:-1;
    if (t==MD_SPAN_STRONG) l->bold+=delta;
    if (t==MD_SPAN_EM) l->italic+=delta;
    if (t==MD_SPAN_CODE) l->code+=delta;
    if (t==MD_SPAN_LATEXMATH || t==MD_SPAN_LATEXMATH_DISPLAY) {
        if (enter) ++l->math;
        synthetic(l,t==MD_SPAN_LATEXMATH?"$":"$$");
        if (!enter) --l->math;
    }
    if (t==MD_SPAN_IMG) {
        if (enter) { ++l->image; synthetic(l,"[image: "); }
        else { synthetic(l,"]"); --l->image; }
    }
    return l->failed?-1:0;
}
static int span_enter(MD_SPANTYPE t,void *d,void *u) { (void)d; return span(t,u,true); }
static int span_leave(MD_SPANTYPE t,void *d,void *u) { (void)d; return span(t,u,false); }
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
    Layout *l=user; uint64_t at=l->last_source;
    uintptr_t p=(uintptr_t)s,base=(uintptr_t)l->block;
    if (p>=base && p-base<l->block_n) at=l->block_source+(p-base);
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
    if (r && (cp==0 || (cp<32 && cp!='\n' && cp!='\r' && cp!='\t') || cp==127)) fail(l,"NUL/control byte in text",l->validation.start);
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
static void scan(Layout *l)
{
    while (!l->failed && peek(l)!=EOF) {
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
    }
    if (l->validation.need) fail(l,"incomplete UTF-8 at EOF",l->validation.start);
    if (l->literal) paragraph(l); else parse(l);
    finish_line(l); end_page(l);
}
int ink_layout_run(FILE *source,FILE *draw,FILE *pages,FILE *chapters,
                   const ink_layout_config *config,ink_layout_stats *stats)
{
    memset(stats,0,sizeof(*stats));
    Layout *l=calloc(1,sizeof(*l));
    if (!l) { snprintf(stats->error,sizeof(stats->error),"layout allocation failed"); return -1; }
    l->input=source; l->draw=draw; l->pages=pages; l->chapters=chapters;
    l->cfg=*config; l->stats=stats; l->y=MARGIN; stats->context_bytes=sizeof(*l);
    if (config->width<120 || config->width>800 || config->height<100 || config->height>800 ||
        config->font_pixels<16 || config->font_pixels>28 || !config->read_bytes || config->read_bytes>sizeof(l->read)) fail(l,"invalid prototype geometry/read size",0);
    if (ink_md_reset()) fail(l,"parser allocator still in use",0);
    unsigned char bom[3]; size_t n=fread(bom,1,3,source);
    l->position=n==3 && !memcmp(bom,"\xef\xbb\xbf",3)?3:0;
    if (fseeko(source,(off_t)l->position,SEEK_SET)) fail(l,"source must be seekable",0);
    if (!l->failed) scan(l);
    if (fflush(draw)||fflush(pages)||fflush(chapters)) fail(l,"cache flush failed",l->position);
    stats->source_bytes=l->position; stats->parser_peak_bytes=ink_md_peak();
    int result=l->failed?-1:0; free(l); return result;
}
