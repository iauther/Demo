#ifndef __MYMQTT_Hx__
#define __MYMQTT_Hx__

#include "mqtt.h"

//#define USE_HV

typedef void (*mymqtt_print_t)(const char* format, ...);

#ifdef USE_HV
#include "mqtt_client.h"

typedef struct {
    int flag;
}mqtt_callback_t;
#else
#include "MQTTClient.h"

typedef struct {
    MQTTClient_disconnected     *disconn;
    MQTTClient_messageArrived   *arrived;
    MQTTClient_deliveryComplete *complete;
    MQTTClient_connectionLost   *lostconn;
    MQTTClient_published        *published;

    MQTTClient_traceCallback    *trace;
}mqtt_callback_t;
#endif

#include "utils_hmac.h"

enum {
    MODE_SYS=0,
    MODE_DM,
    MODE_USR,
};

enum {
    TOPIC_DN_USR=0,

    TOPIC_DN_RAW,
    TOPIC_UP_RAW_REPLY,

    TOPIC_SUB_MAX,
};

enum {
    TOPIC_UP_USR = 0,

    TOPIC_UP_RAW,
    TOPIC_DN_RAW_REPLY,

    TOPIC_PUB_MAX,
};


class myMqtt {
public:
    myMqtt();
    ~myMqtt();
    
    void* conn(conn_para_t* para, void* userdata);
    void* conn(conn_para_t* para, sign_data_t* sign, void* userdata);

    int disconn(void* conn);
    int is_connected(void* conn);
    int pub(void* conn, void*para, void* payload, int payloadlen);
    int sub(void* conn, void* para);
    int read(void* conn, void* buf, int buflen);

private:
    mqtt_callback_t cb;
    uint16_t packet_id;

    int set_cb(void *conn);
};


#endif

