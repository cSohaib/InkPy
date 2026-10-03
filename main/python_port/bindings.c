#include "native.h"
#include "py/runtime.h"
#include "py/objstr.h"
#include <string.h>
#include <errno.h>
extern void ink_python_poll(void);
static mp_obj_t wifi(mp_obj_t ssid,mp_obj_t password)
{
    int e=ink_python_wifi(mp_obj_str_get_str(ssid),mp_obj_str_get_str(password));
    if(e) mp_raise_OSError(e);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(wifi_obj,wifi);
static mp_obj_t wifi_status(void)
{
    char ip[16]; int connected=ink_python_wifi_status(ip);
    mp_obj_t values[]={mp_obj_new_bool(connected),mp_obj_new_str(ip,strlen(ip))};
    return mp_obj_new_tuple(2,values);
}
static MP_DEFINE_CONST_FUN_OBJ_0(wifi_status_obj,wifi_status);
static mp_obj_t wifi_off(void) { ink_python_network_close(); return mp_const_none; }
static MP_DEFINE_CONST_FUN_OBJ_0(wifi_off_obj,wifi_off);
static mp_obj_t http(size_t n,const mp_obj_t *args,mp_map_t *kwargs)
{
    enum { URL,DEST,METHOD,BODY,HEADERS };
    static const mp_arg_t allowed[]={
        {MP_QSTR_url,MP_ARG_REQUIRED|MP_ARG_OBJ,{.u_obj=MP_OBJ_NULL}},
        {MP_QSTR_destination,MP_ARG_REQUIRED|MP_ARG_OBJ,{.u_obj=MP_OBJ_NULL}},
        {MP_QSTR_method,MP_ARG_KW_ONLY|MP_ARG_OBJ,{.u_obj=MP_OBJ_NEW_QSTR(MP_QSTR_GET)}},
        {MP_QSTR_body,MP_ARG_KW_ONLY|MP_ARG_OBJ,{.u_obj=mp_const_none}},
        {MP_QSTR_headers,MP_ARG_KW_ONLY|MP_ARG_OBJ,{.u_obj=mp_const_none}}};
    mp_arg_val_t v[5]; mp_arg_parse_all(n,args,kwargs,5,allowed,v);
    char destination[512]; int e=ink_python_path(mp_obj_str_get_str(v[DEST].u_obj),destination);
    if(e) mp_raise_OSError(e);
    size_t bytes=0; const char *body="";
    if(v[BODY].u_obj!=mp_const_none) body=(const char *)mp_obj_str_get_data(v[BODY].u_obj,&bytes);
    vstr_t headers; vstr_init(&headers,128);
    if(v[HEADERS].u_obj!=mp_const_none) {
        if(!mp_obj_is_type(v[HEADERS].u_obj,&mp_type_dict)) mp_raise_TypeError(MP_ERROR_TEXT("headers must be a dict"));
        mp_obj_dict_t *d=MP_OBJ_TO_PTR(v[HEADERS].u_obj);
        for(size_t i=0;i<d->map.alloc;i++) if(mp_map_slot_is_filled(&d->map,i)) {
            const char *k=mp_obj_str_get_str(d->map.table[i].key),*value=mp_obj_str_get_str(d->map.table[i].value);
            if(strpbrk(k,":\r\n")||strpbrk(value,"\r\n")||headers.len+strlen(k)+strlen(value)+3>4096)
                mp_raise_ValueError(MP_ERROR_TEXT("invalid headers"));
            vstr_add_str(&headers,k); vstr_add_char(&headers,':'); vstr_add_str(&headers,value); vstr_add_char(&headers,'\n');
        }
    }
    int status=ink_python_http(mp_obj_str_get_str(v[URL].u_obj),destination,mp_obj_str_get_str(v[METHOD].u_obj),body,bytes,vstr_null_terminated_str(&headers));
    vstr_clear(&headers); if(status<0) mp_raise_OSError(-status); return mp_obj_new_int(status);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(http_obj,2,http);
static mp_obj_t sleep_seconds(mp_obj_t seconds)
{
    mp_float_t value=mp_obj_get_float(seconds);
    if(value<0||value>4294967) mp_raise_ValueError(MP_ERROR_TEXT("invalid sleep"));
    unsigned ms=(unsigned)(value*1000);
    while(ms) { ink_python_poll(); unsigned chunk=ms>10?10:ms; ink_python_delay(chunk); ms-=chunk; }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(sleep_obj,sleep_seconds);
static mp_obj_t ticks_ms(void) { return mp_obj_new_int_from_uint(ink_python_ticks()); }
static MP_DEFINE_CONST_FUN_OBJ_0(ticks_obj,ticks_ms);
static const mp_rom_map_elem_t ink_globals_table[]={
    {MP_ROM_QSTR(MP_QSTR___name__),MP_ROM_QSTR(MP_QSTR_inkpy)},
    {MP_ROM_QSTR(MP_QSTR_wifi),MP_ROM_PTR(&wifi_obj)},
    {MP_ROM_QSTR(MP_QSTR_wifi_status),MP_ROM_PTR(&wifi_status_obj)},
    {MP_ROM_QSTR(MP_QSTR_wifi_off),MP_ROM_PTR(&wifi_off_obj)},
    {MP_ROM_QSTR(MP_QSTR_http),MP_ROM_PTR(&http_obj)},
};
static MP_DEFINE_CONST_DICT(ink_globals,ink_globals_table);
const mp_obj_module_t ink_module={.base={&mp_type_module},.globals=(mp_obj_dict_t *)&ink_globals};
MP_REGISTER_MODULE(MP_QSTR_inkpy,ink_module);
static const mp_rom_map_elem_t time_globals_table[]={
    {MP_ROM_QSTR(MP_QSTR___name__),MP_ROM_QSTR(MP_QSTR_time)},
    {MP_ROM_QSTR(MP_QSTR_sleep),MP_ROM_PTR(&sleep_obj)},
    {MP_ROM_QSTR(MP_QSTR_ticks_ms),MP_ROM_PTR(&ticks_obj)},
};
static MP_DEFINE_CONST_DICT(time_globals,time_globals_table);
const mp_obj_module_t ink_time={.base={&mp_type_module},.globals=(mp_obj_dict_t *)&time_globals};
MP_REGISTER_MODULE(MP_QSTR_time,ink_time);

/* Keep the standard JSON API authoritative even if SD contains json.py/json/. */
extern const mp_obj_module_t mp_module_json;
MP_REGISTER_MODULE(MP_QSTR_json,mp_module_json);
