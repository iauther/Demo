#include "mqtt.h"


static int mqtt_self_init(void)
{
    int i,r;

    
    return 0;
}


static int mqtt_self_deinit(void)
{
    
    
    return 0;
}


static handle_t mqtt_self_conn(conn_para_t *para, void *userdata)
{
    int i,r;
    
    return NULL;
}


static int mqtt_self_disconn(handle_t h)
{
    int r;
    
    if(!h) {
        return -1;
    }
    
    return 0;
}



static int mqtt_self_sub(handle_t hconn, void *para)
{
    int r=0;
    
    if(!hconn || !para) {
        return -1;
    }

    
    
    return 0;
}


static int mqtt_self_pub(handle_t hconn, void *para, void *data, int len)
{
    int r,tlen=0;

    if(!hconn || !data || !len) {
        return -1;
    }    
    
    
    
    return r;
}


static int mqtt_self_set_userdata(void *handle, void *userdata)
{
    return 0;
}


static int mqtt_self_req(handle_t hconn)
{
    int r;
    
    return 0;
}



static int mqtt_self_ntp_synced(void)
{
    return 0;
}



mqtt_fn_t mqtt_self_fn={
    mqtt_self_init,
    mqtt_self_deinit,
    
    mqtt_self_conn,
    mqtt_self_disconn,
    mqtt_self_sub,
    mqtt_self_pub,
    mqtt_self_req,
    mqtt_self_ntp_synced,
};
