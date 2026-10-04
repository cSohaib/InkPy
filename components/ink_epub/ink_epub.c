#include "ink_epub.h"
#include "ink_xml.h"
#include "ink_zip.h"
#include <ctype.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/stat.h>

typedef struct {
    char id[128], path[512];
} Item;
typedef ink_epub_chapter Chapter;
static char debug_path[640];
static bool cancelled;
void ink_epub_cancel(void){cancelled=true;}
void ink_epub_reset_cancel(void){cancelled=false;}
bool ink_epub_cancelled(void){return cancelled;}
static const char *phase="";
const char *ink_epub_phase(void){return phase;}
void ink_epub_debug(const char *format,...)
{
    if(!debug_path[0])return;
    struct stat st;
    if(!stat(debug_path,&st)&&st.st_size>524288)return;
    FILE *f=fopen(debug_path,"ab");
    if(!f)return;
    va_list args;va_start(args,format);vfprintf(f,format,args);va_end(args);
    fputc('\n',f);
    fclose(f);
}
void ink_epub_debug_start(const char *root,const char *source)
{
    snprintf(debug_path,sizeof(debug_path),"%s/inkpy-epub-debug.txt",root);
    ink_epub_debug("\nInkPy stage38 EPUB: %s",source);
}
typedef struct {
    ink_zip zip;
    const char *cache;
    FILE *map, *spine, *toc, *body, *chapters, *positions;
    ink_xml xml;
    unsigned images, ticks;
    void (*progress)(void);
} Import;
static int path(char *out, size_t size, const char *base, const char *name) {
    int n = snprintf(out, size, "%s/%s", base, name);
    return n < 0 || (size_t)n >= size ? -1 : 0;
}
static FILE *scratch(Import *b, const char *name, const char *mode) {
    char p[640];
    return path(p, sizeof(p), b->cache, name) ? NULL : fopen(p, mode);
}
static int extract(Import *b, const char *name, const char *dest) {
    FILE *f = scratch(b, dest, "w+b");
    if (!f)
        return -1;
    int r = ink_zip_extract(&b->zip, name, f);
    if (fclose(f))
        r = -1;
    return r;
}
/* Resolve archive-relative references, dropping queries/fragments. Never use an
 * archive name as a filesystem output path. Percent-decoding supports spaces. */
static int resolve(char out[512], const char *base, const char *href) {
    if(href[0]=='#'){snprintf(out,512,"%s",base);
    return 0;}
    if (!*href || strstr(href, ":"))
        return -1;
    char temp[1024];
    const char *slash = strrchr(base, '/');
    size_t prefix = href[0]=='/'?0:slash ? (size_t)(slash - base) + 1 : 0;
    if (prefix + strlen(href) >= sizeof(temp))
        return -1;
    memcpy(temp, base, prefix);
    strcpy(temp + prefix, href);
    char *end = strpbrk(temp, "#?");
    if (end)
        *end = 0;
    char decoded[1024];
    size_t n = 0;
    for (size_t i = 0; temp[i]; i++) {
        if (temp[i] == '%' && isxdigit((unsigned char)temp[i + 1]) && temp[i + 1] && temp[i + 2] &&
            isxdigit((unsigned char)temp[i + 2])) {
            char hex[3] = {temp[i + 1], temp[i + 2], 0};
            decoded[n++] = (char)strtoul(hex, NULL, 16);
            i += 2;
        } else
            decoded[n++] = temp[i];
    }
    decoded[n] = 0;
    out[0] = 0;
    char *save = NULL;
    for (char *part = strtok_r(decoded, "/", &save); part; part = strtok_r(NULL, "/", &save)) {
        if (!strcmp(part, "."))
            continue;
        if (!strcmp(part, "..")) {
            char *last = strrchr(out, '/');
            if (last)
                *last = 0;
            else if (out[0])
                out[0] = 0;
            else
                return -1;
            continue;
        }
        size_t used = strlen(out), len = strlen(part);
        if (used + len + (used ? 1 : 0) >= 512)
            return -1;
        if (used)
            out[used++] = '/';
        memcpy(out + used, part, len + 1);
    }
    return out[0] ? 0 : -1;
}
static int lookup(Import *b, const char *id, Item *item) {
    long start=ftell(b->map);
    if(start<0||fseek(b->map,start,SEEK_SET))return -1;
    for(unsigned pass=0;pass<2;pass++) {
        while(fread(item,1,sizeof(*item),b->map)==sizeof(*item)) {
            if(!strcmp(item->id,id))return 0;
            if(pass&&ftell(b->map)>=start)break;
        }
        rewind(b->map);
    }
    return -1;
}
static int xml_open(Import *b, const char *name) {
    if (b->xml.file) {
        fclose(b->xml.file);
        b->xml.file = NULL;
    }
    if (extract(b, name, "epub-xml"))
        return -1;
    b->xml.file = scratch(b, "epub-xml", "rb");
    return b->xml.file ? 0 : -1;
}
static void append(char *out, size_t cap, const char *s) {
    size_t n = strlen(out);
    if (n + 1 < cap) {
        size_t len = strlen(s);
        if (len >= cap - n)
            len = cap - n - 1;
        memcpy(out + n, s, len);
        out[n + len] = 0;
    }
}
static void tidy(char *s) {
    char *out = s;
    bool space = true;
    for (char *p = s; *p; p++) {
        if (isspace((unsigned char)*p)) {
            if (!space)
                *out++ = ' ';
            space = true;
        } else {
            *out++ = *p;
            space = false;
        }
    }
    if (out > s && out[-1] == ' ')
        out--;
    *out = 0;
}
static int navigation(Import *b, const char *nav, bool ncx, bool loose) {
    if (xml_open(b, nav))
        return -1;
    Chapter c = {0};
    unsigned depth = 0, toc_depth = 0;
    bool label = false, link = false;
    int kind;
    while ((kind = ink_xml_next(&b->xml)) > 0) {
        if(cancelled)return -1;
        ink_xml *x = &b->xml;
        if (kind == 2) {
            if (label || link) {
                ink_xml_entities(x->text);
                append(c.title, sizeof(c.title), x->text);
            }
            continue;
        }
        if (!ncx && !strcmp(x->name, "nav")) {
            if (x->closing) {
                if (depth == toc_depth)
                    toc_depth = 0;
                if (depth)
                    depth--;
            } else {
                depth++;
                char type[128];
                ink_xml_attr(x, "epub:type", type, sizeof(type));
                if(!type[0])ink_xml_attr(x,"type",type,sizeof(type));
                if(!type[0])ink_xml_attr(x,"role",type,sizeof(type));
                if (strstr(type, "toc"))
                    toc_depth = depth;
            }
        }
        if (ncx && !strcmp(x->name, "navLabel")) {
            label = !x->closing;
            if (label)
                c.title[0] = 0;
        }
        if (!ncx && (toc_depth||loose) && !strcmp(x->name, "a")) {
            if (!x->closing) {
                char href[704];
                ink_xml_attr(x, "href", href, sizeof(href));
                memset(&c, 0, sizeof(c));
                if (!resolve(c.path, nav, href)) {
                    char *hash = strchr(href, '#');
                    if (hash)
                        snprintf(c.fragment, sizeof(c.fragment), "%s", hash + 1);
                    link = true;
                }
            } else if (link) {
                tidy(c.title);
                if (fwrite(&c, 1, sizeof(c), b->toc) != sizeof(c))
                    return -1;
                link = false;
            }
        }
        if (ncx && !x->closing && !strcmp(x->name, "content")) {
            char href[704];
            ink_xml_attr(x, "src", href, sizeof(href));
            if (resolve(c.path, nav, href))
                continue;
            char *hash = strchr(href, '#');
            c.fragment[0] = 0;
            if (hash)
                snprintf(c.fragment, sizeof(c.fragment), "%s", hash + 1);
            tidy(c.title);
            if (fwrite(&c, 1, sizeof(c), b->toc) != sizeof(c))
                return -1;
        }
    }
    return kind < 0 ? -1 : 0;
}
static int chapter(Import *b, const char *document, const char *id, bool fallback) {
    Chapter c;
    bool found = false;unsigned id_number=0;
    rewind(b->toc);
    while (fread(&c, 1, sizeof(c), b->toc) == sizeof(c)) {
        id_number++;
        if (strcmp(c.path, document) || strcmp(c.fragment, id))
            continue;
        found = true;
        break;
    }
    if (!found && !fallback)
        return 0;
    if (!found) {
        memset(&c, 0, sizeof(c));
        const char *name = strrchr(document, '/');
        (void)name;snprintf(c.title,sizeof(c.title),"%s",document);
    }
    fputs("\n\n", b->body);
    long at = ftell(b->body);
    if (at < 0)
        return -1;
    if(b->positions&&found) {
        uint64_t position[2]={c.id?c.id:id_number,(uint64_t)at};
        if(fwrite(position,1,sizeof(position),b->positions)!=sizeof(position))return -1;
    }
    unsigned char record[202] = {0};
    uint64_t source = (uint64_t)at;
    unsigned n = (unsigned)strlen(c.title);
    for (unsigned i = 0; i < 8; i++)
        record[i] = (unsigned char)(source >> (i * 8));
    record[8] = n & 255;
    record[9] = n >> 8;
    memcpy(record + 10, c.title, n);
    return fwrite(record, 1, sizeof(record), b->chapters) == sizeof(record) ? 1 : -1;
}
static int document(Import *b, const char *name, bool fallback) {
    if (xml_open(b, name))
        return -1;
    int marked = chapter(b, name, "", fallback);
    if (marked < 0)
        return -1;
    bool body = false, pre = false;
    unsigned skip = 0, table = 0, cells = 0, rows = 0, list_depth = 0, list_next[16] = {0};
    bool in_cell = false;
    int kind;
    while ((kind = ink_xml_next(&b->xml)) > 0) {
        if(cancelled)return -1;
        ink_xml *x = &b->xml;
        if (kind == 2) {
            if (!body || skip)
                continue;
            ink_xml_entities(x->text);
            for (char *p = x->text; *p; p++) {
                if (in_cell && (*p == '\n' || *p == '\r'))
                    fputc(' ', b->body);
                else {
                    if (in_cell && *p == '|')
                        fputc('\\', b->body);
                    fputc(*p, b->body);
                }
            }
            continue;
        }
        if (!strcmp(x->name, "body")) {
            body = !x->closing;
            continue;
        }
        if (!body)
            continue;
        if (!strcmp(x->name, "script") || !strcmp(x->name, "style") || !strcmp(x->name, "math")) {
            if (x->closing) {
                if (skip)
                    skip--;
            } else if (!x->empty)
                skip++;
            continue;
        }
        if (skip)
            continue;
        if (!x->closing) {
            char id[192];
            ink_xml_attr(x, "id", id, sizeof(id));
            if (id[0] && chapter(b, name, id, false) < 0)
                return -1;
        }
        const char *tag = x->name;
        if (!strcmp(tag, "table")) {
            if (x->closing) {
                table = 0;
                fputs("\n\n", b->body);
            } else {
                table = 1;
                rows = 0;
                fputs("\n\n", b->body);
            }
            continue;
        }
        if (table && !strcmp(tag, "tr")) {
            if (!x->closing) {
                cells = 0;
                fputc('|', b->body);
            } else {
                fputc('\n', b->body);
                if (!rows++) {
                    fputc('|', b->body);
                    for (unsigned i = 0; i < cells; i++)
                        fputs(" --- |", b->body);
                    fputc('\n', b->body);
                }
            }
            continue;
        }
        if (table && (!strcmp(tag, "td") || !strcmp(tag, "th"))) {
            in_cell = !x->closing;
            if (x->closing)
                fputs(" |", b->body);
            else {
                cells++;
                fputc(' ', b->body);
            }
            continue;
        }
        if (!strcmp(tag, "img") || !strcmp(tag, "image")) {
            if (x->closing)
                continue;
            char src[704], resource[512], alt[192];
            ink_xml_attr(x, "src", src, sizeof(src));
            if (!src[0])
                ink_xml_attr(x, "xlink:href", src, sizeof(src));
            if (!src[0])
                ink_xml_attr(x, "href", src, sizeof(src));
            ink_xml_attr(x, "alt", alt, sizeof(alt));
            if (!resolve(resource,name,src)) {
                /* Angle-bracket destination supports spaces and parentheses. */
                fprintf(b->body,"![image](<%s>)",resource);
            } else {
                ink_epub_debug("image unresolved document=%s src=%s",name,src);
                fprintf(b->body,"[image: %s]",alt);
            }
            continue;
        }
        if (!strcmp(tag, "pre")) {
            pre = !x->closing;
            fputs("\n\n```\n", b->body);
            if (!pre)
                fputc('\n', b->body);
            continue;
        }
        if (!strcmp(tag, "code") && !pre) {
            fputc('`', b->body);
            continue;
        }
        if (!strcmp(tag, "strong") || !strcmp(tag, "b")) {
            fputs("**", b->body);
            continue;
        }
        if (!strcmp(tag, "em") || !strcmp(tag, "i")) {
            fputc('*', b->body);
            continue;
        }
        if (!strcmp(tag, "br")) {
            fputs(in_cell ? " " : "  \n", b->body);
            continue;
        }
        if (strlen(tag) == 2 && tag[0] == 'h' && tag[1] >= '1' && tag[1] <= '6') {
            if (!in_cell) {
                fputs("\n\n", b->body);
                if (!x->closing) {
                    for (int i = 0; i < tag[1] - '0'; i++)
                        fputc('#', b->body);
                    fputc(' ', b->body);
                }
            }
            continue;
        }
        if (!strcmp(tag, "ul") || !strcmp(tag, "ol")) {
            if (x->closing) {
                if (list_depth)
                    list_depth--;
            } else {
                if (list_depth == 16)
                    return -1;
                char start[16];
                ink_xml_attr(x, "start", start, sizeof(start));
                list_next[list_depth++] =
                    !strcmp(tag, "ol") ? (start[0] ? (unsigned)strtoul(start, NULL, 10) : 1) : 0;
            }
            fputs(in_cell ? " " : "\n\n", b->body);
            continue;
        }
        if (!strcmp(tag, "li")) {
            if (!in_cell) {
                fputc('\n', b->body);
                if (!x->closing) {
                    for (unsigned i = 1; i < list_depth; i++)
                        fputs("  ", b->body);
                    if (list_depth && list_next[list_depth - 1])
                        fprintf(b->body, "%u. ", list_next[list_depth - 1]++);
                    else
                        fputs("- ", b->body);
                }
            }
            continue;
        }
        if (!strcmp(tag, "p") || !strcmp(tag, "div") || !strcmp(tag, "section") || !strcmp(tag, "ul") ||
            !strcmp(tag, "ol") || !strcmp(tag, "blockquote"))
            fputs(in_cell ? " " : "\n\n", b->body);
        if (b->progress && ++b->ticks % 64 == 0)
            b->progress();
    }
    fputs("\n\n", b->body);
    return kind < 0 || ferror(b->body) ? -1 : 0;
}
void ink_epub_cleanup(const char *cache) {
    DIR *d = opendir(cache);
    if (!d)
        return;
    struct dirent *e;
    char p[640];
    while ((e = readdir(d)))
        if (!strncmp(e->d_name, "epub-", 5) && !path(p, sizeof(p), cache, e->d_name))
            unlink(p);
    closedir(d);
}
int ink_epub_prepare(const char *source, const char *cache, void (*progress)(void), char *error, size_t cap) {
    Import *b = calloc(1, sizeof(*b));
    if (!b) {
        snprintf(error, cap, "EPUB allocation failed");
        return -1;
    }
    b->cache = cache;
    b->progress = progress;
    int result = -1, kind;
    char opf[512] = {0}, nav[512] = {0}, ncx[512] = {0},guide[512]={0},spine_toc[128]={0};
    const char *why = "Cannot read EPUB archive";
    if (ink_zip_open(&b->zip, source, progress))
        goto done;
    b->map = scratch(b, "epub-map", "w+b");
    b->spine = scratch(b, "epub-spine", "w+b");
    b->toc = scratch(b, "epub-toc", "w+b");
    phase="contents";
    why = "Cannot create EPUB cache";
    if (!b->map || !b->spine || !b->toc)
        goto done;
    why = "Invalid EPUB container/package";
    if (xml_open(b, "META-INF/container.xml"))
        goto done;
    while ((kind = ink_xml_next(&b->xml)) > 0)
        if (kind == 1 && !b->xml.closing && !strcmp(b->xml.name, "rootfile")) {
            ink_xml_attr(&b->xml, "full-path", opf, sizeof(opf));
            break;
        }
    if (!opf[0] || xml_open(b, opf))
        goto done;
    while ((kind = ink_xml_next(&b->xml)) > 0) {
        if(cancelled)return -1;
        if (kind != 1 || b->xml.closing)
            continue;
        if (!strcmp(b->xml.name, "item")) {
            Item item = {0};
            char href[704], properties[256], media[128];
            ink_xml_attr(&b->xml, "id", item.id, sizeof(item.id));
            ink_xml_attr(&b->xml, "href", href, sizeof(href));
            ink_xml_attr(&b->xml, "properties", properties, sizeof(properties));
            ink_xml_attr(&b->xml, "media-type", media, sizeof(media));
            if (resolve(item.path, opf, href))
                continue;
            if (fwrite(&item, 1, sizeof(item), b->map) != sizeof(item))
                goto done;
            if (strstr(properties, "nav"))
                strcpy(nav, item.path);
            const char *extension=strrchr(item.path,'.');
            if (!strcmp(media, "application/x-dtbncx+xml")||(extension&&!strcasecmp(extension,".ncx")))
                strcpy(ncx, item.path);
        } else if(!strcmp(b->xml.name,"spine")) {
            ink_xml_attr(&b->xml,"toc",spine_toc,sizeof(spine_toc));
        } else if(!strcmp(b->xml.name,"reference")) {
            char type[64],href[704];ink_xml_attr(&b->xml,"type",type,sizeof(type));
            ink_xml_attr(&b->xml,"href",href,sizeof(href));
            if(!strcmp(type,"toc"))resolve(guide,opf,href);
        } else if (!strcmp(b->xml.name, "itemref")) {
            Item item;
            char id[128], linear[16];
            ink_xml_attr(&b->xml, "linear", linear, sizeof(linear));
            if (!strcmp(linear, "no"))
                continue;
            ink_xml_attr(&b->xml, "idref", id, sizeof(id));
            if (lookup(b, id, &item))
                goto done;
            if (fwrite(item.path, 1, sizeof(item.path), b->spine) != sizeof(item.path))
                goto done;
        }
    }
    if (kind < 0)
        goto done;
    why = "Invalid EPUB navigation";
    int nav_status=-1;
    if(!ncx[0]&&spine_toc[0]){Item item;if(!lookup(b,spine_toc,&item))strcpy(ncx,item.path);}
    if(!nav[0]&&guide[0])strcpy(nav,guide);
    if(nav[0]) {
        nav_status=navigation(b,nav,false,false);
        if(!nav_status&&!ftell(b->toc))nav_status=navigation(b,nav,false,true);
    }
    if(nav_status||!ftell(b->toc)) {
        if(nav[0])ink_epub_debug("nav fallback path=%s status=%d entries=%ld",nav,nav_status,ftell(b->toc)/(long)sizeof(Chapter));
        fclose(b->toc);b->toc=scratch(b,"epub-toc","w+b");
        if(!b->toc)goto done;
        if(ncx[0])nav_status=navigation(b,ncx,true,false);
        else nav_status=-1;
        if(nav_status){fclose(b->toc);b->toc=scratch(b,"epub-toc","w+b");
    if(!b->toc)goto done;}
    }
    if(fflush(b->toc)||fflush(b->spine))goto done;
    char name[512];unsigned count=0;
    rewind(b->spine);while(fread(name,1,sizeof(name),b->spine)==sizeof(name))count++;
    if(!count){why="EPUB has no readable spine";goto done;}
    bool fallback=ftell(b->toc)==0;
    if(fallback) {
        rewind(b->spine);
        for(unsigned i=1;i<=count;i++) {
            Chapter c={.document=i,.id=i};
            if(fread(c.path,1,sizeof(c.path),b->spine)!=sizeof(c.path))goto done;
            snprintf(c.title,sizeof(c.title),"%u",i);
            if(fwrite(&c,1,sizeof(c),b->toc)!=sizeof(c))goto done;
        }
        ink_epub_debug("no usable TOC; spine fallback=%u (titles resolved when opened)",count);
    } else {
        rewind(b->toc);rewind(b->spine);Chapter c;unsigned toc_id=0,spine_at=0;name[0]=0;
        while(fread(&c,1,sizeof(c),b->toc)==sizeof(c)) {
            c.id=++toc_id;
            long next=ftell(b->toc);c.document=0;
            if(spine_at&&!strcmp(name,c.path))c.document=spine_at;
            else for(unsigned pass=0;pass<2&&!c.document;pass++) {
                while(fread(name,1,sizeof(name),b->spine)==sizeof(name)) {
                    spine_at++;
                    if(!strcmp(name,c.path)){c.document=spine_at;break;}
                }
                if(!c.document){rewind(b->spine);spine_at=0;name[0]=0;}
            }
            if(!c.title[0])snprintf(c.title,sizeof(c.title),"%u",(unsigned)c.document);
            if(fseek(b->toc,next-(long)sizeof(c),SEEK_SET)||fwrite(&c,1,sizeof(c),b->toc)!=sizeof(c)||
               fseek(b->toc,next,SEEK_SET))goto done;
            ink_epub_debug("toc document=%u title=%s path=%s fragment=%s",c.document,c.title,c.path,c.fragment);
        }
    }
    if(fflush(b->toc))goto done;
    ink_epub_debug("metadata complete spine=%u chapters=%ld nav=%s ncx=%s",count,ftell(b->toc)/(long)sizeof(Chapter),nav,ncx);
    result = 0;
done:
    if (b->xml.file)
        fclose(b->xml.file);
    if (b->zip.file)
        fclose(b->zip.file);
    FILE *files[] = {b->map, b->spine, b->toc, b->body, b->chapters};
    for (unsigned i = 0; i < 5; i++)
        if (files[i] && fclose(files[i]))
            result = -1;
    if (result){snprintf(error,cap,"%s",why);ink_epub_debug("prepare error: %s",why);}
    phase="layout";
    free(b);
    return result;
}

static unsigned records(const char *cache,const char *name,unsigned size)
{
    char p[640];struct stat st;
    return !path(p,sizeof(p),cache,name)&&!stat(p,&st)?(unsigned)(st.st_size/size):0;
}
unsigned ink_epub_documents(const char *cache){return records(cache,"epub-spine",512);}
unsigned ink_epub_chapters(const char *cache){return records(cache,"epub-toc",sizeof(Chapter));}
int ink_epub_get_chapter(const char *cache,unsigned id,ink_epub_chapter *c)
{
    char p[640];
    if(!id||path(p,sizeof(p),cache,"epub-toc"))return -1;
    FILE *f=fopen(p,"rb");
    if(!f)return -1;
    int r=fseek(f,(long)(id-1)*sizeof(*c),SEEK_SET)||fread(c,1,sizeof(*c),f)!=sizeof(*c);
    fclose(f);
    return r?-1:0;
}
int ink_epub_document(const char *source,const char *cache,unsigned id,const char *dest,
    void (*progress)(void),char *error,size_t capacity)
{
    Import *b=calloc(1,sizeof(*b));
    if(!b)return -1;
    b->cache=dest;b->progress=progress;phase="chapter";
    char p[640],name[512];int result=-1;
    b->toc=!path(p,sizeof(p),cache,"epub-toc")?fopen(p,"r+b"):NULL;
    b->spine=!path(p,sizeof(p),cache,"epub-spine")?fopen(p,"rb"):NULL;
    if(!b->toc||!b->spine||!id||fseek(b->spine,(long)(id-1)*512,SEEK_SET)||
       fread(name,1,sizeof(name),b->spine)!=sizeof(name)||ink_zip_open(&b->zip,source,progress))goto done;
    ink_epub_debug("convert document=%u path=%s",id,name);
    /* Resolve numeric fallback titles from the document's title or first heading.
     * Only this document is inspected; opening never scans the whole book. */
    rewind(b->toc);Chapter c;bool needs_title=false;char fallback[16];
    snprintf(fallback,sizeof(fallback),"%u",id);
    while(fread(&c,1,sizeof(c),b->toc)==sizeof(c))if(c.document==id&&!strcmp(c.title,fallback))needs_title=true;
    if(needs_title) {
        if(xml_open(b,name))goto done;
        char title[192]={0};bool label=false;int kind;unsigned ticks=0;
        while(ftell(b->xml.file)<65536&&(kind=ink_xml_next(&b->xml))>0) {
            if(progress&&++ticks%64==0)progress();
            if(cancelled)goto done;
            if(kind==1&&(!strcmp(b->xml.name,"title")||!strcmp(b->xml.name,"h1")||!strcmp(b->xml.name,"h2"))) {
                if(b->xml.closing){if(title[0])break;label=false;}else label=true;
            }else if(kind==2&&label){ink_xml_entities(b->xml.text);append(title,sizeof(title),b->xml.text);}
        }
        tidy(title);rewind(b->toc);
        while(fread(&c,1,sizeof(c),b->toc)==sizeof(c)) {
            if(c.document==id&&!strcmp(c.title,fallback)&&title[0]) {
                long next=ftell(b->toc);snprintf(c.title,sizeof(c.title),"%s",title);
                if(fseek(b->toc,next-(long)sizeof(c),SEEK_SET)||fwrite(&c,1,sizeof(c),b->toc)!=sizeof(c)||
                   fseek(b->toc,next,SEEK_SET))goto done;
            }
        }
    }
    /* Avoid rescanning the entire book's TOC for each XHTML id. */
    FILE *local=scratch(b,"epub-local-toc","w+b");if(!local)goto done;
    rewind(b->toc);
    while(fread(&c,1,sizeof(c),b->toc)==sizeof(c))if(c.document==id) {
        if(fwrite(&c,1,sizeof(c),local)!=sizeof(c)){fclose(local);goto done;}
    }
    fclose(b->toc);b->toc=local;
    b->body=scratch(b,"epub-body","w+b");b->chapters=scratch(b,"epub-chapters","w+b");b->positions=scratch(b,"epub-positions","w+b");
    if(!b->body||!b->chapters||!b->positions||document(b,name,false)||fflush(b->body)||fflush(b->chapters))goto done;
    result=0;
done:
    if(result){snprintf(error,capacity,"Cannot read EPUB document %u",id);ink_epub_debug("document error=%u xml=%s",id,b->xml.name);}
    if(b->xml.file)fclose(b->xml.file);
    if(b->zip.file)fclose(b->zip.file);
    FILE *files[]={b->toc,b->spine,b->body,b->chapters,b->positions};
    for(unsigned i=0;i<5;i++)if(files[i])fclose(files[i]);
    free(b);phase="layout";
    return result;
}
int ink_epub_image(const char *source,const char *cache,const char *resource,char *out,size_t size,void (*progress)(void))
{
    uint64_t hash=1469598103934665603ULL;
    for(const unsigned char *p=(const unsigned char*)resource;*p;p++)hash=(hash^*p)*1099511628211ULL;
    char name[48];snprintf(name,sizeof(name),"epub-img-%016llx",(unsigned long long)hash);
    if(path(out,size,cache,name))return -1;
    struct stat st;
    if(!stat(out,&st)&&st.st_size)return 0;
    phase="image";ink_zip zip;
    if(ink_zip_open(&zip,source,progress))return -1;
    FILE *f=fopen(out,"wb");int result=f?ink_zip_extract(&zip,resource,f):-1;
    if(f&&fclose(f))result=-1;
    fclose(zip.file);
    if(result)unlink(out);
    ink_epub_debug("image extract status=%d resource=%s",result,resource);phase="layout";
    return result;
}

int ink_epub_anchor(const char *cache,unsigned chapter,uint64_t *offset)
{
    char p[640];
    if(path(p,sizeof(p),cache,"epub-positions"))return -1;
    FILE *f=fopen(p,"rb");
    if(!f)return -1;
    uint64_t record[2];int result=-1;
    while(fread(record,1,sizeof(record),f)==sizeof(record))if(record[0]==chapter){*offset=record[1];result=0;break;}
    fclose(f);
    return result;
}
