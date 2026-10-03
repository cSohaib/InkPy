#include "ink_python.h"
#include "native.h"
#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
static union { max_align_t alignment; unsigned char bytes[256*1024]; } heap;
static bool stop;
static bool control(void *unused) { (void)unused; return stop; }
static int input_count;
static int line_input(void *context,char *value,size_t capacity)
{
    (void)context; ink_python_poll();
    const char *answers[]={"hello world","","42"};
    if(input_count==3) return -1;
    const char *answer=answers[input_count++];
    assert(strlen(answer)<capacity); strcpy(value,answer); return (int)strlen(answer);
}
int main(int argc,char **argv)
{
    assert(argc==2); int top;
    ink_python_callbacks(control,NULL,NULL); ink_python_init(heap.bytes,sizeof(heap.bytes),&top);
    assert(!ink_python_file(argv[1]));
    ink_python_input_callback(line_input); input_count=0;
    assert(!ink_python_text("assert input(\"Value: \") == \"hello world\"\nassert input() == \"\"\nassert int(input()) == 42",false));
    assert(ink_python_text("input()",false)==1);
    assert(!ink_python_text("held = [open('text.txt') for _ in range(8)]",false));
    stop=true; assert(ink_python_text("input()",false)==2);
    assert(ink_python_text("while True: pass",false)==2);
    ink_python_close(); stop=false;
    ink_python_init(heap.bytes,sizeof(heap.bytes),&top);
    assert(!ink_python_file(argv[1]));
    ink_python_close();
    puts("PASS: native configuration, SD streams/imports/JSON/os/time, cleanup after abort and reopen");
}
