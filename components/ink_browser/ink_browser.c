#include "ink_browser.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

static int notice(ink_browser *b,const char *message)
{ snprintf(b->message,sizeof(b->message),"%s",message); b->view=INK_NOTICE; return -1; }
static int join(char *out,size_t capacity,const char *folder,const char *name)
{ int n=snprintf(out,capacity,"%s/%s",folder,name); return n<0||(size_t)n>=capacity?-1:0; }
int ink_browser_reload(ink_browser *b)
{
    b->count=0; b->has_next=false; b->view=INK_FILES;
    DIR *dir=opendir(b->folder);
    if(!dir) return notice(b,"Cannot open folder");
    struct dirent *entry; unsigned seen=0;
    while((entry=readdir(dir))) {
        if(entry->d_name[0]=='.') continue;
        char path[INK_BROWSER_PATH]; struct stat st;
        if(join(path,sizeof(path),b->folder,entry->d_name)||stat(path,&st)) continue;
        if(!S_ISDIR(st.st_mode)&&!S_ISREG(st.st_mode)) continue;
        if(seen++ < b->page*INK_BROWSER_ROWS) continue;
        if(b->count==INK_BROWSER_ROWS) { b->has_next=true; break; }
        ink_browser_entry *row=&b->rows[b->count++];
        snprintf(row->name,sizeof(row->name),"%s",entry->d_name); row->directory=S_ISDIR(st.st_mode);
    }
    closedir(dir); return 0;
}
int ink_browser_init(ink_browser *b,const char *root)
{
    memset(b,0,sizeof(*b));
    size_t n=strlen(root); while(n>1 && root[n-1]=='/') --n;
    if(!n || n>=sizeof(b->root)) return notice(b,"Invalid root path");
    memcpy(b->root,root,n); b->root[n]=0; memcpy(b->folder,b->root,n+1);
    return ink_browser_reload(b);
}
bool ink_browser_page(ink_browser *b,int direction)
{
    if(b->view!=INK_FILES) return false;
    if(direction>0) { if(!b->has_next) return false; ++b->page; }
    else { if(!b->page) return false; --b->page; }
    ink_browser_reload(b); return true;
}
bool ink_browser_home(ink_browser *b)
{
    if(b->view!=INK_FILES) { b->view=INK_FILES; return true; }
    if(!strcmp(b->folder,b->root)) return false;
    char *slash=strrchr(b->folder,'/');
    if(slash && (size_t)(slash-b->folder)>=strlen(b->root)) *slash=0;
    else snprintf(b->folder,sizeof(b->folder),"%s",b->root);
    b->page=0; ink_browser_reload(b); return true;
}
bool ink_browser_tap(ink_browser *b,unsigned x,unsigned y)
{
    if(x>=480 || y>=800) return false;
    if(b->view!=INK_FILES) return ink_browser_home(b);
    if(y>=48 && y<92) {
        notice(b,x<240?"New file: coming next":"Python console: coming later"); return true;
    }
    if(y<148 || y>=148+42*INK_BROWSER_ROWS) return false;
    unsigned row=(y-148)/42; if(row>=b->count) return false;
    if(join(b->selected,sizeof(b->selected),b->folder,b->rows[row].name)) {
        notice(b,"Path is too long"); return true;
    }
    if(b->rows[row].directory) {
        snprintf(b->folder,sizeof(b->folder),"%s",b->selected);
        b->page=0; ink_browser_reload(b); return true;
    }
    FILE *file=fopen(b->selected,"rb");
    if(!file) { notice(b,"Cannot open file"); return true; }
    /* Cheap first-block binary heuristic; full UTF-8 validation belongs to reader. */
    unsigned char block[256]; bool bad=false;
    for(unsigned remaining=4096;remaining && !bad;) {
        size_t n=fread(block,1,sizeof(block),file);
        for(size_t i=0;i<n;++i) if(block[i]==0 || (block[i]<32 && block[i]!='\n' && block[i]!='\r' && block[i]!='\t')) bad=true;
        remaining-=(unsigned)n;
        if(n<sizeof(block)) break;
    }
    bad|=ferror(file)!=0; fclose(file);
    if(bad) { notice(b,"Binary file or read error"); return true; }
    const char *extension=strrchr(b->rows[row].name,'.');
    b->view=extension && !strcasecmp(extension,".md")?INK_OPEN_MARKDOWN:INK_OPEN_TEXT;
    return true;
}
