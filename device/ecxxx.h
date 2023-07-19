#ifndef __ECXXX_Hx__
#define __ECXXX_Hx__


#include "types.h"
#include "comm.h"


typedef enum {
    CONTEXT_ID_1=1,
    CONTEXT_ID_2,
    CONTEXT_ID_3,
    CONTEXT_ID_4,
    CONTEXT_ID_5,
    CONTEXT_ID_6,
    CONTEXT_ID_7,
}CONTEXT_ID;


typedef enum {
    CONNECT_ID_0=0,
    CONNECT_ID_2,
    CONNECT_ID_3,
    CONNECT_ID_4,
    CONNECT_ID_5,
    CONNECT_ID_6,
    CONNECT_ID_7,
    CONNECT_ID_8,
    CONNECT_ID_9,
    CONNECT_ID_10,
    CONNECT_ID_11,
}CONNECT_ID;


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


typedef enum {
    ACCESS_MODE_CACHE=0,
    ACCESS_MODE_DIRECT,
}ACCESS_MODE;


enum {
    PROTO_TCP=0,
    PROTO_UDP,
    PROTO_MQTT,
};


typedef int (*ecxxx_cb_t)(void *data, int len);

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
    U8      port;
    U32     baudrate;
    ecxxx_cb_t callback;
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

int ecxxx_test(void);
#endif

