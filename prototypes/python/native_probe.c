#include "ink_python.h"
#include "native.h"
#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
static union { max_align_t alignment; unsigned char bytes[256*1024]; } heap;
static bool stop;
static bool control(void *unused) { (void)unused; return stop; }
int main(int argc,char **argv)
{
    assert(argc==2); int top;
    ink_python_callbacks(control,NULL,NULL); ink_python_init(heap.bytes,sizeof(heap.bytes),&top);
    assert(!ink_python_file(argv[1]));
    assert(!ink_python_text("held = [open('text.txt') for _ in range(8)]",false));
    stop=true; assert(ink_python_text("while True: pass",false)==2);
    ink_python_close(); stop=false;
    ink_python_init(heap.bytes,sizeof(heap.bytes),&top);
    assert(!ink_python_file(argv[1]));
    ink_python_close();
    puts("PASS: native configuration, SD streams/imports/JSON/os/time, cleanup after abort and reopen");
}
