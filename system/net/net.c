#include "net.h"
#include "list.h"
#include "task.h"
#include "mqtt.h"

typedef struct {
    
    net_cfg_t   cfg;
    
    handle_t    hio;
    
    handle_t    lconn;      //connect list
    int         inited;
}net_handle_t;

static net_handle_t netHandle;



int net_init(net_cfg_t *cfg)
{
    handle_t hio;
    net_handle_t *nh=&netHandle;

    nh->cfg = *cfg;

#if 0
    hio = netio_init(&nh->cfg.ioc);
    if(!hio) {
        return -1;
    }
#endif
    
    nh->hio = hio;
    nh->inited = 1;
    
    return 0;
}


int net_deinit(void)
{
    net_handle_t *nh=&netHandle;
    
    if(!nh->inited) {
        return -1;
    }

    netio_deinit(nh->hio);
    //lconn_free();
    nh->inited = 0;
    
    return 0;
}


handle_t net_conn(conn_para_t *para)
{
    int r;
    conn_handle_t *ch=NULL;
    net_handle_t *nh=&netHandle;
    
    if(!nh->inited || !para) {
        return NULL;
    }
    
    ch = (conn_handle_t*)calloc(1, sizeof(conn_handle_t));
    if(!ch) {
        return NULL;
    }
    ch->proto = para->proto;
    ch->callback = para->callback;
    
    switch(ch->proto) {
        case PROTO_TCP:
        break;
        
        case PROTO_UDP:
        break;
        
        case PROTO_MQTT:
        {
            r = mqtt_init(MQTT_ALI);
            if(r) {
                free(ch);
                return NULL;
            }
            
            ch->h = mqtt_conn(para, ch);
            if(ch->h) {
                mqtt_set_userdata(ch->h, ch);
            }
        }
        break;
        
        case PROTO_COAP:
        {
        }
        break;
    }
    
    return ch;
}


int net_disconn(handle_t hconn)
{
    int r=-1;
    conn_handle_t *ch=(conn_handle_t*)hconn;
    
    if(!ch) {
        return -1;
    }
    
    switch(ch->proto) {
        case PROTO_TCP:
        break;
        
        case PROTO_UDP:
        break;
        
        case PROTO_MQTT:
        {
            r = mqtt_disconn(ch->h);
        }
        break;
        
        case PROTO_COAP:
        {
        }
        break;
    }
    
    return r;
}


int net_read(handle_t hconn, void *para, void *data, int len)
{
    conn_handle_t *ch=(conn_handle_t*)hconn;
    
    if(!ch || !data || !len) {
        return -1;
    }
    
    switch(ch->proto) {
        case PROTO_TCP:
        break;
        
        case PROTO_UDP:
        break;
        
        case PROTO_MQTT:
        {
            //mqtt_read(ch->h, );
        }
        break;
        
        case PROTO_COAP:
        {
        }
        break;
    }
    
    return 0;
}


int net_write(handle_t hconn, void *para, void *data, int len)
{
    int r=-1;
    conn_handle_t *ch=(conn_handle_t*)hconn;
    
    if(!ch || !data || !len) {
        return -1;
    }
    
    switch(ch->proto) {
        case PROTO_TCP:
        {
            
        }
        break;
        
        case PROTO_UDP:
        {
            
        }
        break;
        
        case PROTO_MQTT:
        {
            r = mqtt_pub(ch->h, para, data, len);
        }
        break;
        
        case PROTO_COAP:
        {
            
        }
        break;
    }
    
    return r;
}


int net_broadcast(void *data, int len)
{
    net_handle_t *nh=&netHandle;
    
    if(!data || !len) {
        return -1;
    }
    
    //lconnÁĞ±í²Ù×÷
    
    return 0;
}



