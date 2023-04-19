#include "net.h"
#include "task.h"

//https://wenku.baidu.com/view/43a00bbf52e79b89680203d8ce2f0066f53364ad.html
//https://www.shuzhiduo.com/A/LPdo0K8Ez3/
//https://blog.csdn.net/qq_31847339/article/details/95051300      手把手教程
//https://shequ.stmicroelectronics.cn/forum.php?mod=viewthread&tid=615089
//https://blog.csdn.net/Motseturtle/article/details/126165780      H723ZET6踩坑过程
//https://www.stmcu.org.cn/module/forum/forum.php?mod=viewthread&tid=615089&page=1&extra=
//https://blog.csdn.net/qq_45607873/article/details/126843107
//https://blog.csdn.net/monei3525/article/details/108941383?spm=1001.2101.3001.6650.2&utm_medium=distribute.pc_relevant.none-task-blog-2%7Edefault%7ECTRLIST%7ERate-2-108941383-blog-121591919.pc_relevant_recovery_v2&depth_1-utm_source=distribute.pc_relevant.none-task-blog-2%7Edefault%7ECTRLIST%7ERate-2-108941383-blog-121591919.pc_relevant_recovery_v2&utm_relevant_index=5
/*
MPU使用教程：
https://blog.csdn.net/xuzhexing/article/details/115401554?spm=1001.2101.3001.6650.5&utm_medium=distribute.pc_relevant.none-task-blog-2%7Edefault%7ECTRLIST%7ERate-5-115401554-blog-126165780.pc_relevant_3mothn_strategy_and_data_recovery&depth_1-utm_source=distribute.pc_relevant.none-task-blog-2%7Edefault%7ECTRLIST%7ERate-5-115401554-blog-126165780.pc_relevant_3mothn_strategy_and_data_recovery&utm_relevant_index=6

Lwip使用DMA传递信息，对应的DMA内存定义在sram中。H7的sram分为好几段，高速段为cpu独享，通俗点说就是这一段允许用户编写的程序使用，
但是不允许DMA使用。所以为DMA定义的内存或者数组要避开这一段。另外Lwip使用DMA时存在交互存取问题，避开这一段后，也不能让cpu像使用
普通cache那样乱序使用，否则将可能出现严重问题。很多人用F7、H7和Lwip协议栈都出现ping不通的现象，都是内存管理问题。怎样管理？？
需要使用内存守护单元MPU。

使用CubeMX配置MPU，最多可以管理16段。为lwip配置，管理两段即可。

MPU设定总结(非操作步骤), 这样做的原因与目的：
（1）Lwip不被允许使用cpu专用的高速L1缓存（DTCM），只能用D2 Sram区域；
（2）cpu可以无序访问cache，为防止这种情况，Lwip的DMA段必须是device类型或者Strongly-ordered类型，保证有序；
（3）通过MPU配置这段cache，其中一段允许share、允许buffer，长度为256Byte，放TXRX交互存取头；另外一段不share，不buffer，不cache；长度32k。

这里面有很多名词比如share/buffer/cache，如果需要详细了解，可以参考另外一篇博文：
STM32H7的Cache与Buffer     https://blog.csdn.net/monei3525/article/details/109166516

以下两篇对于这个问题原理与配置过程有详细描述：
https://community.st.com/s/article/FAQ-Ethernet-not-working-on-STM32H7x3
https://community.st.com/s/article/How-to-create-project-for-STM32H7-with-Ethernet-and-LwIP-stack-working
*/



//LwIP 提供了三种编程接口，分别为 RAW/Callback API、 NETCONN API、 SOCKETAPI. 它们的易用性从左到右依次提高，而执行效率从左到右依次降低


#define BIND_PORT        8888


static int tcp_connected=0;
static tcp_pcb_t *tcp_pcb=NULL;

///////////////////////////////////////
static err_t tcp_server_accepted(void *arg, tcp_pcb_t *newpcb, err_t err);
static err_t tcp_server_recv(void *arg, tcp_pcb_t *tpcb, pbuf_t *p, err_t err);
static void tcp_server_init(void);

static err_t tcp_client_connected(void *arg, struct tcp_pcb *pcb, err_t err);
static err_t tcp_client_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *tcp_recv_pbuf, err_t err);
static void tcp_client_init(void);

static void my_test(void);




#if LWIP_DHCP
#define MAX_DHCP_TRIES  4
__IO uint8_t DHCP_state = DHCP_OFF;
void DHCP_Thread(void const * argument)
{
    struct netif *netif = (struct netif *) argument;
    ip_addr_t ipaddr;
    ip_addr_t netmask;
    ip_addr_t gw;
    struct dhcp *dhcp;

    for (;;) {
        switch (DHCP_state)
        {
            case DHCP_START:
            {
                ip_addr_set_zero_ip4(&netif->ip_addr);
                ip_addr_set_zero_ip4(&netif->netmask);
                ip_addr_set_zero_ip4(&netif->gw);
                DHCP_state = DHCP_WAIT_ADDRESS;

                dhcp_start(netif);
            }
            break;
          
            case DHCP_WAIT_ADDRESS:
            {
                if (dhcp_supplied_address(netif))
                {
                    DHCP_state = DHCP_ADDRESS_ASSIGNED;
                }
                else
                {
                    dhcp = (struct dhcp *)netif_get_client_data(netif, LWIP_NETIF_CLIENT_DATA_INDEX_DHCP);

                    /* DHCP timeout */
                    if (dhcp->tries > MAX_DHCP_TRIES)
                    {
                        DHCP_state = DHCP_TIMEOUT;

                        /* Static address used */
                        //IP_ADDR4(&ipaddr, IP_ADDR0 ,IP_ADDR1 , IP_ADDR2 , IP_ADDR3 );
                        //IP_ADDR4(&netmask, NETMASK_ADDR0, NETMASK_ADDR1, NETMASK_ADDR2, NETMASK_ADDR3);
                        //IP_ADDR4(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
                        //set_ipaddr(&netHandle.eth.addr);
                        netif_set_addr(netif, ip_2_ip4(&ipaddr), ip_2_ip4(&netmask), ip_2_ip4(&gw));
                    }
                }
            }
            break;
            
            case DHCP_LINK_DOWN:
            {
                DHCP_state = DHCP_OFF;

            }
            break;
            
            default:
                break;
        }

        osDelay(500);
    }
}
#endif  /* LWIP_DHCP */
/////////////////////////////////////////////////////////////////////////////////////////////////////

static void net_link_callback(netif_t *netif)
{
     if(netif_is_link_up(netif)) {
        //connected
         tcp_connected = 1;
     }
     else {
        //disconnected
         tcp_connected = 0;
     }
}




static int net_data_recv(net_handle_t *h, void *data, int len)
{
    int wlen;
    
    if(h->rlen>=RECV_BUF_LEN) {
        return -1;
    }
    
    wlen = (h->rlen+len>RECV_BUF_LEN)?(RECV_BUF_LEN-h->rlen):len;
    memcpy(h->buff+h->rlen, data, len);
    
    h->rlen += wlen;
    
    return 0;
}

#ifdef OS_KERNEL
static void data_task_start(net_handle_t *h);

static void conn_listen_task(void *arg)
{
    int r;
    err_t e;
    pbuf_t *p;
    netbuf_t *rbuf=NULL;
    netconn_t *newconn=NULL;
    net_handle_t *h=(net_handle_t*)arg;

    while(1) {
        e = netconn_accept(h->conn, &newconn);
        if(e==ERR_OK) {
            h->newconn = newconn;
            
            data_task_start(h);
            if(h->cfg.callback) {
                h->rlen = 0;
                h->cfg.callback(h, h->newconn, NET_EVT_NEW_CONN, NULL, 0);
            }
        }
    }
}



static void data_listen_task(void *arg)
{
    int r,quit=0;
    err_t e;
    pbuf_t *p;
    netbuf_t *rbuf=NULL;
    net_handle_t *h=(net_handle_t*)arg;

    while(1) {
        e = netconn_recv(h->newconn, &rbuf);
        if(e==ERR_OK) {
            for(p=rbuf->ptr;p!=NULL;p=p->next) {
                r = net_data_recv(h, p->payload, p->len);
                if(r<0) {
                    break;
                }
            }
            
            if(h->cfg.callback) {
                h->cfg.callback(h, h->newconn, NET_EVT_DATA_IN, h->buff, h->rlen);
                h->rlen = 0;
            }
            
            netbuf_delete(rbuf);
        }
        else if(e==ERR_CLSD || e==ERR_RST) {
            if(h->cfg.callback) {
                h->cfg.callback(h, h->newconn, NET_EVT_DIS_CONN, NULL, 0);
            }
            quit = 1;
        }
        
        if(quit) {
            netconn_close(h->newconn);
            netconn_delete(h->newconn);
            h->newconn = NULL;
            break;
        }
    }
    
    osThreadExit();
}

static void conn_task_start(net_handle_t *h)
{
    osThreadAttr_t  attr={0};
    
    attr.name = "connListen";
    attr.stack_size = 1024;
    attr.priority = osPriorityAboveNormal;
    osThreadNew(conn_listen_task, h, &attr);
}

static void data_task_start(net_handle_t *h)
{
    osThreadAttr_t  attr={0};
    
    attr.name = "dataListen";
    attr.stack_size = 1024;
    attr.priority = osPriorityBelowNormal;
    osThreadNew(data_listen_task, h, &attr);
}
#endif



net_handle_t *g_net_handle=NULL;
net_cfg_t *gCfg= NULL;
handle_t net_init(net_cfg_t *cfg)
{
    err_t e;
    task_attr_t attr;
    eth_cfg_t cfg2;
    net_handle_t *h=calloc(1, sizeof(net_handle_t));
    
    if(!cfg || !h) {
        return NULL;
    }
    g_net_handle = h;

    h->connected = 0;
    h->cfg = *cfg;
    
    gCfg = &h->cfg;
    cfg2.cb = net_link_callback;
    h->eth = eth_init(&cfg2);
    if(h->eth) {
        //eth_set_ip(h->eth, "ip", "ipmask", "gateway");
        //h->list = list_init(CONN_MAX, 50);
    }
    
    
#ifdef OS_KERNEL
    h->conn = netconn_new(NETCONN_TCP);
    if(!h->conn) {
        free(h);
        return NULL;
    }
    //ip_set_option(nh->conn->pcb.tcp, SOF_REUSEADDR);

    e = netconn_bind(h->conn, IP_ADDR_ANY, BIND_PORT);
    e = netconn_listen(h->conn);
    
    conn_task_start(h);
#else
    tcp_test();
#endif

    return h;
}


int net_deinit(handle_t *h)
{
    net_handle_t **nh=(net_handle_t**)h;
    
    if(!(*nh)) {
        return -1;
    }
    
    //netconn_disconnect();
    netconn_close((*nh)->conn);
    free(*nh);
    
    return 0;
}



int net_write(handle_t h, netconn_t *conn, U8 *data, int len)
{
    if(!h || !conn || !data || !len) {
        return -1;
    }
    
#ifdef OS_KERNEL
    return netconn_write(conn, data, len, NETCONN_NOCOPY);
#else
    return tcp_write(conn, data, len, NETCONN_NOCOPY);
#endif
}


int net_broadcast(handle_t h, U8 *data, int len)
{
    net_handle_t *nh=(net_handle_t*)h;
    
    return 0;
}



