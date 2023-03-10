#ifndef __ETH_Hx__
#define __ETH_Hx__

#include "devs.h"
#include "lwip.h"

#ifdef __cplusplus
 extern "C" {
#endif


typedef struct pbuf pbuf_t;
typedef struct netif netif_t;
typedef struct netbuf netbuf_t;
typedef struct netconn netconn_t;
typedef struct tcp_pcb tcp_pcb_t;


typedef void (*link_callback)(netif_t *netif);


typedef struct {
    ip_addr_t           ip;
    ip_addr_t           netmask;
    ip_addr_t           gateway;
}ipaddr_t;


typedef struct {
    ipaddr_t            addr;
    
    lan8742_Object_t    lan;
    netif_t             netif;
    int                 connected;
    
    link_callback       link_cb;
}eth_handle_t;


int eth_init(eth_handle_t *eh);
void eth_link_check(void);
void eth_test(void);

#ifdef __cplusplus
}
#endif

#endif


