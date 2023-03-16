#include "lwip.h"
#include "eth/eth.h"
#include "dal/dal.h"


#define DEFAULT_IP          "192.168.2.88"
#define DEFAULT_IPMASK      "255.255.255.0"
#define DEFAULT_GATEWAY     "192.168.2.1"


typedef struct {
    netif_t             netif;
    int                 connected;
    eth_linkcb_t        link_cb;
}eth_handle_t;


static eth_handle_t *ethHandle=NULL;

void set_ipaddr(char *ip, char *mask, char *gw, ipaddr_t *addr)
{
    ip4addr_aton(ip, &addr->ip);
    ip4addr_aton(mask, &addr->netmask);
    ip4addr_aton(gw, &addr->gateway);
}
static void ethernet_link_callback(netif_t *netif)
{
    if(ethHandle && ethHandle->link_cb) {
        ethHandle->link_cb(netif);
    }
}



handle_t eth_init(eth_cfg_t *cfg)
{
    err_t e;
    ipaddr_t addr;
    eth_handle_t *h=calloc(1, sizeof(eth_handle_t));
    
    if(!h) {
        return NULL;
    }

    if(cfg) {
        h->link_cb = cfg->cb;
    }
    
#ifdef OS_KERNEL
    tcpip_init( NULL, NULL );
    
    netif_add_noaddr(&h->netif, NULL, ethernetif_init, tcpip_input);
    eth_set_ip(&h->netif, DEFAULT_IP, DEFAULT_IPMASK, DEFAULT_GATEWAY);
    
    //set_ipaddr(DEFAULT_IP, DEFAULT_IPMASK, DEFAULT_GATEWAY, &addr);
    //netif_add(&h->netif, &addr.ip, &addr.netmask, &addr.gateway, NULL, ethernetif_init, tcpip_input);
#else
    lwip_init();
    netif_add(&h->netif, &addr.ip, &addr.netmask, &addr.gateway, NULL, ethernetif_init, ethernet_input);
#endif
	
    netif_set_link_callback(&h->netif, ethernet_link_callback);
    netif_set_default(&h->netif);
    netif_set_up(&h->netif);
    
    /* Start DHCP negotiation for a network interface (IPv4) */
    //dhcp_start(&h->netif);

    ethHandle = h;
    
    return h;
}


void eth_set_ip(handle_t h, char *ip, char *mask, char *gw)
{
    ipaddr_t addr;
    eth_handle_t *eh=(eth_handle_t*)h;
    
    if(!eh) {
        return;
    }
    
    set_ipaddr(ip, mask, gw, &addr);
    netif_set_addr(&eh->netif, &addr.ip, &addr.netmask, &addr.gateway);
}



void eth_link_check(handle_t h)
{
    eth_handle_t *eh=(eth_handle_t*)h;
    
    if(!eh) {
        return;
    }
    
    sys_check_timeouts();
    ethernet_link_check(&eh->netif);
}


////////////////////////////////////////
sio_fd_t sio_open(u8_t devnum)
{
  sio_fd_t sd=0;

  return sd;
}
void sio_send(u8_t c, sio_fd_t fd)
{
}
u32_t sio_read(sio_fd_t fd, u8_t *data, u32_t len)
{
  return 0;
}
u32_t sio_tryread(sio_fd_t fd, u8_t *data, u32_t len)
{
  return 0;
}



