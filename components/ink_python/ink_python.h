#pragma once
#include <stdbool.h>
#include <stddef.h>
/* One VM owner. Caller owns heap and supplies its worker's stack top.
 * Shared host/device embedding. main/python_worker provides the FreeRTOS adapter;
 * device console UI integration is still pending.
 * Execution returns 0 success, 1 exception, 2 stopped. After 2, close/reset the
 * session before accepting more input; abort does not run Python finally blocks. */
void ink_python_init(void *heap,size_t bytes,void *stack_top);
int ink_python_file(const char *path);
int ink_python_text(const char *source,bool repl);
bool ink_python_more(const char *source);
void ink_python_close(void);
/* Called only by the VM owner. Control callback may wait for resume and returns
 * true to stop. Output callback must copy bytes, never retain the pointer. */
void ink_python_callbacks(bool (*control)(void *),
    void (*output)(void *,const char *,size_t),void *context);
void ink_python_poll(void);

/* Complete-line input: callback blocks cooperatively until Enter, returns byte
 * count or -1 for EOF. It must continue checking ink_python_poll for Stop/sleep. */
void ink_python_input_callback(int (*read_line)(void *,char *,size_t));
