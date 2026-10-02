#include "ink_python.h"
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

/* Host adapter proof, not a device scheduler. Only worker touches the VM. */
static struct {
    pthread_mutex_t lock;
    pthread_cond_t changed;
    bool pause,paused,stop,done;
    unsigned polls;
    int result;
    char output[8192];
    size_t used,dropped;
} session={.lock=PTHREAD_MUTEX_INITIALIZER,.changed=PTHREAD_COND_INITIALIZER};
static union { max_align_t align; unsigned char bytes[256*1024]; } heap;
static void check(bool ok,const char *message)
{
    if(!ok) { fprintf(stderr,"FAIL: %s\n",message); exit(1); }
}
static bool control(void *context)
{
    (void)context;
    pthread_mutex_lock(&session.lock);
    session.polls++;
    while(session.pause && !session.stop) {
        session.paused=true;
        pthread_cond_broadcast(&session.changed);
        pthread_cond_wait(&session.changed,&session.lock);
    }
    session.paused=false;
    bool stop=session.stop;
    pthread_cond_broadcast(&session.changed);
    pthread_mutex_unlock(&session.lock);
    return stop;
}
static void output(void *context,const char *bytes,size_t length)
{
    (void)context;
    pthread_mutex_lock(&session.lock);
    /* Retain newest bytes, bounded independently of script runtime. */
    for(size_t i=0;i<length;i++) {
        if(session.used==sizeof(session.output)) {
            memmove(session.output,session.output+1,--session.used);
            session.dropped++;
        }
        session.output[session.used++]=bytes[i];
    }
    pthread_mutex_unlock(&session.lock);
}
static void *worker(void *context)
{
    (void)context;
    int stack_top;
    ink_python_callbacks(control,output,NULL);
    ink_python_init(heap.bytes,sizeof(heap.bytes),&stack_top);
    ink_python_text("print('worker ready')",false);
    int result=ink_python_text(
        "while True:\n"
        "    try:\n"
        "        while True:\n"
        "            pass\n"
        "    except BaseException:\n"
        "        pass\n",false);
    ink_python_close(); /* Stop acknowledgment comes AFTER VM cleanup. */
    pthread_mutex_lock(&session.lock);
    session.result=result; session.done=true;
    pthread_cond_broadcast(&session.changed);
    pthread_mutex_unlock(&session.lock);
    return NULL;
}
static void wait_for(bool *flag)
{
    struct timespec deadline;
    clock_gettime(CLOCK_REALTIME,&deadline); deadline.tv_sec+=5;
    while(!*flag) check(pthread_cond_timedwait(&session.changed,&session.lock,&deadline)==0,
        "host check watchdog (not a Python runtime limit)");
}
int main(void)
{
    pthread_t thread;
    session.pause=true;
    check(pthread_create(&thread,NULL,worker,NULL)==0,"create worker");
    pthread_mutex_lock(&session.lock);
    wait_for(&session.paused);
    unsigned before=session.polls;
    puts("pause acknowledged; UI thread remains responsive");
    session.pause=false; pthread_cond_broadcast(&session.changed);
    while(session.polls==before) {
        struct timespec deadline; clock_gettime(CLOCK_REALTIME,&deadline); deadline.tv_sec+=5;
        check(pthread_cond_timedwait(&session.changed,&session.lock,&deadline)==0,"resume");
    }
    session.pause=true;
    wait_for(&session.paused);
    puts("resume and second pause acknowledged");
    session.stop=true; pthread_cond_broadcast(&session.changed);
    wait_for(&session.done);
    check(session.result==2,"uncatchable stop");
    fwrite(session.output,1,session.used,stdout);
    pthread_mutex_unlock(&session.lock);
    pthread_join(thread,NULL);
    puts("stop acknowledged after VM cleanup, including while paused");
    return 0;
}
