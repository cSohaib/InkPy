#include "python_port/native.h"
#include "ink_python.h"
#include "board.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <sys/time.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
static esp_netif_t *netif;
static bool initialized,started,suspended;
static esp_http_client_handle_t client;
static FILE *download;
static char temporary[544];
int ink_python_set_time(int year,int month,int day,int hour,int minute,int second)
{
    struct tm t={.tm_year=year-1900,.tm_mon=month-1,.tm_mday=day,
        .tm_hour=hour,.tm_min=minute,.tm_sec=second,.tm_isdst=-1};
    if(mktime(&t)==(time_t)-1) return EINVAL;
    return ink_rtc_set(&t)==ESP_OK?0:EIO;
}
uint32_t ink_python_ticks(void) { return (uint32_t)(esp_timer_get_time()/1000); }
void ink_python_delay(unsigned ms) { vTaskDelay(pdMS_TO_TICKS(ms)?pdMS_TO_TICKS(ms):1); }
static void http_close(void)
{
    if(client) { esp_http_client_cleanup(client); client=NULL; }
    if(download) { fclose(download); download=NULL; }
    if(temporary[0]) { unlink(temporary); temporary[0]=0; }
}
void ink_python_network_close(void)
{ http_close(); if(started) { esp_wifi_disconnect(); esp_wifi_stop(); started=false; } suspended=false; }
int ink_python_network_sleep(bool sleep)
{
    if(sleep) { suspended=started; if(started&&esp_wifi_stop()!=ESP_OK) return EIO; }
    else if(suspended) { suspended=false; if(esp_wifi_start()!=ESP_OK||esp_wifi_connect()!=ESP_OK) return EIO; }
    return 0;
}
int ink_python_wifi(const char *ssid,const char *password)
{
    if(!ssid[0]||strlen(ssid)>32||strlen(password)>63) return EINVAL;
    if(!initialized) {
        /* Never erase NVS (installer/other firmware may own entries). */
        if(nvs_flash_init()!=ESP_OK) return EIO;
        esp_err_t e=esp_netif_init(); if(e!=ESP_OK&&e!=ESP_ERR_INVALID_STATE) return EIO;
        e=esp_event_loop_create_default(); if(e!=ESP_OK&&e!=ESP_ERR_INVALID_STATE) return EIO;
        netif=esp_netif_create_default_wifi_sta(); if(!netif) return ENOMEM;
        wifi_init_config_t cfg=WIFI_INIT_CONFIG_DEFAULT();
        if(esp_wifi_init(&cfg)!=ESP_OK) return EIO;
        if(esp_wifi_set_storage(WIFI_STORAGE_RAM)!=ESP_OK||esp_wifi_set_mode(WIFI_MODE_STA)!=ESP_OK) return EIO;
        initialized=true;
    }
    if(started) { esp_wifi_disconnect(); esp_wifi_stop(); started=false; }
    wifi_config_t cfg={0}; memcpy(cfg.sta.ssid,ssid,strlen(ssid)); memcpy(cfg.sta.password,password,strlen(password));
    if(esp_wifi_set_config(WIFI_IF_STA,&cfg)!=ESP_OK||esp_wifi_start()!=ESP_OK) return EIO;
    started=true; if(esp_wifi_connect()!=ESP_OK) { ink_python_network_close(); return EIO; }
    /* RTC is set only by the user; TLS uses it for certificate validity. */
    struct tm t; if(ink_rtc_read(&t)==ESP_OK) { struct timeval tv={.tv_sec=mktime(&t)}; settimeofday(&tv,NULL); }
    return 0;
}
int ink_python_wifi_status(char ip[16])
{
    ip[0]=0; esp_netif_ip_info_t info;
    if(!started||suspended||esp_netif_get_ip_info(netif,&info)!=ESP_OK||!info.ip.addr) return 0;
    wifi_ap_record_t ap; if(esp_wifi_sta_get_ap_info(&ap)!=ESP_OK) return 0;
    snprintf(ip,16,IPSTR,IP2STR(&info.ip)); return 1;
}
int ink_python_http(const char *url,const char *destination,const char *method,const char *body,size_t body_bytes,const char *headers)
{
    if(body_bytes>INT_MAX||strlen(url)>2048) return -EINVAL;
    if(strncmp(url,"http://",7)&&strncmp(url,"https://",8)) return -EINVAL;
    esp_http_client_method_t m;
    if(!strcmp(method,"GET")) m=HTTP_METHOD_GET;
    else if(!strcmp(method,"POST")) m=HTTP_METHOD_POST;
    else if(!strcmp(method,"PUT")) m=HTTP_METHOD_PUT;
    else if(!strcmp(method,"DELETE")) m=HTTP_METHOD_DELETE;
    else return -EINVAL;
    char ip[16]; if(!ink_python_wifi_status(ip)) return -ENETDOWN;
    if(strlen(destination)+16>=sizeof(temporary)) return -ENAMETOOLONG;
    char temp[544]; snprintf(temp,sizeof(temp),"%s.inkpy-download",destination);
    int fd=open(temp,O_WRONLY|O_CREAT|O_EXCL,0600); if(fd<0) return -errno;
    strcpy(temporary,temp); download=fdopen(fd,"wb");
    if(!download) { close(fd); http_close(); return -EIO; }
    esp_http_client_config_t cfg={.url=url,.method=m,.timeout_ms=500,.buffer_size=1024,
        .buffer_size_tx=1024,.crt_bundle_attach=esp_crt_bundle_attach,.disable_auto_redirect=true};
    client=esp_http_client_init(&cfg); int result=-EIO;
    if(!client) { http_close(); return -ENOMEM; }
    /* Small header buffer; don't keep an SDK lock or SD call active at a poll. */
    const char *h=headers; char line[4097];
    while(*h) {
        const char *end=strchr(h,'\n'); if(!end) break; size_t bytes=(size_t)(end-h);
        if(bytes>=sizeof(line)) { http_close(); return -EINVAL; }
        memcpy(line,h,bytes); line[bytes]=0; char *colon=strchr(line,':');
        if(!colon) { http_close(); return -EINVAL; } *colon=0;
        if(esp_http_client_set_header(client,line,colon+1)!=ESP_OK) { http_close(); return -EIO; }
        h=end+1;
    }
    ink_python_poll();
    if(esp_http_client_open(client,(int)body_bytes)!=ESP_OK) goto done;
    for(size_t sent=0;sent<body_bytes;) {
        ink_python_poll(); int n=esp_http_client_write(client,body+sent,(int)(body_bytes-sent>1024?1024:body_bytes-sent));
        if(n<=0) goto done;
        sent+=(size_t)n;
    }
    for(;;) {
        ink_python_poll(); int64_t n=esp_http_client_fetch_headers(client);
        if(n==-ESP_ERR_HTTP_EAGAIN) continue;
        if(n<0) goto done;
        break;
    }
    for(;;) {
        char buffer[1024]; ink_python_poll(); int n=esp_http_client_read(client,buffer,sizeof(buffer));
        if(n==-ESP_ERR_HTTP_EAGAIN) continue;
        if(n<0) goto done;
        if(!n) { if(!esp_http_client_is_complete_data_received(client)) goto done; break; }
        if(fwrite(buffer,1,(size_t)n,download)!=(size_t)n) goto done;
    }
    result=esp_http_client_get_status_code(client);
    if(fflush(download)) { result=-EIO; goto done; }
    if(fclose(download)) { download=NULL; result=-EIO; goto done; } download=NULL;
    /* FatFs rename refuses overwriting: an existing destination stays intact. */
    if(rename(temporary,destination)) result=-errno;
    else temporary[0]=0;
done:
    http_close(); return result;
}
