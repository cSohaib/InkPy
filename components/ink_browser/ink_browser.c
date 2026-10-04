#include "ink_browser.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

static int notice(ink_browser *b,const char *message)
{ snprintf(b->message,sizeof(b->message),"%s",message); b->view=INK_NOTICE; return -1; }
static int join(char *out,size_t capacity,const char *folder,const char *name)
{ int n=snprintf(out,capacity,"%s/%s",folder,name); return n<0||(size_t)n>=capacity?-1:0; }
static int entry_compare(const ink_browser_entry *a,const ink_browser_entry *b)
{
    if(a->directory!=b->directory) return a->directory?-1:1;
    int result=strcasecmp(a->name,b->name);
    return result?result:strcmp(a->name,b->name);
}
int ink_browser_reload(ink_browser *b)
{
    b->count=0; b->has_next=false; b->view=INK_FILES;
    DIR *dir=opendir(b->folder);
    if(!dir) return notice(b,"Cannot open folder");
    /* Select one sorted page per scan: fixed RAM regardless of directory size.
       Later pages rescan preceding ranges rather than keeping a whole index. */
    ink_browser_entry anchor={0}; bool anchored=false;
    for(unsigned page=0;page<=b->page;page++) {
        b->count=0; b->has_next=false;
        struct dirent *entry;
        while((entry=readdir(dir))) {
            if(entry->d_name[0]=='.') continue;
            char path[INK_BROWSER_PATH]; struct stat st;
            if(join(path,sizeof(path),b->folder,entry->d_name)||stat(path,&st)) continue;
            if(!S_ISDIR(st.st_mode)&&!S_ISREG(st.st_mode)) continue;
            ink_browser_entry item={.directory=S_ISDIR(st.st_mode)};
            snprintf(item.name,sizeof(item.name),"%s",entry->d_name);
            if(anchored&&entry_compare(&item,&anchor)<=0) continue;
            unsigned at=0;
            while(at<b->count&&entry_compare(&b->rows[at],&item)<0) at++;
            if(b->count==INK_BROWSER_ROWS) b->has_next=true;
            if(at==INK_BROWSER_ROWS) continue;
            if(b->count<INK_BROWSER_ROWS) b->count++;
            memmove(&b->rows[at+1],&b->rows[at],(b->count-at-1)*sizeof(item));
            b->rows[at]=item;
        }
        if(page==b->page||!b->has_next) { b->page=page; break; }
        anchor=b->rows[b->count-1]; anchored=true; rewinddir(dir);
    }
    closedir(dir); return 0;
}
void ink_browser_root(ink_browser *b)
{
    snprintf(b->folder,sizeof(b->folder),"%s",b->root);
    b->page=0; b->selected[0]=b->new_name[0]=b->message[0]=0;
    ink_browser_reload(b);
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
    if(b->view==INK_OPEN_TEXT) {
        int changed=ink_text_turn(&b->text,b->selected,direction);
        if(changed<0) notice(b,b->text.error);
        return changed!=0;
    }
    if(b->view!=INK_FILES) return false;
    if(direction>0) { if(!b->has_next) return false; ++b->page; }
    else { if(!b->page) return false; --b->page; }
    ink_browser_reload(b); return true;
}
bool ink_browser_home(ink_browser *b)
{
    if(b->view!=INK_FILES) { ink_browser_reload(b); return true; }
    if(!strcmp(b->folder,b->root)) return false;
    char *slash=strrchr(b->folder,'/');
    if(slash && (size_t)(slash-b->folder)>=strlen(b->root)) *slash=0;
    else snprintf(b->folder,sizeof(b->folder),"%s",b->root);
    b->page=0; ink_browser_reload(b); return true;
}
static bool create_file(ink_browser *b)
{
    size_t n=strlen(b->new_name);
    if(!n || !strcmp(b->new_name,".") || !strcmp(b->new_name,"..") ||
       b->new_name[n-1]==' ' || b->new_name[n-1]=='.' || strpbrk(b->new_name,"/\\:*?\"<>|")) {
        snprintf(b->message,sizeof(b->message),"Invalid filename"); return true;
    }
    if(join(b->selected,sizeof(b->selected),b->folder,b->new_name)) {
        snprintf(b->message,sizeof(b->message),"Path is too long"); return true;
    }
    int fd=open(b->selected,O_WRONLY|O_CREAT|O_EXCL,0666);
    if(fd<0) {
        snprintf(b->message,sizeof(b->message),"%s",errno==EEXIST?"Already exists":"Cannot create file"); return true;
    }
    if(close(fd)) { snprintf(b->message,sizeof(b->message),"Create close failed"); return true; }
    b->page=0; ink_browser_reload(b); b->view=INK_EDIT_TEXT; return true;
}
static bool filename_tap(ink_browser *b,unsigned x,unsigned y)
{
    if(y>=720 && y<776 && x>=16 && x<464)
        return x<240?ink_browser_home(b):create_file(b);
    int key=ink_keyboard_tap(&b->keyboard,x,y);
    if(!key) return false;
    if(key==INK_KEY_ENTER) return create_file(b);
    size_t n=strlen(b->new_name);
    b->message[0]=0;
    if(key==INK_KEY_DELETE) { if(n) b->new_name[n-1]=0; }
    else if(key>=32 && key<=126) {
        if(n+1<sizeof(b->new_name)) { b->new_name[n]=(char)key; b->new_name[n+1]=0; }
        else snprintf(b->message,sizeof(b->message),"Filename is too long");
    }
    return true;
}
bool ink_browser_tap(ink_browser *b,unsigned x,unsigned y)
{
    if(x>=480 || y>=800) return false;
    if(b->view==INK_FILE_MENU) {
        if(x<8 || x>=472 || y<140 || y>=380) return false;
        if(y>=300) {
            struct stat st;
            if(stat(b->selected,&st)||!S_ISREG(st.st_mode)||unlink(b->selected)) notice(b,"Cannot delete file");
            else if(!ink_browser_reload(b)&&!b->count&&b->page) { b->page--; ink_browser_reload(b); }
        } else if(y>=220) {
            const char *name=strrchr(b->selected,'/'); name=name?name+1:b->selected;
            const char *ext=strrchr(name,'.');
            if(!ext || strcasecmp(ext,".py")) notice(b,"not executable");
            else b->view=INK_EXECUTE_PYTHON;
        } else {
            if(ink_text_open(&b->text,b->selected)) notice(b,b->text.error);
            else b->view=INK_EDIT_TEXT;
        }
        return true;
    }
    if(b->view==INK_NEW_FILE) return filename_tap(b,x,y);
    if(b->view==INK_OPEN_TEXT) return false;
    if(b->view!=INK_FILES) return ink_browser_home(b);
    if(y>=INK_BROWSER_ACTION_Y && y<800) {
        if(x<240) {
            b->new_name[0]=b->message[0]=0; b->keyboard=(ink_keyboard){0}; b->view=INK_NEW_FILE;
        } else { b->selected[0]=0; b->view=INK_OPEN_CONSOLE; }
        return true;
    }
    if(y<INK_BROWSER_LIST_Y || y>=INK_BROWSER_LIST_Y+INK_BROWSER_ROW_HEIGHT*INK_BROWSER_ROWS) return false;
    unsigned row=(y-INK_BROWSER_LIST_Y)/INK_BROWSER_ROW_HEIGHT; if(row>=b->count) return false;
    if(join(b->selected,sizeof(b->selected),b->folder,b->rows[row].name)) {
        notice(b,"Path is too long"); return true;
    }
    if(b->rows[row].directory) {
        snprintf(b->folder,sizeof(b->folder),"%s",b->selected);
        b->page=0; ink_browser_reload(b); return true;
    }
    const char *ext=strrchr(b->rows[row].name,'.');
    if(ext&&!strcasecmp(ext,".epub")){b->view=INK_OPEN_MARKDOWN;return true;}
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
    b->view=extension && (!strcasecmp(extension,".md")||!strcasecmp(extension,".epub"))?INK_OPEN_MARKDOWN:INK_OPEN_TEXT;
    if(b->view==INK_OPEN_TEXT && ink_text_open(&b->text,b->selected)) notice(b,b->text.error);
    return true;
}
bool ink_browser_long_press(ink_browser *b,unsigned x,unsigned y)
{
    if(b->view!=INK_FILES || x>=480 || y<INK_BROWSER_LIST_Y || y>=INK_BROWSER_LIST_Y+INK_BROWSER_ROW_HEIGHT*INK_BROWSER_ROWS) return false;
    unsigned row=(y-INK_BROWSER_LIST_Y)/INK_BROWSER_ROW_HEIGHT;
    if(row>=b->count || b->rows[row].directory) return false;
    if(join(b->selected,sizeof(b->selected),b->folder,b->rows[row].name)) {
        notice(b,"Path is too long"); return true;
    }
    b->view=INK_FILE_MENU; return true;
}
