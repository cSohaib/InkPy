/* Single VM owner. FatFs/stdio owns per-call locking; no lock crosses a poll. */
#include "native.h"
#include "py/runtime.h"
#include "py/builtin.h"
#include "py/stream.h"
#include "py/lexer.h"
#include "py/reader.h"
#include "py/objlist.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#ifndef INK_PY_ROOT
#define INK_PY_ROOT "/sd"
#endif
static char cwd[512]=INK_PY_ROOT;
typedef struct { FILE *file; unsigned generation; } slot;
static slot files[8];
typedef struct { mp_obj_base_t base; unsigned slot,generation; } file_obj;
extern void ink_python_poll(void);
int ink_python_path(const char *name,char out[512])
{
    char joined[1024];
    int n=snprintf(joined,sizeof(joined),"%s%s%s",name[0]=='/'?"":cwd,name[0]=='/'?"":"/",name);
    if(n<0||(size_t)n>=sizeof(joined)) return ENAMETOOLONG;
    unsigned used=0; out[0]=0; const char *p=joined;
    while(*p) {
        while(*p=='/') p++;
        const char *start=p; while(*p&&*p!='/') p++; unsigned len=(unsigned)(p-start);
        if(!len||(len==1&&*start=='.')) continue;
        if(len==2&&start[0]=='.'&&start[1]=='.') { while(used&&out[used-1]!='/') used--; if(used) used--; out[used]=0; continue; }
        if(used+len+1>=512) return ENAMETOOLONG;
        out[used++]='/'; memcpy(out+used,start,len); used+=len; out[used]=0;
    }
    size_t root=strlen(INK_PY_ROOT);
    if(strncmp(out,INK_PY_ROOT,root)||(out[root]&&out[root]!='/')) return EACCES;
    /* Firmware's disposable indexes are never script data. */
    if(!strncmp(out+root,"/.inkpy-",8)) return EACCES;
    return 0;
}
static unsigned open_slot(const char *path,const char *mode)
{
    char resolved[512]; int e=ink_python_path(path,resolved); if(e) mp_raise_OSError(e);
    for(unsigned i=0;i<8;i++) if(!files[i].file) {
        FILE *f=fopen(resolved,mode); if(!f) mp_raise_OSError(errno);
        files[i].file=f; files[i].generation++; return i;
    }
    mp_raise_OSError(EMFILE);
}
static FILE *handle(file_obj *o)
{ return o->slot<8&&files[o->slot].generation==o->generation?files[o->slot].file:NULL; }
static mp_uint_t file_read(mp_obj_t self,void *buf,mp_uint_t size,int *err)
{
    ink_python_poll(); FILE *f=handle(MP_OBJ_TO_PTR(self));
    if(!f) { *err=EBADF; return MP_STREAM_ERROR; }
    if(size>4096) size=4096;
    size_t n=fread(buf,1,size,f);
    if(ferror(f)) { *err=errno?errno:EIO; return MP_STREAM_ERROR; } return n;
}
static mp_uint_t file_write(mp_obj_t self,const void *buf,mp_uint_t size,int *err)
{
    ink_python_poll(); FILE *f=handle(MP_OBJ_TO_PTR(self));
    if(!f) { *err=EBADF; return MP_STREAM_ERROR; }
    if(size>4096) size=4096;
    size_t n=fwrite(buf,1,size,f);
    if(n!=size) { *err=errno?errno:EIO; return MP_STREAM_ERROR; } return n;
}
static mp_uint_t file_ioctl(mp_obj_t self,mp_uint_t op,uintptr_t arg,int *err)
{
    file_obj *o=MP_OBJ_TO_PTR(self); FILE *f=handle(o);
    if(op==MP_STREAM_CLOSE) {
        if(!f) return 0;
        files[o->slot].file=NULL;
        if(fclose(f)) { *err=errno; return MP_STREAM_ERROR; } return 0;
    }
    if(!f) { *err=EBADF; return MP_STREAM_ERROR; }
    if(op==MP_STREAM_FLUSH) { if(!fflush(f)) return 0; *err=errno; }
    else if(op==MP_STREAM_SEEK) {
        struct mp_stream_seek_t *seek=(void *)arg;
        if(!fseek(f,seek->offset,seek->whence)) { long pos=ftell(f); if(pos>=0) { seek->offset=pos; return 0; } }
        *err=errno;
    } else *err=EINVAL;
    return MP_STREAM_ERROR;
}
static const mp_rom_map_elem_t file_methods_table[]={
    {MP_ROM_QSTR(MP_QSTR_read),MP_ROM_PTR(&mp_stream_read_obj)},
    {MP_ROM_QSTR(MP_QSTR_readinto),MP_ROM_PTR(&mp_stream_readinto_obj)},
    {MP_ROM_QSTR(MP_QSTR_readline),MP_ROM_PTR(&mp_stream_unbuffered_readline_obj)},
    {MP_ROM_QSTR(MP_QSTR_readlines),MP_ROM_PTR(&mp_stream_unbuffered_readlines_obj)},
    {MP_ROM_QSTR(MP_QSTR_write),MP_ROM_PTR(&mp_stream_write_obj)},
    {MP_ROM_QSTR(MP_QSTR_seek),MP_ROM_PTR(&mp_stream_seek_obj)},
    {MP_ROM_QSTR(MP_QSTR_tell),MP_ROM_PTR(&mp_stream_tell_obj)},
    {MP_ROM_QSTR(MP_QSTR_flush),MP_ROM_PTR(&mp_stream_flush_obj)},
    {MP_ROM_QSTR(MP_QSTR_close),MP_ROM_PTR(&mp_stream_close_obj)},
    {MP_ROM_QSTR(MP_QSTR___del__),MP_ROM_PTR(&mp_stream_close_obj)},
    {MP_ROM_QSTR(MP_QSTR___enter__),MP_ROM_PTR(&mp_identity_obj)},
    {MP_ROM_QSTR(MP_QSTR___exit__),MP_ROM_PTR(&mp_stream___exit___obj)},
};
static MP_DEFINE_CONST_DICT(file_methods,file_methods_table);
static const mp_stream_p_t text_stream={.read=file_read,.write=file_write,.ioctl=file_ioctl,.is_text=1};
static const mp_stream_p_t binary_stream={.read=file_read,.write=file_write,.ioctl=file_ioctl};
static MP_DEFINE_CONST_OBJ_TYPE(text_file,MP_QSTR_TextIOWrapper,MP_TYPE_FLAG_ITER_IS_STREAM,
    protocol,&text_stream,locals_dict,&file_methods);
static MP_DEFINE_CONST_OBJ_TYPE(binary_file,MP_QSTR_FileIO,MP_TYPE_FLAG_ITER_IS_STREAM,
    protocol,&binary_stream,locals_dict,&file_methods);
mp_obj_t mp_builtin_open(size_t n_args,const mp_obj_t *args,mp_map_t *kwargs)
{
    enum { ARG_file,ARG_mode };
    static const mp_arg_t allowed[]={
        {MP_QSTR_file,MP_ARG_REQUIRED|MP_ARG_OBJ,{.u_obj=MP_OBJ_NULL}},
        {MP_QSTR_mode,MP_ARG_OBJ,{.u_obj=MP_OBJ_NEW_QSTR(MP_QSTR_r)}}};
    mp_arg_val_t values[2]; mp_arg_parse_all(n_args,args,kwargs,2,allowed,values);
    const char *mode=mp_obj_str_get_str(values[ARG_mode].u_obj);
    bool binary=false,plus=false;
    if(!strchr("rwa",mode[0])||!mode[0]) mp_raise_ValueError(MP_ERROR_TEXT("mode must be r, w or a"));
    for(unsigned i=1;mode[i];i++) {
        if(mode[i]=='b'&&!binary) binary=true;
        else if(mode[i]=='+'&&!plus) plus=true;
        else if(mode[i]!='t') mp_raise_ValueError(MP_ERROR_TEXT("invalid mode"));
    }
    file_obj *o=mp_obj_malloc_with_finaliser(file_obj,binary?&binary_file:&text_file);
    o->slot=8; char native_mode[]={mode[0],'b',plus?'+':0,0};
    o->slot=open_slot(mp_obj_str_get_str(values[ARG_file].u_obj),native_mode);
    o->generation=files[o->slot].generation; return MP_OBJ_FROM_PTR(o);
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj,1,mp_builtin_open);
mp_import_stat_t mp_import_stat(const char *name)
{
    char p[512]; struct stat st;
    if(ink_python_path(name,p)||stat(p,&st)) return MP_IMPORT_STAT_NO_EXIST;
    return S_ISDIR(st.st_mode)?MP_IMPORT_STAT_DIR:S_ISREG(st.st_mode)?MP_IMPORT_STAT_FILE:MP_IMPORT_STAT_NO_EXIST;
}
static mp_uint_t reader_byte(void *data)
{
    file_obj *o=data; FILE *f=handle(o); ink_python_poll();
    int c=f?fgetc(f):EOF;
    if(f&&ferror(f)) mp_raise_OSError(EIO);
    return c==EOF?MP_READER_EOF:(mp_uint_t)c;
}
static void reader_close(void *data)
{ int err; file_ioctl(MP_OBJ_FROM_PTR(data),MP_STREAM_CLOSE,0,&err); }
mp_lexer_t *mp_lexer_new_from_file(qstr name)
{
    file_obj *o=mp_obj_malloc_with_finaliser(file_obj,&binary_file); o->slot=8;
    o->slot=open_slot(qstr_str(name),"rb"); o->generation=files[o->slot].generation;
    mp_reader_t reader={o,reader_byte,reader_close}; return mp_lexer_new(name,reader);
}
void ink_python_files_reset(void)
{
    for(unsigned i=0;i<8;i++) if(files[i].file) { fclose(files[i].file); files[i].file=NULL; }
    strcpy(cwd,INK_PY_ROOT);
}
void ink_python_directory(const char *script)
{
    char p[512]; if(ink_python_path(script,p)) return;
    char *slash=strrchr(p,'/'); if(!slash) return; *slash=0; strcpy(cwd,p);
    mp_obj_t list=mp_sys_path;
    /* First entry follows the current script; /sd/lib remains a fixed second entry. */
    mp_obj_list_t *paths=MP_OBJ_TO_PTR(list);
    if(paths->len) paths->items[0]=mp_obj_new_str(cwd,strlen(cwd));
}
static void resolve(mp_obj_t name,char p[512])
{ int e=ink_python_path(mp_obj_str_get_str(name),p); if(e) mp_raise_OSError(e); }
static mp_obj_t os_getcwd(void) { return mp_obj_new_str(cwd,strlen(cwd)); }
static MP_DEFINE_CONST_FUN_OBJ_0(os_getcwd_obj,os_getcwd);
static mp_obj_t os_chdir(mp_obj_t name) { char p[512]; resolve(name,p); struct stat st; if(stat(p,&st)||!S_ISDIR(st.st_mode)) mp_raise_OSError(ENOENT); strcpy(cwd,p); return mp_const_none; }
static MP_DEFINE_CONST_FUN_OBJ_1(os_chdir_obj,os_chdir);
static mp_obj_t os_remove(mp_obj_t name) { char p[512]; resolve(name,p); if(unlink(p)) mp_raise_OSError(errno); return mp_const_none; }
static MP_DEFINE_CONST_FUN_OBJ_1(os_remove_obj,os_remove);
static mp_obj_t os_mkdir(mp_obj_t name) { char p[512]; resolve(name,p); if(mkdir(p,0700)) mp_raise_OSError(errno); return mp_const_none; }
static MP_DEFINE_CONST_FUN_OBJ_1(os_mkdir_obj,os_mkdir);
static mp_obj_t os_rmdir(mp_obj_t name) { char p[512]; resolve(name,p); if(rmdir(p)) mp_raise_OSError(errno); return mp_const_none; }
static MP_DEFINE_CONST_FUN_OBJ_1(os_rmdir_obj,os_rmdir);
static mp_obj_t os_rename(mp_obj_t a,mp_obj_t b) { char p[512],q[512]; resolve(a,p); resolve(b,q); if(rename(p,q)) mp_raise_OSError(errno); return mp_const_none; }
static MP_DEFINE_CONST_FUN_OBJ_2(os_rename_obj,os_rename);
static mp_obj_t os_stat(mp_obj_t name) {
    char p[512]; resolve(name,p); struct stat st; if(stat(p,&st)) mp_raise_OSError(errno);
    mp_obj_t items[10]={mp_obj_new_int(st.st_mode),mp_obj_new_int(0),mp_obj_new_int(0),mp_obj_new_int(0),mp_obj_new_int(0),mp_obj_new_int(0),mp_obj_new_int_from_ll(st.st_size),mp_obj_new_int(st.st_atime),mp_obj_new_int(st.st_mtime),mp_obj_new_int(st.st_ctime)};
    return mp_obj_new_tuple(10,items);
}
static MP_DEFINE_CONST_FUN_OBJ_1(os_stat_obj,os_stat);
static DIR *listing;
static mp_obj_t os_listdir(size_t n,const mp_obj_t *args) {
    char p[512]; if(n) resolve(args[0],p); else strcpy(p,cwd);
    listing=opendir(p); if(!listing) mp_raise_OSError(errno);
    mp_obj_t result=mp_obj_new_list(0,NULL); struct dirent *e;
    while((e=readdir(listing))) { ink_python_poll(); if(strcmp(e->d_name,".")&&strcmp(e->d_name,"..")) mp_obj_list_append(result,mp_obj_new_str(e->d_name,strlen(e->d_name))); }
    closedir(listing); listing=NULL; return result;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(os_listdir_obj,0,1,os_listdir);
void ink_python_listing_close(void) { if(listing) { closedir(listing); listing=NULL; } }
static const mp_rom_map_elem_t os_globals_table[]={
    {MP_ROM_QSTR(MP_QSTR___name__),MP_ROM_QSTR(MP_QSTR_os)},
    {MP_ROM_QSTR(MP_QSTR_getcwd),MP_ROM_PTR(&os_getcwd_obj)},
    {MP_ROM_QSTR(MP_QSTR_chdir),MP_ROM_PTR(&os_chdir_obj)},
    {MP_ROM_QSTR(MP_QSTR_listdir),MP_ROM_PTR(&os_listdir_obj)},
    {MP_ROM_QSTR(MP_QSTR_mkdir),MP_ROM_PTR(&os_mkdir_obj)},
    {MP_ROM_QSTR(MP_QSTR_rmdir),MP_ROM_PTR(&os_rmdir_obj)},
    {MP_ROM_QSTR(MP_QSTR_remove),MP_ROM_PTR(&os_remove_obj)},
    {MP_ROM_QSTR(MP_QSTR_rename),MP_ROM_PTR(&os_rename_obj)},
    {MP_ROM_QSTR(MP_QSTR_stat),MP_ROM_PTR(&os_stat_obj)},
};
static MP_DEFINE_CONST_DICT(os_globals,os_globals_table);
const mp_obj_module_t ink_os={.base={&mp_type_module},.globals=(mp_obj_dict_t *)&os_globals};
MP_REGISTER_MODULE(MP_QSTR_os,ink_os);
MP_REGISTER_MODULE(MP_QSTR_uos,ink_os);
