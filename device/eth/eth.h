#ifndef __ETH_Hx__
#define __ETH_Hx__

#include "types.h"
#include "cfg.h"

#ifdef __cplusplus
 extern "C" {
#endif


#ifdef USE_ETH

#include "lwip.h"


typedef struct pbuf pbuf_t;
typedef struct netif netif_t;
typedef struct netbuf netbuf_t;
typedef struct netconn netconn_t;
typedef struct tcp_pcb tcp_pcb_t;


typedef void (*eth_linkcb_t)(netif_t *netif);


typedef struct {
    ip_addr_t           ip;
    ip_addr_t           netmask;
    ip_addr_t           gateway;
}ipaddr_t;


typedef struct {
    eth_linkcb_t        cb;
}eth_cfg_t;



handle_t eth_init(eth_cfg_t *cfg);
void eth_set_ip(handle_t h, char *ip, char *mask, char *gw);
void eth_link_check(handle_t h);
#endif

#ifdef __cplusplus
}
#endif

#endif


