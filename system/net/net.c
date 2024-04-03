#include "net.h"

#ifndef _WIN32
#include "list.h"
#include "task.h"
#endif


typedef struct {
    net_cfg_t   cfg;
    handle_t    hio;

#ifdef _WIN32
    mySock      mSock;
    myMqtt      mMqtt;
    mySerial    mSerial;
#endif
}net_handle_t;


handle_t net_init(net_cfg_t *cfg)
{
#ifdef _WIN32
    net_handle_t* h = new net_handle_t;
#else
    net_handle_t* h = (net_handle_t*)malloc(sizeof(net_handle_t));
#endif

    if(!h) {
        return NULL;
    }
    
#ifndef _WIN32
    h->cfg = *cfg;
    //h->hio = netio_init(NULL);
    //if(!h->hio) {
    //    free(h);
    //    return NULL;
    //}
#endif
    
    return h;
}


int net_deinit(handle_t h)
{
    net_handle_t *nh=(net_handle_t*)h;
    
    if(!nh) {
        return -1;
    }

    //netio_deinit(nh->hio);
#ifdef _WIN32
    delete nh;
#else
    free(nh);
#endif

    return 0;
}


handle_t net_conn(handle_t h, conn_para_t *para)
{
    int r;
    net_handle_t *nh=(net_handle_t*)h;
    conn_handle_t *ch=NULL;
    
    if(!nh || !para) {
        return NULL;
    }
    
    ch = (conn_handle_t*)malloc(sizeof(conn_handle_t));
    if(!ch) {
        return NULL;
    }
    ch->para = *para;
    ch->h    = h;
    
    switch(ch->para.proto) {
        case PROTO_TCP:
        break;
        
        case PROTO_UDP:
        break;
        
        case PROTO_MQTT:
        {
#ifdef _WIN32
            ch->hc = nh->mMqtt.conn(para, ch);
#else
            r = mqtt_init(MQTT_ALI);
            if(r) {
                free(ch); return NULL;
            }
            ch->hc = mqtt_conn(para, ch);
#endif
            if(!ch->hc) {
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
            net_handle_t* nh = (net_handle_t*)ch->h;
            r = nh->mMqtt.disconn(ch->hc);
#else
            r = mqtt_disconn(ch->hc);
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


int net_reconn(handle_t hconn)
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
            net_handle_t* nh = (net_handle_t*)ch->h;
            nh->mMqtt.disconn(ch->hc);
            ch->hc = nh->mMqtt.conn(&ch->para, ch);
            if(ch->hc) {
                r = 0;
            }
#else
            r = mqtt_reconn(ch->hc);
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
            net_handle_t* nh = (net_handle_t*)ch->h;
            r = nh->mMqtt.read(ch->hc, data, len);
#else
            //mqtt_read(ch->hc, );
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
#ifdef _WIN32
            net_handle_t* nh = (net_handle_t*)ch->h;
            r = nh->mMqtt.pub(ch->hc, para, data, len);
#else
            r = mqtt_pub(ch->hc, para, data, len);
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


int net_req(handle_t hconn, int req)
{
    int r=-1;
    conn_handle_t *ch=(conn_handle_t*)hconn;
    
    if(!ch) {
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
#ifdef _WIN32
            //
#else
            r = mqtt_req(ch->hc, req);
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







