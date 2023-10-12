#include "net.h"

#ifndef _WIN32
#include "list.h"
#include "task.h"
#endif


typedef struct {
    
    net_cfg_t   cfg;
    
    handle_t    hio;
    
    handle_t    lconn;      //connect list
    int         inited;

#ifdef _WIN32
    mySock      mSock;
    myMqtt      mMqtt;
    mySerial    mSerial;
#endif
}net_handle_t;

static net_handle_t netHandle;



int net_init(net_cfg_t *cfg)
{
    handle_t hio=NULL;
    net_handle_t *nh=&netHandle;

#ifndef _WIN32
    nh->cfg = *cfg;
    //hio = netio_init(NULL);
    if(!hio) {
        //return -1;
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

    //netio_deinit(nh->hio);
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
    ch->para = *para;
    
    switch(ch->para.proto) {
        case PROTO_TCP:
        break;
        
        case PROTO_UDP:
        break;
        
        case PROTO_MQTT:
        {
#ifdef _WIN32
            ch->h = netHandle.mMqtt.conn(para, ch);
#else
            r = mqtt_init(MQTT_ALI);
            if(r) {
                free(ch); return NULL;
            }
            ch->h = mqtt_conn(para, ch);
#endif
            if(!ch->h) {
                free(ch); return NULL;
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
    
    switch(ch->para.proto) {
        case PROTO_TCP:
        break;
        
        case PROTO_UDP:
        break;
        
        case PROTO_MQTT:
        {
#ifdef _WIN32
            r = netHandle.mMqtt.disconn(ch->h);
#else
            r = mqtt_disconn(ch->h);
#endif
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
    int r;
    conn_handle_t *ch=(conn_handle_t*)hconn;
    
    if(!ch || !data || !len) {
        return -1;
    }
    
    switch(ch->para.proto) {
        case PROTO_TCP:
        break;
        
        case PROTO_UDP:
        break;
        
        case PROTO_MQTT:
        {
#ifdef _WIN32
            r = netHandle.mMqtt.read(ch->h, data, len);
#else
            //mqtt_read(ch->h, );
#endif
        }
        break;
        
        case PROTO_COAP:
        {
        }
        break;
    }
    
    return r;
}


int net_write(handle_t hconn, void *para, void *data, int len)
{
    int r=-1;
    conn_handle_t *ch=(conn_handle_t*)hconn;
    
    if(!ch || !data || !len) {
        return -1;
    }
    
    switch(ch->para.proto) {
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
            mqtt_para_t mp;
            
            mp.datato = para?(*((U8*)para)):DATATO_USR;
#ifdef _WIN32
            r = netHandle.mMqtt.pub(ch->h, &mp, data, len);
#else
            r = mqtt_pub(ch->h, &mp, data, len);
#endif
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
    
    //lconnÁÐ±í²Ù×÷
    
    return 0;
}



