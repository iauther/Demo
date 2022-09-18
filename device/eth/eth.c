#include "eth.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/tcp.h"
#include "lwip/api.h"
#include "lwip/timeouts.h"
#include "netif/ethernet.h"

#if defined ( __CC_ARM )
#include "lwip/sio.h"
#endif

#include <string.h>
#include "platform.h"
#include "ethernetif.h"
#include "lan8742.h"



static void ethernet_link_status_updated(struct netif *netif);
static void Ethernet_Link_Periodic_Handle(struct netif *netif);


//https://wenku.baidu.com/view/43a00bbf52e79b89680203d8ce2f0066f53364ad.html
static uint32_t eth_timer=0;
static netif_t  *eth_netif=NULL;

static void ethernet_link_status_updated(struct netif *netif)
{
    if (netif_is_up(netif)) {
    }
    else {  /* netif is down */
    }
}
static void ethernet_link_thread(void* arg)
{
    eth_check();
}

static void ethernet_link_check(struct netif *netif)
{
    
}


void ETH_IRQHandler(void)
{
    extern ETH_HandleTypeDef heth;
    
    ethernetif_input(eth_netif);
    __HAL_ETH_DMA_CLEAR_IT(&heth, ETH_DMA_NORMAL_IT);
    __HAL_ETH_DMA_CLEAR_IT(&heth, ETH_DMA_RX_IT);
    __HAL_ETH_DMA_CLEAR_IT(&heth, ETH_DMA_TX_IT);
}


int eth_init(eth_handle_t *eh)
{
    err_t e;
    
#ifdef OS_KERNEL
    osThreadAttr_t  attributes;
    tcpip_init( NULL, NULL );
    netif_add(&eh->addr.netif, &eh->addr.ip, &eh->addr.netmask, &eh->addr.gateway, NULL, &ethernetif_init, &tcpip_input);
#else
    lwip_init();
    netif_add(&eh->netif, &eh->addr.ip, &eh->addr.netmask, &eh->addr.gateway, NULL, &ethernetif_init, &ethernet_input);
#endif

#if 0
    /* Registers the default network interface */
    netif_set_default(&eth->netif);

    
    if (netif_is_link_up(&eth->netif)) {
    /* When the netif is fully configured this function must be called */
        netif_set_up(&eth->netif);
    }
    else {
    /* When the netif link is down this function must be called */
        netif_set_down(&eth->netif);
    }
#endif
    
    eth_netif = &eh->netif;
    /* Set the link callback function, this function is called on change of link status*/
    netif_set_link_callback(&eh->netif, ethernet_link_status_updated);

#ifdef OS_KERNEL
    /* Create the Ethernet link handler thread */
    memset(&attributes, 0x0, sizeof(osThreadAttr_t));
    attributes.name = "EthLink";
    attributes.stack_size = 1024;//INTERFACE_THREAD_STACK_SIZE;
    attributes.priority = osPriorityBelowNormal;
    osThreadNew(ethernet_link_thread, &eh->netif, &attributes);
    
    eth->conn = netconn_new(NETCONN_TCP);
    if(!eth->conn) {
        return -1;
    }
    
    //ip_set_option(netHandle.conn->pcb.tcp, SOF_REUSEADDR);
    netconn_bind(eth->conn, IP_ADDR_ANY, DEV_PORT);
    netconn_listen(eth->conn);
#endif
  
    /* Start DHCP negotiation for a network interface (IPv4) */
    //dhcp_start(&eth->netif);
    
    eth_test();
    
    
    return 0;
}



int eth_check(void)
{
    sys_check_timeouts();
    
    /* Ethernet Link every 100ms */
    if (HAL_GetTick() - eth_timer >= 100) {
        eth_timer = HAL_GetTick();
        ethernet_link_check_state(eth_netif);
    }
    
    return 0;
}


///////////////////////////////////////
static void tcp_client_init(void);
static err_t tcp_client_connected(void *arg, struct tcp_pcb *pcb, err_t err);
static err_t tcp_client_data_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *tcp_recv_pbuf, err_t err)
 {
   struct pbuf *tcp_send_pbuf;
   char echoString[]="This is the server content echo:";
  
   if (tcp_recv_pbuf != NULL)
   {
     tcp_recved(pcb, tcp_recv_pbuf->tot_len);
  
     tcp_write(pcb,echoString, strlen(echoString), 1);
     tcp_send_pbuf = tcp_recv_pbuf;
     tcp_write(pcb, tcp_send_pbuf->payload, tcp_send_pbuf->len, 1);
  
     pbuf_free(tcp_recv_pbuf);
   }
   else if (err == ERR_OK) {
     tcp_close(pcb);
     tcp_client_init();
       
     return ERR_OK;
   }
  
   return ERR_OK;
 }
static err_t tcp_client_connected(void *arg, struct tcp_pcb *pcb, err_t err)
{
    char clientString[]="This is a new client connection.";

    tcp_recv(pcb, tcp_client_data_recv);
    tcp_write(pcb,clientString, strlen(clientString),0);

    return ERR_OK;
 }
 static void tcp_client_connect_error(void *arg, err_t err)
 {
    tcp_client_init();
 }

 
 
static void tcp_client_init(void)
{
    struct tcp_pcb *tcp_client_pcb;
    ip_addr_t ipaddr;

    tcp_client_pcb = tcp_new();
    if (tcp_client_pcb != NULL) {
        ip4addr_aton("192.168.2.77", &ipaddr);
        
        tcp_bind(tcp_client_pcb, IP_ADDR_ANY, 8888);
        tcp_connect(tcp_client_pcb, &ipaddr, 9999, tcp_client_connected);
        tcp_err(tcp_client_pcb, tcp_client_connect_error);
    }
 }



void eth_test(void)
{
    tcp_client_init();
    while(1);
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



