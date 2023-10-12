#include "utils_hmac.h"
#include "net.h"
#include "mqtt.h"


extern mqtt_fn_t mqtt_ali_fn;
extern mqtt_fn_t mqtt_self_fn;
mqtt_fn_t *mqtt_fns[MQTT_MAX]={
    &mqtt_ali_fn,
    &mqtt_self_fn,
};
static U8  mqtt_init_flag=0;
static U8  mqtt_conn_flag=0;
static mqtt_fn_t *p_mqtt_fn=NULL;


int mqtt_init(U8 id)
{
    int r=0;
    
    if(mqtt_init_flag) {
        return 0;
    }
    
    if(id>=MQTT_MAX) {
        return -1;
    }
    p_mqtt_fn = mqtt_fns[id];
    
    r = p_mqtt_fn->init();
    if(r==0) {
        mqtt_init_flag = 1;
        mqtt_conn_flag = 0;
    }
    
    return r;
}


int mqtt_deinit(void)
{
    if(!p_mqtt_fn) {
        return -1;
    }
    
    return p_mqtt_fn->deinit();
}



handle_t mqtt_conn(conn_para_t *para, void *userdata)
{
   if(!p_mqtt_fn) {
        return NULL;
    }
    
    return p_mqtt_fn->conn(para, userdata);
}


int mqtt_disconn(handle_t hconn)
{
    if(!p_mqtt_fn) {
        return -1;
    }
    
    return p_mqtt_fn->disconn(hconn);
}


int mqtt_pub(handle_t hconn, mqtt_para_t *para, void *data, int len)
{
    if(!p_mqtt_fn) {
        return -1;
    }
    
    return p_mqtt_fn->pub(hconn, para, data, len);
}


int mqtt_sub(handle_t hconn, mqtt_para_t *para)
{
    if(!p_mqtt_fn) {
        return NULL;
    }
    
    return p_mqtt_fn->sub(hconn, para);
}


int mqtt_ntp_synced(void)
{
    if(!p_mqtt_fn) {
        return 0;
    }
    
    return p_mqtt_fn->ntp_synced();
}



