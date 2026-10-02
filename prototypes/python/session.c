#include "session.h"
#include "ink_python.h"
#include <string.h>
#include <strings.h>
static bool control(void *context)
{
    ink_session *s=context;
    pthread_mutex_lock(&s->lock);
    while(s->pause && !s->stop && !s->closing) {
        s->paused=true;
        pthread_cond_broadcast(&s->changed);
        pthread_cond_wait(&s->changed,&s->lock);
    }
    s->paused=false;
    bool stop=s->stop || s->closing;
    pthread_cond_broadcast(&s->changed);
    pthread_mutex_unlock(&s->lock);
    return stop;
}
static void output(void *context,const char *bytes,size_t length)
{
    ink_session *s=context;
    pthread_mutex_lock(&s->lock);
    ink_console_output(&s->console,bytes,length);
    pthread_mutex_unlock(&s->lock);
}
static void *worker(void *context)
{
    ink_session *s=context; int stack_top;
    ink_python_callbacks(control,output,s);
    ink_python_init(s->heap,s->heap_size,&stack_top);
    for(;;) {
        pthread_mutex_lock(&s->lock);
        while(!s->closing && (s->pause || !s->queued)) {
            s->paused=s->pause;
            pthread_cond_broadcast(&s->changed);
            pthread_cond_wait(&s->changed,&s->lock);
        }
        s->paused=false;
        if(s->closing) { pthread_mutex_unlock(&s->lock); break; }
        s->queued=false;
        pthread_cond_broadcast(&s->changed);
        pthread_mutex_unlock(&s->lock);
        bool more=!s->file_job && ink_python_more(s->command);
        int result=s->file_job?ink_python_file(s->path):more?0:ink_python_text(s->command,true);
        if(result==2) {
            /* Hard stop discards globals: never reuse a partially unwound VM. */
            ink_python_close();
            ink_python_init(s->heap,s->heap_size,&stack_top);
            more=false;
        }
        pthread_mutex_lock(&s->lock);
        if(result==2) ink_console_output(&s->console,"Stopped; Python reset\n",22);
        ink_console_completed(&s->console,more);
        s->stop=false;
        pthread_cond_broadcast(&s->changed);
        pthread_mutex_unlock(&s->lock);
    }
    ink_python_close();
    pthread_mutex_lock(&s->lock);
    s->closed=true; s->paused=false; s->console.busy=false;
    pthread_cond_broadcast(&s->changed);
    pthread_mutex_unlock(&s->lock);
    return NULL;
}
int ink_session_start(ink_session *s,void *heap,size_t size)
{
    memset(s,0,sizeof(*s)); s->heap=heap; s->heap_size=size;
    ink_console_init(&s->console);
    int error=pthread_mutex_init(&s->lock,NULL); if(error) return error;
    error=pthread_cond_init(&s->changed,NULL);
    if(error) { pthread_mutex_destroy(&s->lock); return error; }
    error=pthread_create(&s->thread,NULL,worker,s);
    if(error) { pthread_cond_destroy(&s->changed); pthread_mutex_destroy(&s->lock); }
    return error;
}
void ink_session_submit(ink_session *s)
{
    s->file_job=false;
    ink_console *c=&s->console;
    ink_console_output(c,c->more?"... ":">>> ",4);
    const char *last=strrchr(c->input,'\n'); last=last?last+1:c->input;
    ink_console_output(c,last,strlen(last)); ink_console_output(c,"\n",1);
    memcpy(s->command,c->input,c->used+1); s->queued=true;
    pthread_cond_broadcast(&s->changed);
}
bool ink_session_file(ink_session *s,const char *path)
{
    if(s->console.busy || s->closing || s->closed) return false;
    const char *name=strrchr(path,'/'); name=name?name+1:path;
    const char *ext=strrchr(name,'.');
    const char *error=(!ext || strcasecmp(ext,".py"))?"not executable\n":
        strlen(path)>=sizeof(s->path)?"Path is too long\n":NULL;
    if(error) { ink_console_output(&s->console,error,strlen(error)); return false; }
    memcpy(s->path,path,strlen(path)+1); s->file_job=true;
    ink_console_completed(&s->console,false);
    s->console.busy=true; s->console.page=0;
    ink_console_output(&s->console,"Run: ",5);
    ink_console_output(&s->console,name,strlen(name)); ink_console_output(&s->console,"\n",1);
    s->queued=true; pthread_cond_broadcast(&s->changed); return true;
}
void ink_session_stop(ink_session *s)
{
    if(s->console.busy) { s->stop=true; s->pause=false; }
    else ink_console_completed(&s->console,false);
    pthread_cond_broadcast(&s->changed);
}
void ink_session_pause(ink_session *s,bool pause)
{ s->pause=pause; pthread_cond_broadcast(&s->changed); }
void ink_session_close(ink_session *s)
{ s->closing=true; pthread_cond_broadcast(&s->changed); }
void ink_session_join(ink_session *s)
{
    pthread_join(s->thread,NULL);
    pthread_cond_destroy(&s->changed); pthread_mutex_destroy(&s->lock);
}
