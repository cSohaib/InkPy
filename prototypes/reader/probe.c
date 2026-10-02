#define _POSIX_C_SOURCE 200809L
#include "ink_layout.h"
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <unistd.h>

static int path(char *out,size_t n,const char *dir,const char *name)
{ int r=snprintf(out,n,"%s/%s",dir,name); return r<0||(size_t)r>=n?-1:0; }
int main(int argc,char **argv)
{
    if (argc!=3 && argc!=6) { fprintf(stderr,"usage: reader-probe SOURCE NEW_OUTPUT_DIR [WIDTH HEIGHT READ_BYTES]\n"); return 2; }
    ink_layout_config cfg={480,800,22,1024};
    if (argc==6) {
        unsigned *values[]={&cfg.width,&cfg.height,&cfg.read_bytes};
        for (int i=0;i<3;++i) {
            char *end; unsigned long v=strtoul(argv[i+3],&end,10);
            if (!*argv[i+3] || *end || v>1024) return 2;
            *values[i]=(unsigned)v;
        }
    }
    FILE *in=fopen(argv[1],"rb");
    if (!in) { perror("source"); return 2; }
    struct stat before,after;
    if (fstat(fileno(in),&before) || !S_ISREG(before.st_mode)) { fclose(in); return 2; }
    if (mkdir(argv[2],0700)) { perror("output must be a new directory"); fclose(in); return 2; }
    const char *names[]={"draw.bin","pages.bin","chapters.bin"};
    FILE *files[3]={0}; char paths[3][4096],dest[4096];
    for (unsigned i=0;i<3;++i) {
        char part[32]; snprintf(part,sizeof(part),"%s.part",names[i]);
        if (path(paths[i],sizeof(paths[i]),argv[2],part) || !(files[i]=fopen(paths[i],"wb"))) {
            perror("cache"); for (unsigned j=0;j<i;++j) fclose(files[j]); fclose(in); return 2;
        }
    }
    ink_layout_stats stats;
    int result=ink_layout_run(in,files[0],files[1],files[2],&cfg,&stats);
    if (fstat(fileno(in),&after) || before.st_size!=after.st_size ||
        before.st_mtim.tv_sec!=after.st_mtim.tv_sec || before.st_mtim.tv_nsec!=after.st_mtim.tv_nsec ||
        before.st_ctim.tv_sec!=after.st_ctim.tv_sec || before.st_ctim.tv_nsec!=after.st_ctim.tv_nsec) {
        result=-1; snprintf(stats.error,sizeof(stats.error),"source changed while indexing");
    }
    fclose(in);
    for (unsigned i=0;i<3;++i) if (fclose(files[i])) { result=-1; snprintf(stats.error,sizeof(stats.error),"cache close failed"); }
    if (result) { fprintf(stderr,"%s at byte %" PRIu64 "\n",stats.error,stats.error_offset); return 1; }
    for (unsigned i=0;i<3;++i) {
        if (path(dest,sizeof(dest),argv[2],names[i]) || rename(paths[i],dest)) { perror("publish cache"); return 1; }
    }
    struct rusage usage; getrusage(RUSAGE_SELF,&usage);
    char manifest[1024];
    snprintf(manifest,sizeof(manifest),"{\"version\":1,\"complete\":true,\"width\":%u,\"height\":%u,\"font_pixels\":%u,"
        "\"source_bytes\":%" PRIu64 ",\"pages\":%" PRIu64 ",\"chapters\":%" PRIu64 ",\"literal_blocks\":%" PRIu64
        ",\"runs\":%" PRIu64 ",\"context_bytes\":%zu,\"parser_peak_bytes\":%zu,\"parser_budget_bytes\":%u,\"peak_rss_kib\":%ld}\n",
        cfg.width,cfg.height,cfg.font_pixels,stats.source_bytes,stats.pages,stats.chapters,
        stats.literal_blocks,stats.runs,stats.context_bytes,stats.parser_peak_bytes,INK_MD_HEAP_BYTES,usage.ru_maxrss);
    if (path(dest,sizeof(dest),argv[2],"manifest.json")) return 1;
    FILE *out=fopen(dest,"wb"); if (!out) return 1;
    int bad=fputs(manifest,out)==EOF;
    if (fclose(out)) bad=1;
    if (bad) { unlink(dest); return 1; }
    fputs(manifest,stdout); return 0;
}
