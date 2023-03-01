#include "lwip.h"
#include "devs.h"
#include "dal/dal.h"

static void Ethernet_Link_Periodic_Handle(struct netif *netif);

static uint32_t g_timer=0;
static eth_handle_t  *g_eth=NULL;


static eth_handle_t ethHandle;


static void eth_link_check(netif_t *netif)
{
    sys_check_timeouts();
    
    /* Ethernet Link every 100ms */
    if (HAL_GetTick() - g_timer >= 100) {
        g_timer = HAL_GetTick();
        if(g_eth) g_eth->link_cb(&g_eth->netif);
    }
}

static void ethernet_link_thread(void* arg)
{
    while(1) {
        if(g_eth) {
            eth_link_check(&g_eth->netif);
        }
    }
}

static void ethernet_link_callback(netif_t *netif)
{
    if(g_eth) {
        eth_link_check(&g_eth->netif);
    }
}


int eth_init(eth_handle_t *eh)
{
    err_t e;
    g_eth = eh;
    
#ifdef OS_KERNEL
    osThreadAttr_t  attributes;
    tcpip_init( NULL, NULL );
    netif_add(&eh->netif, &eh->addr.ip, &eh->addr.netmask, &eh->addr.gateway, NULL, &ethernetif_init, &tcpip_input);
#else
    lwip_init();
    netif_add(&eh->netif, &eh->addr.ip, &eh->addr.netmask, &eh->addr.gateway, NULL, &ethernetif_init, &ethernet_input);
#endif

#if 1
    netif_set_default(&eh->netif);
    netif_set_up(&eh->netif);
#endif

#ifdef OS_KERNEL
    /* Create the Ethernet link handler thread */
    memset(&attributes, 0x0, sizeof(osThreadAttr_t));
    attributes.name = "ethLink";
    attributes.stack_size = 512;
    attributes.priority = osPriorityBelowNormal;
    osThreadNew(ethernet_link_thread, &eh->netif, &attributes);
#else
    netif_set_link_callback(&eh->netif, ethernet_link_callback);
#endif
  
    /* Start DHCP negotiation for a network interface (IPv4) */
    //dhcp_start(&eh->netif);
    
    return 0;
}



void eth_test(void)
{
    
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



