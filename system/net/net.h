#ifndef __NET_Hx__
#define __NET_Hx__

#include "types.h"
#include "netio.h"
#include "protocol.h"

enum {
    ROLE_CLIENT=0,
    ROLE_SERVER,
};

enum {
    PROTO_TCP=0,
    PROTO_UDP,
    PROTO_MQTT,
    PROTO_COAP,
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
    netio_cfg_t ioc;
    net_gate_t  gate;
}net_cfg_t;

typedef struct {
    U8          proto;
    net_para_t  para;
    rx_cb_t     callback;
}conn_para_t;

typedef struct {
    handle_t     h;
    U8           proto;
    rx_cb_t      callback;
}conn_handle_t;


int net_init(net_cfg_t *cfg);
int net_deinit(void);

handle_t net_conn(conn_para_t *para);
int net_disconn(handle_t hconn);

int net_read(handle_t hconn, void *para, void *data, int len);
int net_write(handle_t hconn, void *para, void *data, int len);

int net_broadcast(void *data, int len);

#endif
