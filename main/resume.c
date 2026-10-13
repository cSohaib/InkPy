#include "resume.h"
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
static const char *saved="/sd/.inkpy-resume",*next="/sd/.inkpy-resume-next";
static uint32_t checksum(uint32_t value,const void *bytes,size_t size)
{
    const uint8_t *p=bytes;
    while(size--) {value^=*p++;value*=16777619u;}
    return value;
}
static uint32_t digest(const ink_resume *state,const uint8_t *frame)
{return checksum(checksum(2166136261u,state,sizeof(*state)),frame,48000);}
int ink_resume_save(const ink_resume *state,const uint8_t frame[48000])
{
    uint32_t header[]={0x49505246,sizeof(*state),digest(state,frame)};
    FILE *f=fopen(next,"wb");if(!f)return -1;
    bool ok=fwrite(header,1,sizeof(header),f)==sizeof(header)&&
        fwrite(state,1,sizeof(*state),f)==sizeof(*state)&&fwrite(frame,1,48000,f)==48000;
    if(fflush(f)||fsync(fileno(f)))ok=false;
    if(fclose(f))ok=false;
    if(!ok){unlink(next);return -1;}
    /* FatFs cannot replace destinations; next is complete before removing old. */
    if(unlink(saved)&&access(saved,F_OK)==0)return -1;
    return rename(next,saved);
}
int ink_resume_load(ink_resume *state,uint8_t frame[48000])
{
    FILE *f=fopen(saved,"rb");if(!f)f=fopen(next,"rb");
    if(!f)return -1;
    uint32_t header[3];struct stat st;
    bool ok=!fstat(fileno(f),&st)&&st.st_size==(off_t)(sizeof(header)+sizeof(*state)+48000)&&
        fread(header,1,sizeof(header),f)==sizeof(header)&&header[0]==0x49505246&&header[1]==sizeof(*state)&&
        fread(state,1,sizeof(*state),f)==sizeof(*state)&&fread(frame,1,48000,f)==48000&&
        header[2]==digest(state,frame)&&!state->editor.work;
    fclose(f);
    /* One shot: a failed file reopen must not create repeated restore crashes. */
    unlink(saved);unlink(next);
    return ok?0:-1;
}
