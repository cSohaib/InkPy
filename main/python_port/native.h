#pragma once
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
void ink_python_files_reset(void);
void ink_python_directory(const char *script);
int ink_python_path(const char *path,char out[512]);
int ink_python_wifi(const char *ssid,const char *password);
int ink_python_wifi_status(char ip[16]);
void ink_python_network_close(void);
int ink_python_network_sleep(bool sleeping);
int ink_python_http(const char *url,const char *destination,const char *method,
    const char *body,size_t body_bytes,const char *headers);
uint32_t ink_python_ticks(void);
void ink_python_delay(unsigned ms);
