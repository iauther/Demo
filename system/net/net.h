#ifndef __NET_Hx__
#define __NET_Hx__

#include "comm.h"
#include "eth/eth.h"
#include "list.h"
#include "dal.h"


#define CONN_MAX  20


enum {
    NET_ETH=0,
    NET_WIFI,

    NET_MAX,
};

enum {
    NET_EVT_NEW_CONN=0,
    NET_EVT_DIS_CONN,
    NET_EVT_DATA_IN,
};


#define DHCP_OFF                   (uint8_t) 0
#define DHCP_START                 (uint8_t) 1
#define DHCP_WAIT_ADDRESS          (uint8_t) 2
#define DHCP_ADDRESS_ASSIGNED      (uint8_t) 3
#define DHCP_TIMEOUT               (uint8_t) 4
#define DHCP_LINK_DOWN             (uint8_t) 5

typedef struct {
    rx_cb_t     callback;
    char        *ip;
    char        *mask;
    char        *gateway;
}net_cfg_t;


typedef struct {
    handle_t        eth;
    net_cfg_t       cfg;
    
    void            *conn;
    void            *newconn;
}net_handle_t;



handle_t net_init(net_cfg_t *cfg);
int net_deinit(handle_t h);
int net_write(handle_t h, void *conn, U8 *data, int len);
int net_broadcast(handle_t h, U8 *data, int len);

#endif