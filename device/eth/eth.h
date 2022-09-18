#ifndef __ETH_Hx__
#define __ETH_Hx__

#ifdef __cplusplus
 extern "C" {
#endif

#include "lwip/opt.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "netif/etharp.h"
#include "lwip/dhcp.h"
#include "lwip/netif.h"
#include "lwip/timeouts.h"
#include "ethernetif.h"
#include "lan8742.h"
#include "platform.h"

#ifdef OS_KERNEL
#include "lwip/tcpip.h"
#endif


typedef struct pbuf pbuf_t;
typedef struct tcp_pcb tcp_pcb_t;
typedef struct netif netif_t;
typedef struct netbuf netbuf_t;
typedef struct netconn netconn_t;



typedef struct {
    ip_addr_t           ip;
    ip_addr_t           netmask;
    ip_addr_t           gateway;
}ipaddr_t;


typedef struct {
    ipaddr_t            addr;
    
    lan8742_Object_t    lan;
    netconn_t           *conn;
    netif_t             netif;
    int                 connected;
}eth_handle_t;


int eth_init(eth_handle_t *eh);
int eth_check(void);
void eth_test(void);

#ifdef __cplusplus
}
#endif

#endif


