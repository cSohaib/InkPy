#include "ink_python.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "port/micropython_embed.h"
#include "py/compile.h"
#include "py/reader.h"
#include "py/repl.h"
#include "py/runtime.h"
#include "py/stackctrl.h"
#ifdef INK_PY_NATIVE
#include "native.h"
void ink_python_listing_close(void);
#endif

static FILE *source_file;
static bool (*control_callback)(void *);
static void (*output_callback)(void *,const char *,size_t);
static void *callback_context;
void ink_python_callbacks(bool (*control)(void *),
    void (*output)(void *,const char *,size_t),void *context)
{
    control_callback=control; output_callback=output; callback_context=context;
}
void ink_python_poll(void)
{
    if(control_callback && control_callback(callback_context)) nlr_jump_abort();
}
void mp_hal_stdout_tx_strn_cooked(const char *str,size_t len)
{
    if(output_callback) output_callback(callback_context,str,len);
    else fwrite(str,1,len,stdout);
}
void ink_python_init(void *heap,size_t bytes,void *stack_top)
{
    mp_embed_init(heap,bytes,stack_top);
    mp_stack_set_limit(32*1024); /* Provisional budget; device worker reserves 48 KiB. */
#ifdef INK_PY_NATIVE
    mp_obj_list_append(mp_sys_path,mp_obj_new_str("/sd/lib",7));
#endif
}
static mp_uint_t read_byte(void *data)
{
    FILE *f=data; int c=fgetc(f);
    if(c==EOF && ferror(f)) mp_raise_OSError(EIO);
    return c==EOF?MP_READER_EOF:(mp_uint_t)c;
}
static void close_source(void *data)
{
    (void)data;
    if(source_file) { fclose(source_file); source_file=NULL; }
}
static void execute(mp_lexer_t *lex,bool repl)
{
    qstr name=lex->source_name;
    mp_parse_tree_t tree=mp_parse(lex,repl?MP_PARSE_SINGLE_INPUT:MP_PARSE_FILE_INPUT);
    mp_obj_t function=mp_compile(&tree,name,repl);
    mp_call_function_0(function);
}
int ink_python_file(const char *path)
{
    nlr_buf_t nlr;
    nlr_set_abort(&nlr);
    if(nlr_push(&nlr)==0) {
#ifdef INK_PY_NATIVE
        ink_python_directory(path);
#endif
        source_file=fopen(path,"rb");
        if(!source_file) mp_raise_OSError(errno);
        mp_reader_t reader={source_file,read_byte,close_source};
        execute(mp_lexer_new(qstr_from_str(path),reader),false);
        nlr_pop(); nlr_set_abort(NULL); close_source(NULL); return 0;
    }
    close_source(NULL);
    nlr_set_abort(NULL);
    if(!nlr.ret_val) return 2;
    mp_obj_print_exception(&mp_plat_print,(mp_obj_t)nlr.ret_val); return 1;
}
int ink_python_text(const char *source,bool repl)
{
    nlr_buf_t nlr;
    nlr_set_abort(&nlr);
    if(nlr_push(&nlr)==0) {
        execute(mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_,source,strlen(source),0),repl);
        nlr_pop(); nlr_set_abort(NULL); return 0;
    }
    nlr_set_abort(NULL);
    if(!nlr.ret_val) return 2;
    mp_obj_print_exception(&mp_plat_print,(mp_obj_t)nlr.ret_val); return 1;
}
bool ink_python_more(const char *source) { return mp_repl_continue_with_input(source); }
void ink_python_close(void) {
    close_source(NULL);
#ifdef INK_PY_NATIVE
    ink_python_network_close(); ink_python_listing_close(); ink_python_files_reset();
#endif
    mp_embed_deinit();
}
