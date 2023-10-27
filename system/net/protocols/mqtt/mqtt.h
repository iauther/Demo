#ifndef __MQTT_Hx__
#define __MQTT_Hx__

#include "protocol.h"
#include "net.h"


enum {
    MQTT_ALI=0,
    MQTT_SELF,
    
    MQTT_MAX
};


typedef struct {
    U8    dato;     //ali or usr
}mqtt_para_t;

#ifndef _WIN32


#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int (*init)(void);
    int (*deinit)(void);

    handle_t(*conn)(conn_para_t* para, void* userdata);
    int (*disconn)(handle_t h);
    int (*sub)(handle_t hconn, mqtt_para_t* para);
    int (*pub)(handle_t hconn, mqtt_para_t* para, void* data, int len);
    int (*ntp_synced)(void);
}mqtt_fn_t;


int mqtt_init(U8 id);
int mqtt_deinit(void);
handle_t mqtt_conn(conn_para_t *para, void *userdata);
int mqtt_disconn(handle_t hconn);
int mqtt_pub(handle_t hconn, mqtt_para_t *para, void *data, int len);
int mqtt_sub(handle_t hconn, mqtt_para_t *para);
int mqtt_ntp_synced(void);

#ifdef __cplusplus
}
#endif

#endif

#endif
