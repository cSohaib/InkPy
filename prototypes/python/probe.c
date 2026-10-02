#include "ink_python.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
/* Provisional host VM heap. The API lets firmware supply its own PSRAM heap. */
static union { max_align_t align; unsigned char bytes[256*1024]; } heap;
static int console(void)
{
    char input[4096]={0}; size_t used=0;
    puts("InkPy MicroPython console; EOF closes, blank line finishes a block.");
    for(;;) {
        fputs(used?"... ":">>> ",stdout); fflush(stdout);
        if(used>=sizeof(input)-2) {
            puts("Console input limit is 4095 bytes"); input[0]=0; used=0; continue;
        }
        if(used) { input[used++]='\n'; input[used]=0; }
        if(!fgets(input+used,sizeof(input)-used,stdin)) {
            if(used) ink_python_text(input,true);
            break;
        }
        size_t length=strlen(input);
        if(length==sizeof(input)-1 && input[length-1]!='\n') {
            int c; while((c=fgetc(stdin))!=EOF && c!='\n') {}
            puts("Console input limit is 4095 bytes"); input[0]=0; used=0; continue;
        }
        /* Trim this line only; preserve the separator for a blank continuation. */
        while(length>used && (input[length-1]=='\n' || input[length-1]=='\r')) input[--length]=0;
        used=length;
        if(!ink_python_more(input)) { ink_python_text(input,true); input[0]=0; used=0; }
    }
    return 0;
}
int main(int argc,char **argv)
{
    bool interactive=argc==1 || (argc==3 && !strcmp(argv[1],"-i"));
    if(argc>3 || (argc==3 && !interactive)) {
        fputs("usage: inkpy-python [SCRIPT.py | -i SCRIPT.py]\n",stderr); return 2;
    }
    int stack_top;
    ink_python_init(heap.bytes,sizeof(heap.bytes),&stack_top);
    int result=0;
    if(argc>1) result=ink_python_file(argv[argc-1]);
    if(interactive) console();
    ink_python_close(); return result;
}
