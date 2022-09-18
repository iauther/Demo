#include "net.h"
#include "eth/lan8742.h"
#include "eth/ethernetif.h"
#include "lwip/init.h"
#include "lwip/ip.h"
#include "lwip/debug.h"
#include "lwip/ip_addr.h"
#include "lwip/api.h"
#include "lwip/tcpip.h"
#include "drv/delay.h"

//https://wenku.baidu.com/view/43a00bbf52e79b89680203d8ce2f0066f53364ad.html

#define DEV_IP          "192.168.2.88"
#define DEV_PORT        8888
#define DEV_IP_MASK     "255.255.255.0"
#define DEV_GATEWAY     "192.168.2.1"


typedef struct {
    eth_handle_t        eth;
    int                 connected;
}net_handle_t;

static net_handle_t netHandle={0};
#define RECV_BUF_LEN  1024
static U32 recved_len=0;
static U8 recv_buffer[RECV_BUF_LEN];
static void eth_link_callback(struct netif *netif)
{
     if(netif_is_link_up(netif)) {
        //connected
         ;
     }
     else {
        //disconnected
         ;
     }
}


static void data_callback(netconn_t *conn, void *data, int len)
{
    //netconn_write(conn, data, len, NETCONN_NOCOPY);
}

static int data_recv(void *data, int len)
{
    int wlen;
    
    if(recved_len>=RECV_BUF_LEN) {
        return -1;
    }
    
    wlen = (recved_len+len>RECV_BUF_LEN)?(RECV_BUF_LEN-recved_len):len;
    memcpy(recv_buffer+recved_len, data, len);
    
    recved_len += wlen;
    
    return 0;
}

static void set_ipaddr(ipaddr_t *ipaddr)
{
    ip4addr_aton(DEV_IP, &ipaddr->ip);
    ip4addr_aton(DEV_IP_MASK, &ipaddr->netmask);
    ip4addr_aton(DEV_GATEWAY, &ipaddr->gateway);
}



int net2_init(void)
{
    err_t e;
    netif_t *nif;
    netHandle.connected=0;

    set_ipaddr(&netHandle.eth.addr);
    eth_init(&netHandle.eth);
    
	netif_set_link_callback(&netHandle.eth.netif, eth_link_callback);
    netHandle.eth.conn = netconn_new(NETCONN_TCP);
    if(!netHandle.eth.conn) {
        return -1;
    }
    //ip_set_option(netHandle.conn->pcb.tcp, SOF_REUSEADDR);
    
    netconn_bind(netHandle.eth.conn, IP_ADDR_ANY, DEV_PORT);
    netconn_listen(netHandle.eth.conn);

    return 0;
}


int net2_loop(void)
{
    int r;
    err_t e;
    pbuf_t *p;
    netbuf_t *rbuf=NULL;
    netconn_t *newconn=NULL;
    
    while(1) {
        e = netconn_accept(netHandle.eth.conn, &newconn);
        if(e==ERR_OK) {
            while(1) {
                e = netconn_recv(newconn, &rbuf);
                if(e==ERR_OK) {
                    for(p=rbuf->ptr;p!=NULL;p=p->next) {
                        r = data_recv(p->payload, p->len);
                        if(r<0) {
                            break;
                        }
                    }
                    
                    data_callback(newconn, recv_buffer, recved_len);
                    recved_len = 0;
                }
                netbuf_delete(rbuf);
            }
        }
        rbuf->
    }
    
    return 0;
}



#include "task.h"
static void task_eth_chk(void *arg)
{
    while(1) {
        eth_check();
    }
}
int net2_test(void)
{
    int i=0;
    char tmp[30];

#ifdef OS_KERNEL
    task_create(TASK_TEST, task_eth_chk, 1024);
#endif
    
    net2_init();
    net2_loop();
    
    return 0;
}






