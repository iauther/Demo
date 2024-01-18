#ifndef __NET_Hx__
#define __NET_Hx__

#include "types.h"

#ifdef _WIN32
#include "mySock.h"
#include "mySerial.h"
#include "myMqtt.h"
#else
#ifndef BOOTLOADER
#include "mqtt.h"
#endif
#endif
//#include "netio.h"
#include "protocol.h"

enum {
    ROLE_CLIENT=0,
    ROLE_SERVER,
};


typedef struct {
    char        *host;
    char        *ip;
    U16         port;
}net_addr_t;

typedef struct {
    char        *ip;
    char        *mask;
    char        *gateway;
}net_gate_t;

typedef struct {
    //netio_cfg_t ioc;
    net_gate_t  gate;
}net_cfg_t;

typedef struct {
    handle_t     h;
    handle_t     hc;
    conn_para_t  para;
}conn_handle_t;


handle_t net_init(net_cfg_t *cfg);
int net_deinit(handle_t h);

handle_t net_conn(handle_t h, conn_para_t *para);
int net_disconn(handle_t hconn);

int net_read(handle_t hconn, void *para, void *data, int len);
int net_write(handle_t hconn, void *para, void *data, int len);

#endif
