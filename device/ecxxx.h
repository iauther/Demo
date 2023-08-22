#ifndef __ECXXX_Hx__
#define __ECXXX_Hx__


#include "types.h"
#include "comm.h"
#include "dal_uart.h"

typedef enum {
    TCP_CONT_ID_0=1,
    TCP_CONT_ID_1,
    TCP_CONT_ID_2,
    TCP_CONT_ID_3,
    TCP_CONT_ID_4,
    TCP_CONT_ID_5,
    TCP_CONT_ID_6,
}TCP_CONT_ID;


typedef enum {
    TCP_CONN_ID_0=0,
    TCP_CONN_ID_2,
    TCP_CONN_ID_3,
    TCP_CONN_ID_4,
    TCP_CONN_ID_5,
    TCP_CONN_ID_6,
    TCP_CONN_ID_7,
    TCP_CONN_ID_8,
    TCP_CONN_ID_9,
    TCP_CONN_ID_10,
    TCP_CONN_ID_11,
}TCP_CONN_ID;


typedef enum {
    MQTT_CLI_ID_0=0,
    MQTT_CLI_ID_1,
    MQTT_CLI_ID_2,
    MQTT_CLI_ID_3,
    MQTT_CLI_ID_4,
    MQTT_CLI_ID_5,
}MQTT_CLI_ID;


typedef enum {
    APN_ID_CMNET=0,
    APN_ID_UNNET,
    APN_ID_CTNET,
    
    APN_ID_MAX
}APN_ID;


typedef struct {
    char *apn;  //移动:CMNET  联通:UNINET  电信:CTNET
    U8   auth;  //0:None  1:PAP  2:CHAP
}apn_info_t;


typedef struct {
    int  rssi;
    int  ber;
}ecxxx_stat_t;


typedef struct {
    int  proto;
    int  err;
}ecxxx_evt_t;


typedef enum {
    ACCESS_MODE_CACHE=0,
    ACCESS_MODE_DIRECT,
}ACCESS_MODE;

enum {
    TOPIC_PUB=0,
    TOPIC_SUB,
    TOPIC_PSUB,  //PUB & SUB
    
    TOPIC_MAX
};


enum {
    PROTO_TCP=0,
    PROTO_UDP,
    PROTO_MQTT,
};


enum {
    MQTT_ERR_DISCONN=1,
    MQTT_ERR_PING_FAIL,
    MQTT_ERR_CONN_FAIL,
    MQTT_ERR_CONNACK_FAIL,
    MQTT_ERR_SERVER_DISCONN,
    MQTT_ERR_CLIENT_DISCONN1,
    MQTT_ERR_LINK_NOT_WORK,
    MQTT_ERR_CLIENT_DISCONN2,
};


typedef int (*ecxxx_err_cb_t)(int proto, int err);
typedef int (*ecxxx_data_cb_t)(int proto, void *data, int len);


typedef struct {
    ecxxx_data_cb_t   data;
    ecxxx_err_cb_t    err;
}ecxxx_cb_t;


typedef struct {
    char *ip;
    U16  port;
}ecxxx_tcp_t;

typedef struct {
    U8  cid;    //client ID
    U8  fmt;    //0:字符   1：二进制
    U16 mid;
}ecxxx_mqtt_t;


typedef struct {
    char clientID[64];
    char username[64];
    char password[64];
}ecxxx_login_t;


typedef struct {
    U8          port;
    U32         baudrate;
    ecxxx_cb_t  cb;
}ecxxx_cfg_t;



int ecxxx_init(ecxxx_cfg_t *cfg);
int ecxxx_deinit(void);
int ecxxx_write(void *data, int len, int timeout);
int ecxxx_wait(U32 ms, char *ok, char *err);
int ecxxx_reg(APN_ID apnID);
int ecxxx_power(U8 on);
int ecxxx_restart(void);

int ecxxx_ntp(char *server, U16 port);

int ecxxx_conn(int proto, void *para);
int ecxxx_disconn(int proto);
int ecxxx_send(int proto, void *data, int len, int timeout);
int ecxxx_clear(void);
int ecxxx_err_proc(int proto, int err);
int ecxxx_poll(void);

int ecxxx_test(void);
#endif

