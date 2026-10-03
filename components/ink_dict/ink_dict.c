#define _POSIX_C_SOURCE 200809L
#include "ink_dict.h"
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
typedef struct { char name[96],types[64]; uint32_t words,synonyms; uint64_t idxsize; unsigned offset; } Info;
static int fail(ink_dict *d,const char *s) { snprintf(d->error,sizeof(d->error),"%s",s); return -1; }
static int join(char *out,size_t size,const char *a,const char *b)
{ int n=snprintf(out,size,"%s/%s",a,b); return n<0||(size_t)n>=size?-1:0; }
static int cache_path(ink_dict *d,char out[560],const char *name) { return join(out,560,d->cache,name); }
static uint64_t be(const unsigned char *b,unsigned n)
{ uint64_t v=0; for(unsigned i=0;i<n;i++) v=(v<<8)|b[i]; return v; }
static int write_number(FILE *f,uint64_t value)
{ unsigned char b[8]; for(unsigned i=0;i<8;i++) { b[7-i]=(unsigned char)value; value>>=8; } return fwrite(b,1,8,f)==8?0:-1; }
static int seek(FILE *f,uint64_t offset) { return offset>LONG_MAX?-1:fseek(f,(long)offset,SEEK_SET); }
static uint64_t file_size(FILE *f)
{ if(fseek(f,0,SEEK_END)) return UINT64_MAX; long n=ftell(f); return n<0?UINT64_MAX:(uint64_t)n; }
static int fold(unsigned char c) { return c>='A'&&c<='Z'?c+32:c; }
static int compare(const char *a,const char *b,bool tie)
{
    const unsigned char *x=(const unsigned char *)a,*y=(const unsigned char *)b;
    while(*x&&fold(*x)==fold(*y)) { x++; y++; }
    int r=fold(*x)-fold(*y); return r?r:tie?strcmp(a,b):0;
}
static bool decimal(const char *s,uint64_t *out)
{
    if(!*s) return false;
    uint64_t n=0;
    for(;*s;s++) { if(*s<'0'||*s>'9'||n>(UINT64_MAX-(unsigned)(*s-'0'))/10) return false; n=n*10+(unsigned)(*s-'0'); }
    *out=n; return true;
}
static char *trim(char *s)
{ while(*s==' '||*s=='\t') s++; size_t n=strlen(s); while(n&&(s[n-1]==' '||s[n-1]=='\t')) s[--n]=0; return s; }
static int metadata(const char *path,Info *info)
{
    memset(info,0,sizeof(*info)); info->offset=4; FILE *f=fopen(path,"rb"); if(!f) return -1;
    char line[1024]; unsigned row=0,required=0; bool version3=false; int result=-1;
    for(;;) {
        unsigned n=0; bool overflow=false; int c;
        while((c=fgetc(f))!=EOF&&c!='\n'&&c!='\r') { if(n+1<sizeof(line)) line[n++]=(char)c; else overflow=true; }
        line[n]=0; if(c=='\r') { int next=fgetc(f); if(next!='\n'&&next!=EOF) ungetc(next,f); }
        if(!row++) { if(strcmp(line,"StarDict's dict ifo file")||overflow) goto done; }
        else {
            char *key=trim(line),*value=strchr(key,'=');
            if(value) {
                *value++=0; key=trim(key); value=trim(value); uint64_t number;
                if(!strcmp(key,"version")) { if(overflow||(strcmp(value,"2.4.2")&&strcmp(value,"3.0.0"))) goto done; version3=!strcmp(value,"3.0.0"); required|=1; }
                else if(!strcmp(key,"bookname")) {
                    if(!*value) goto done;
                    size_t size=strlen(value); if(size>=sizeof(info->name)) size=sizeof(info->name)-1;
                    while(size&&((unsigned char)value[size]&0xc0)==0x80) size--;
                    memcpy(info->name,value,size); info->name[size]=0; required|=2;
                }
                else if(!strcmp(key,"wordcount")) { if(overflow||!decimal(value,&number)||number>UINT32_MAX) goto done; info->words=(uint32_t)number; required|=4; }
                else if(!strcmp(key,"synwordcount")) { if(overflow||!decimal(value,&number)||number>UINT32_MAX) goto done; info->synonyms=(uint32_t)number; }
                else if(!strcmp(key,"idxfilesize")) { if(overflow||!decimal(value,&number)||number>LONG_MAX) goto done; info->idxsize=number; required|=8; }
                else if(!strcmp(key,"idxoffsetbits")) { if(overflow||(strcmp(value,"32")&&strcmp(value,"64"))) goto done; info->offset=!strcmp(value,"64")?8:4; }
                else if(!strcmp(key,"sametypesequence")) {
                    if(overflow||strlen(value)>=sizeof(info->types)) goto done;
                    for(char *p=value;*p;p++) if(!((*p>='a'&&*p<='z')||(*p>='A'&&*p<='Z'))) goto done;
                    strcpy(info->types,value);
                }
            } else if(*key) goto done;
        }
        if(c==EOF) break;
    }
    if(!ferror(f)&&required==15&&(version3||info->offset==4)) result=0;
done:
    fclose(f); return result;
}
static int owned_cache(ink_dict *d)
{
    bool created=mkdir(d->cache,0700)==0;
    if(!created&&errno!=EEXIST) return fail(d,"Cannot create dictionary cache");
    char path[560]; cache_path(d,path,"owner"); FILE *f=fopen(path,created?"wb":"rb");
    const char marker[]="InkPy dictionary cache v1\n"; char data[sizeof(marker)]={0}; bool valid=false;
    if(f) {
        if(created) valid=fwrite(marker,1,sizeof(marker)-1,f)==sizeof(marker)-1;
        else valid=fread(data,1,sizeof(marker)-1,f)==sizeof(marker)-1&&!memcmp(data,marker,sizeof(marker)-1);
        if(fclose(f)) valid=false;
    }
    return valid?0:fail(d,"Dictionary cache owner mismatch");
}
void ink_dict_close(ink_dict *d)
{
    FILE **files[]={&d->idx,&d->ord,&d->data,&d->syn,&d->sord,&d->definition,&d->defpages};
    for(unsigned i=0;i<sizeof(files)/sizeof(files[0]);i++) if(*files[i]) { fclose(*files[i]); *files[i]=NULL; }
    d->ready=false; d->pages=0;
}
void ink_dict_init(ink_dict *d,const char *root,void (*progress)(void))
{
    memset(d,0,sizeof(*d)); d->progress=progress;
    if(join(d->root,sizeof(d->root),root,"dictionaries")||join(d->cache,sizeof(d->cache),root,".inkpy-dict")) {
        d->root[0]=d->cache[0]=0; fail(d,"Dictionary path too long"); return;
    }
    /* Only read our selection after validating the owner marker. */
    char p[560],marker[32]={0}; cache_path(d,p,"owner"); FILE *f=fopen(p,"rb");
    if(f) { if(!fgets(marker,sizeof(marker),f)) marker[0]=0; fclose(f); }
    if(strcmp(marker,"InkPy dictionary cache v1\n")) return;
    cache_path(d,p,"selected"); f=fopen(p,"rb");
    if(f) { if(!fgets(d->selected,sizeof(d->selected),f)) d->selected[0]=0; fclose(f); }
    d->selected[strcspn(d->selected,"\r\n")]=0;
    size_t n=strlen(d->root);
    if(strncmp(d->selected,d->root,n)||d->selected[n]!='/'||strstr(d->selected,"/../")) d->selected[0]=0;
}
static void scan(ink_dict *d,const char *folder,unsigned depth)
{
    DIR *dir=opendir(folder); if(!dir) return;
    struct dirent *entry;
    while((entry=readdir(dir))) {
        if(entry->d_name[0]=='.'||!strcmp(entry->d_name,"res")) continue;
        char p[512]; struct stat st;
        if(join(p,sizeof(p),folder,entry->d_name)||stat(p,&st)) continue;
        if(S_ISDIR(st.st_mode)) { if(depth<8) scan(d,p,depth+1); continue; }
        size_t n=strlen(p); if(!S_ISREG(st.st_mode)||n<4||strcmp(p+n-4,".ifo")) continue;
        Info info; if(metadata(p,&info)) continue;
        unsigned i=d->total++;
        if(i>=d->first&&d->count<INK_DICT_ROWS) {
            ink_dict_choice *choice=&d->rows[d->count++]; strcpy(choice->path,p); strcpy(choice->name,info.name);
        }
        if(d->progress) d->progress();
    }
    closedir(dir);
}
int ink_dict_catalog(ink_dict *d,unsigned first)
{
    d->first=first; d->count=d->total=0; d->error[0]=0;
    if(!d->root[0]) return fail(d,"Dictionary path too long");
    scan(d,d->root,0);
    if(!d->total) return fail(d,"No dictionaries found");
    return 0;
}
static int word_record(FILE *f,char word[256],unsigned tail,unsigned char bytes[12])
{
    unsigned n=0; int c;
    while((c=fgetc(f))!=EOF&&c) { if(n==255) return -1; word[n++]=(char)c; }
    word[n]=0;
    return c==0&&n&&fread(bytes,1,tail,f)==tail?0:-1;
}
static bool valid_word(const char *word)
{
    const unsigned char *p=(const unsigned char *)word;
    while(*p) {
        uint32_t cp; unsigned n; uint32_t min;
        unsigned char c=*p++;
        if(c<128) continue;
        if(c>=0xc2&&c<=0xdf) { cp=c&31; n=1; min=128; }
        else if(c>=0xe0&&c<=0xef) { cp=c&15; n=2; min=2048; }
        else if(c>=0xf0&&c<=0xf4) { cp=c&7; n=3; min=65536; }
        else return false;
        while(n--) { if((*p&0xc0)!=0x80) return false; cp=(cp<<6)|(*p++&63); }
        if(cp<min||cp>0x10ffff||(cp>=0xd800&&cp<=0xdfff)) return false;
    }
    return true;
}
static int make_ord(ink_dict *d,FILE *in,FILE *out,uint32_t count,bool synonym,uint64_t size)
{
    rewind(in); char word[256],previous[256]={0}; unsigned char bytes[12];
    for(uint32_t i=0;i<count;i++) {
        long offset=ftell(in);
        if(offset<0||word_record(in,word,synonym?4:d->offset_bytes+4,bytes)||
           !valid_word(word)||(i&&compare(previous,word,true)>0)||write_number(out,(uint64_t)offset)) return fail(d,"Invalid dictionary index");
        if(synonym) { if(be(bytes,4)>=d->words) return fail(d,"Invalid synonym target"); }
        else { uint64_t pos=be(bytes,d->offset_bytes),n=be(bytes+d->offset_bytes,4); if(pos>size||n>size-pos) return fail(d,"Definition outside data file"); }
        strcpy(previous,word); if(d->progress&&!(i%256)) d->progress();
    }
    if(fgetc(in)!=EOF||ferror(in)||fflush(out)) return fail(d,"Index count or write mismatch");
    return 0;
}
static FILE *source_file(ink_dict *d,const char *base,const char *ext,const char *compressed,const char *cache)
{
    char p[560],out[560]; int n=snprintf(p,sizeof(p),"%s%s",base,ext);
    if(n<0||(size_t)n>=sizeof(p)) return NULL;
    FILE *f=fopen(p,"rb"); if(f) { cache_path(d,out,cache); remove(out); return f; }
    n=snprintf(p,sizeof(p),"%s%s",base,compressed); if(n<0||(size_t)n>=sizeof(p)) return NULL;
    cache_path(d,out,cache);
    if(ink_dict_gunzip(p,out,d->progress)) { fail(d,"Cannot decompress dictionary"); return NULL; }
    return fopen(out,"rb");
}
int ink_dict_select(ink_dict *d,const char *ifo)
{
    char selected[512]; if(strlen(ifo)>=sizeof(selected)) return fail(d,"Dictionary path too long"); strcpy(selected,ifo);
    ink_dict_close(d); d->error[0]=0; Info info;
    if(metadata(selected,&info)) return fail(d,"Invalid dictionary metadata");
    if(owned_cache(d)) return -1;
    strcpy(d->selected,selected); strcpy(d->name,info.name); strcpy(d->types,info.types);
    d->words=info.words; d->synonyms=info.synonyms; d->offset_bytes=info.offset;
    size_t n=strlen(selected); if(n<4||strcmp(selected+n-4,".ifo")) return fail(d,"Not a dictionary file");
    selected[n-4]=0;
    d->idx=source_file(d,selected,".idx",".idx.gz","idx");
    d->data=source_file(d,selected,".dict",".dict.dz","dict");
    if(!d->idx||!d->data) goto bad;
    uint64_t size=file_size(d->data);
    if(size==UINT64_MAX||file_size(d->idx)!=info.idxsize) { fail(d,"Dictionary size mismatch"); goto bad; }
    char p[560]; cache_path(d,p,"ord"); d->ord=fopen(p,"w+b");
    if(!d->ord||make_ord(d,d->idx,d->ord,d->words,false,size)) goto bad;
    snprintf(p,sizeof(p),"%s.syn",selected); d->syn=fopen(p,"rb");
    if(d->syn||d->synonyms) {
        if(!d->syn) { fail(d,"Missing synonym file"); goto bad; }
        cache_path(d,p,"sord"); d->sord=fopen(p,"w+b");
        if(!d->sord||make_ord(d,d->syn,d->sord,d->synonyms,true,size)) goto bad;
    }
    cache_path(d,p,"selected"); FILE *f=fopen(p,"wb");
    if(!f) { fail(d,"Cannot save dictionary choice"); goto bad; }
    bool saved=fprintf(f,"%s\n",d->selected)>0; if(fclose(f)) saved=false;
    if(!saved) { fail(d,"Cannot save dictionary choice"); goto bad; }
    d->ready=true; return 0;
bad:
    if(!d->error[0]) fail(d,"Cannot open dictionary files");
    ink_dict_close(d); return -1;
}
static int record_at(ink_dict *d,bool synonym,uint32_t i,char word[256],unsigned char b[12])
{
    FILE *ord=synonym?d->sord:d->ord,*index=synonym?d->syn:d->idx; unsigned char offset[8];
    if(!ord||!index||seek(ord,(uint64_t)i*8)||fread(offset,1,8,ord)!=8||seek(index,be(offset,8))||
       word_record(index,word,synonym?4:d->offset_bytes+4,b)) return fail(d,"Cannot read dictionary index");
    return 0;
}
static int lower_bound(ink_dict *d,bool synonym,const char *query,uint32_t *position)
{
    uint32_t lo=0,hi=synonym?d->synonyms:d->words; char word[256]; unsigned char bytes[12];
    while(lo<hi) { uint32_t mid=lo+(hi-lo)/2; if(record_at(d,synonym,mid,word,bytes)) return -1;
        if(compare(word,query,false)<0) lo=mid+1; else hi=mid; }
    *position=lo; return 0;
}
typedef struct { ink_dict *d; unsigned columns,line; uint64_t bytes; bool pending; } Sink;
static int emit(Sink *s,uint32_t cp)
{
    if(cp=='\r') return 0;
    if(cp=='\t') cp=' ';
    if(cp=='\n') {
        if(!s->columns) return 0;
        if(fputc('\n',s->d->definition)==EOF) return -1;
        s->bytes++; s->columns=0; if(++s->line==INK_DICT_LINES) { s->pending=true; s->line=0; } return 0;
    }
    if(cp<32) return 0;
    if(s->columns==INK_DICT_COLUMNS&&emit(s,'\n')) return -1;
    if(s->pending) { if(write_number(s->d->defpages,s->bytes)) return -1; s->d->pages++; s->pending=false; }
    unsigned char b[4]; unsigned n;
    if(cp<128) { b[0]=(unsigned char)cp; n=1; }
    else if(cp<2048) { b[0]=0xc0|(cp>>6); b[1]=0x80|(cp&63); n=2; }
    else if(cp<65536) { b[0]=0xe0|(cp>>12); b[1]=0x80|((cp>>6)&63); b[2]=0x80|(cp&63); n=3; }
    else { b[0]=0xf0|(cp>>18); b[1]=0x80|((cp>>12)&63); b[2]=0x80|((cp>>6)&63); b[3]=0x80|(cp&63); n=4; }
    if(fwrite(b,1,n,s->d->definition)!=n) return -1;
    s->bytes+=n; s->columns++; return 0;
}
static int message(Sink *s,const char *text)
{ for(;*text;text++) if(emit(s,(unsigned char)*text)) return -1; return emit(s,'\n'); }
static int codepoint(FILE *f,uint64_t *remaining,uint32_t *out)
{
    if(!*remaining) return -1;
    int c=fgetc(f); if(c==EOF) return -1; (*remaining)--;
    uint32_t value; unsigned n; uint32_t min;
    if(c<128) { *out=(unsigned)c; return 0; }
    if(c>=0xc2&&c<=0xdf) { value=c&31; n=1; min=128; }
    else if(c>=0xe0&&c<=0xef) { value=c&15; n=2; min=2048; }
    else if(c>=0xf0&&c<=0xf4) { value=c&7; n=3; min=65536; }
    else return -1;
    while(n--) { if(!*remaining||(c=fgetc(f))==EOF||(c&0xc0)!=0x80) return -1; (*remaining)--; value=(value<<6)|(c&63); }
    if(value<min||value>0x10ffff||(value>=0xd800&&value<=0xdfff)) return -1;
    *out=value; return 0;
}
static uint32_t entity(const char *s)
{
    static const struct { const char *name; uint32_t cp; } known[]={
        {"eacute",233},{"Eacute",201},{"egrave",232},{"agrave",224},{"ecirc",234},
        {"ouml",246},{"uuml",252},{"auml",228},{"szlig",223},{"ccedil",231},
        {"ndash",0x2013},{"mdash",0x2014},{"lsquo",0x2018},{"rsquo",0x2019},
        {"ldquo",0x201c},{"rdquo",0x201d},{"hellip",0x2026}
    };
    for(unsigned i=0;i<sizeof(known)/sizeof(known[0]);i++) if(!strcmp(s,known[i].name)) return known[i].cp;
    if(!strcmp(s,"amp")) return '&';
    if(!strcmp(s,"lt")) return '<';
    if(!strcmp(s,"gt")) return '>';
    if(!strcmp(s,"quot")) return '"';
    if(!strcmp(s,"apos")) return '\'';
    if(!strcmp(s,"nbsp")) return ' ';
    if(*s=='#') { char *end; unsigned long n=strtoul(s+(s[1]=='x'||s[1]=='X'?2:1),&end,s[1]=='x'||s[1]=='X'?16:10);
        if(!*end&&n&&n<=0x10ffff&&!(n>=0xd800&&n<=0xdfff)) return (uint32_t)n; }
    return 0;
}
static int field_text(Sink *sink,uint64_t *remaining,bool terminated,bool markup)
{
    FILE *f=sink->d->data; char token[64]; unsigned used=0; bool tag=false,ent=false;
    uint64_t checkpoint=*remaining;
    while(*remaining) {
        uint32_t cp; if(codepoint(f,remaining,&cp)) return -1;
        if(checkpoint-*remaining>=4096) { if(sink->d->progress) sink->d->progress(); checkpoint=*remaining; }
        if(!cp) { if(tag||ent) return -1; return 0; }
        if(markup&&tag) {
            if(cp=='>') { token[used]=0; char *name=token; if(*name=='/') name++;
                name[strcspn(name," /\t")]=0;
                if(!strcmp(name,"br")||!strcmp(name,"p")||!strcmp(name,"div")||!strcmp(name,"li")||!strcmp(name,"tr")||!strcmp(name,"def")) if(emit(sink,'\n')) return -1;
                tag=false; used=0;
            } else if(cp<128&&used+1<sizeof(token)) token[used++]=(char)fold((unsigned char)cp);
            continue;
        }
        if(markup&&ent) {
            if(cp==';') { token[used]=0; uint32_t e=entity(token);
                if(e) { if(emit(sink,e)) return -1; } else { if(emit(sink,'&')) return -1; for(unsigned i=0;i<used;i++) if(emit(sink,(unsigned char)token[i])) return -1; if(emit(sink,';')) return -1; }
                ent=false; used=0;
            } else if(cp<128&&used+1<sizeof(token)) token[used++]=(char)cp;
            else return -1;
            continue;
        }
        if(markup&&cp=='<') { tag=true; used=0; }
        else if(markup&&cp=='&') { ent=true; used=0; }
        else if(emit(sink,cp)) return -1;
    }
    return terminated||tag||ent?-1:0;
}
static int article(ink_dict *d,Sink *sink,uint64_t offset,uint64_t remaining)
{
    if(seek(d->data,offset)) return -1;
    unsigned sequence=0;
    while(remaining||(d->types[0]&&d->types[sequence])) {
        int type; bool last=false;
        if(d->types[0]) { type=(unsigned char)d->types[sequence++]; if(!type) return -1; last=!d->types[sequence]; }
        else { type=fgetc(d->data); if(type==EOF) return -1; remaining--; }
        if(type>='a'&&type<='z') {
            bool omitted=type=='l'||type=='r';
            if(omitted) {
                if(message(sink,type=='l'?"[Locale text omitted]":"[Resources omitted]")) return -1;
                bool ended=false; while(remaining) { int c=fgetc(d->data); if(c==EOF) return -1; remaining--; if(!c) { ended=true; break; }
                    if(d->progress&&!(remaining%4096)) d->progress(); }
                if(!last&&!ended) return -1;
            } else if(field_text(sink,&remaining,!last,type=='h'||type=='x'||type=='g')) return -1;
        } else if(type>='A'&&type<='Z') {
            uint64_t n=remaining;
            if(!last) { unsigned char b[4]; if(remaining<4||fread(b,1,4,d->data)!=4) return -1; remaining-=4; n=be(b,4); }
            long pos=ftell(d->data); if(pos<0||n>remaining||seek(d->data,(uint64_t)pos+n)) return -1;
            remaining-=n; if(message(sink,"[Media omitted]")) return -1;
        } else return -1;
        if(emit(sink,'\n')) return -1;
        if(last&&remaining) return -1;
    }
    return d->types[0]&&d->types[sequence]?-1:0;
}
static int load_page(ink_dict *d)
{
    memset(d->lines,0,sizeof(d->lines)); if(!d->definition||!d->defpages||!d->pages) return 0;
    unsigned char b[8]; if(seek(d->defpages,(uint64_t)d->page*8)||fread(b,1,8,d->defpages)!=8||seek(d->definition,be(b,8))) return fail(d,"Cannot read definition page");
    for(unsigned i=0;i<INK_DICT_LINES;i++) { if(!fgets(d->lines[i],sizeof(d->lines[i]),d->definition)) break; d->lines[i][strcspn(d->lines[i],"\r\n")]=0; }
    return ferror(d->definition)?fail(d,"Definition read failed"):0;
}
int ink_dict_page(ink_dict *d,int direction)
{ if(direction<0&&d->page) d->page--; if(direction>0&&d->page+1<d->pages) d->page++; return load_page(d); }
int ink_dict_lookup(ink_dict *d,const char *query)
{
    if(!d->ready) {
        if(!d->selected[0]) { if(ink_dict_catalog(d,0)) goto error; strcpy(d->selected,d->rows[0].path); }
        if(ink_dict_select(d,d->selected)) goto error;
    }
    d->error[0]=0; if(d->definition) fclose(d->definition); if(d->defpages) fclose(d->defpages);
    char p[560]; cache_path(d,p,"definition"); d->definition=fopen(p,"w+b");
    cache_path(d,p,"defpages"); d->defpages=fopen(p,"w+b"); d->page=0; d->pages=1;
    if(!d->definition||!d->defpages||write_number(d->defpages,0)) { fail(d,"Cannot create definition cache"); goto error; }
    Sink sink={.d=d}; uint32_t position; char word[256]; unsigned char bytes[12]; bool found=false;
    if(lower_bound(d,false,query,&position)) goto error;
    while(position<d->words) {
        if(record_at(d,false,position++,word,bytes)) goto error;
        if(compare(word,query,false)) break;
        if(article(d,&sink,be(bytes,d->offset_bytes),be(bytes+d->offset_bytes,4))) { fail(d,"Invalid definition data"); goto error; }
        found=true;
    }
    if(!found&&d->synonyms) {
        if(lower_bound(d,true,query,&position)) goto error;
        while(position<d->synonyms) {
            if(record_at(d,true,position++,word,bytes)) goto error;
            if(compare(word,query,false)) break;
            uint32_t target=(uint32_t)be(bytes,4);
            if(record_at(d,false,target,word,bytes)||article(d,&sink,be(bytes,d->offset_bytes),be(bytes+d->offset_bytes,4))) { fail(d,"Invalid synonym definition"); goto error; }
            found=true;
        }
    }
    if(!sink.bytes&&message(&sink,found?"No textual definition":"Word not found")) { fail(d,"Definition write failed"); goto error; }
    if(fflush(d->definition)||fflush(d->defpages)||load_page(d)) { fail(d,"Definition cache failed"); goto error; }
    return found?0:1;
error:
    memset(d->lines,0,sizeof(d->lines)); d->page=0; d->pages=0;
    if(!d->error[0]) fail(d,"Dictionary lookup failed");
    return -1;
}
