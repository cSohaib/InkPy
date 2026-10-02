#pragma once
#include "ink_console.h"
#include <pthread.h>
/* Host adapter. Guard UI reads/edits and request functions with lock.
 * Worker owns VM; UI renders a copied snapshot after releasing lock. */
typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t changed;
    pthread_t thread;
    ink_console console;
    char command[INK_CONSOLE_INPUT];
    char path[512];
    void *heap;
    size_t heap_size;
    bool queued,stop,pause,paused,closing,closed,file_job;
} ink_session;
int ink_session_start(ink_session *s,void *heap,size_t size);
void ink_session_submit(ink_session *s);
/* Same lock contract. Reject busy/closing, long paths and non-.py files. */
bool ink_session_file(ink_session *s,const char *path);
void ink_session_stop(ink_session *s);
void ink_session_pause(ink_session *s,bool pause);
void ink_session_close(ink_session *s);
void ink_session_join(ink_session *s);
