#include "ink_layout.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void equal(FILE *a,FILE *b)
{
    rewind(a);rewind(b);int x,y;
    do{x=fgetc(a);y=fgetc(b);assert(x==y);}while(x!=EOF);
}
int main(int argc,char **argv)
{
    assert(argc==2);FILE *input=fopen(argv[1],"rb");assert(input);
    FILE *full[3],*part[3];
    for(unsigned i=0;i<3;i++){full[i]=tmpfile();part[i]=tmpfile();assert(full[i]&&part[i]);}
    FILE *table=tmpfile(),*state=tmpfile();assert(table&&state);
    ink_layout_config cfg={.width=480,.height=800,.font_pixels=24,.read_bytes=73,.table_spool=table};
    ink_layout_stats baseline,stats;
    assert(!ink_layout_run(input,full[0],full[1],full[2],&cfg,&baseline));rewind(input);
    void *l=ink_layout_begin(input,part[0],part[1],part[2],&cfg,&stats);assert(l);int result;
    do {
        result=ink_layout_step(l);assert(result>=0);
        rewind(state);assert(!ink_layout_save(l,state));ink_layout_end(l);rewind(state);
        l=ink_layout_restore(state,input,part[0],part[1],part[2],&cfg,&stats);assert(l);
    }while(!result);
    ink_layout_end(l);assert(stats.pages==baseline.pages);
    for(unsigned i=0;i<3;i++){equal(full[i],part[i]);fclose(full[i]);fclose(part[i]);}
    fclose(input);fclose(table);fclose(state);
    puts("PASS: checkpoint/restore at every step matches uninterrupted draw/page/chapter output byte-for-byte");
}
