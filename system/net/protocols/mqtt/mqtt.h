#ifndef __MQTT_Hx__
#define __MQTT_Hx__

#include "protocol.h"
#include "net.h"


enum {
    MQTT_ALI=0,
    MQTT_SELF,
    
    MQTT_MAX
};

enum {
    MQTT_REQ_CFG=1<<0,
    MQTT_REQ_FW=1<<1,
};


typedef struct {
    U8    flag;
}mqtt_sub_para_t;

typedef struct {
    U8    tp;       //data type
}mqtt_pub_para_t;


#ifndef _WIN32


#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int (*init)(void);
    int (*deinit)(void);

    handle_t(*conn)(conn_para_t* para, void* userdata);
    int (*disconn)(handle_t h);
    int (*reconn)(handle_t hconn);
    int (*sub)(handle_t hconn, void *para);
    int (*pub)(handle_t hconn, void *para, void* data, int len);
    int (*request)(handle_t hconn, int req);
}mqtt_fn_t;


int mqtt_init(U8 id);
int mqtt_deinit(void);
handle_t mqtt_conn(conn_para_t *para, void *userdata);
int mqtt_disconn(handle_t hconn);
int mqtt_reconn(handle_t hconn);
int mqtt_pub(handle_t hconn, void *para, void *data, int len);
int mqtt_sub(handle_t hconn, void *para);
int mqtt_req(handle_t hconn, int req);

#ifdef __cplusplus
}
#endif

#endif

#endif
