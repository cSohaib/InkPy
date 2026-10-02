#pragma once
#include <stdbool.h>
#include <stddef.h>
/* One VM owner. Caller owns heap and supplies its worker's stack top.
 * Host embedding prototype; device scheduling/lifecycle are not integrated yet. */
void ink_python_init(void *heap,size_t bytes,void *stack_top);
int ink_python_file(const char *path);
int ink_python_text(const char *source,bool repl);
bool ink_python_more(const char *source);
void ink_python_close(void);
