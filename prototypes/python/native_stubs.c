/* Host verifies VM/files only. Networking deliberately unavailable here. */
#define _POSIX_C_SOURCE 200809L
#include "native.h"
#include <errno.h>
#include <time.h>
int ink_python_wifi(const char *a,const char *b) { (void)a;(void)b; return ENOSYS; }
int ink_python_wifi_status(char ip[16]) { ip[0]=0; return 0; }
void ink_python_network_close(void) {}
int ink_python_network_sleep(bool sleeping) { (void)sleeping; return 0; }
int ink_python_http(const char *a,const char *b,const char *c,const char *d,size_t n,const char *h)
{ (void)a;(void)b;(void)c;(void)d;(void)n;(void)h; return -ENOSYS; }
uint32_t ink_python_ticks(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return (uint32_t)(t.tv_sec*1000+t.tv_nsec/1000000); }
void ink_python_delay(unsigned ms) { struct timespec t={.tv_sec=ms/1000,.tv_nsec=(ms%1000)*1000000}; nanosleep(&t,NULL); }
