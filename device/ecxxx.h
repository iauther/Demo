#ifndef __ECXXX_Hx__
#define __ECXXX_Hx__


#include "types.h"
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
    TCP_CONN_ID_1,
    TCP_CONN_ID_2,
    TCP_CONN_ID_3,
    TCP_CONN_ID_4,
    TCP_CONN_ID_5,
    TCP_CONN_ID_6,
    TCP_CONN_ID_7,
    TCP_CONN_ID_8,
    TCP_CONN_ID_9,
    TCP_CONN_ID_10,
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
typedef int (*ecxxx_data_cb_t)(int proto, int evt, void *data, int len);


typedef struct {
    ecxxx_data_cb_t   data;
    ecxxx_err_cb_t    err;
}ecxxx_cb_t;


typedef struct {
    U8          port;
    U32         baudrate;
    ecxxx_cb_t  cb;
}ecxxx_cfg_t;



handle_t ecxxx_init(ecxxx_cfg_t *cfg);
int ecxxx_deinit(handle_t h);
int ecxxx_wait(handle_t h, U32 ms, char *ok, char *err);
int ecxxx_reg(handle_t h, APN_ID apnID);
int ecxxx_power(handle_t h, U8 on);
int ecxxx_restart(handle_t h);

int ecxxx_ntp(handle_t h, char *server, U16 port);

int ecxxx_conn(handle_t h, void *para, int timemout);
int ecxxx_disconn(handle_t h, int timemout);
int ecxxx_write(handle_t h, void *data, int len, int timeout);
int ecxxx_clear(handle_t h);
int ecxxx_poll(handle_t h);

int ecxxx_test(void);
#endif

