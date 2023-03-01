#include "net.h"

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

#define HOST_IP         "192.168.2.70"

#define DEV_PORT        8888
#define DEV_IP          "192.168.2.88"
#define DEV_IP_MASK     "255.255.255.0"
#define DEV_GATEWAY     "192.168.2.1"


typedef struct {
    eth_handle_t        eth;
    int                 connected;
}net_handle_t;




///////////////////////////////////////
static err_t tcp_server_accepted(void *arg, tcp_pcb_t *newpcb, err_t err);
static err_t tcp_server_recv(void *arg, tcp_pcb_t *tpcb, pbuf_t *p, err_t err);
static void tcp_server_init(void);

static err_t tcp_client_connected(void *arg, struct tcp_pcb *pcb, err_t err);
static err_t tcp_client_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *tcp_recv_pbuf, err_t err);
static void tcp_client_init(void);

static void my_test(void);


#define RECV_BUF_LEN  1024
static U8 recv_buffer[RECV_BUF_LEN];
static net_handle_t netHandle={0};
static U32 recved_len=0;
static tcp_pcb_t *tcp_pcb=NULL;


/////////////////////////////////////////////////////////////////////////////////////////////////////
static err_t tcp_server_accepted(void *arg, tcp_pcb_t *pcb, err_t err)
{
    //printf("tcp client connected\r\n");
    //tcp_write(pcb, "tcp client connected", strlen("tcp client connected"), 0);
    
    /* 注册接收回调函数 */
    tcp_recv(pcb, tcp_server_recv);
    netHandle.connected = 1;
    tcp_pcb = pcb;

    return ERR_OK;
}
static err_t tcp_server_recv(void *arg, tcp_pcb_t *tpcb, pbuf_t *p, err_t err)
{
    uint32_t i;
    
    //tcp_write(tpcb, p->payload, p->len, 1);

    if (err == ERR_OK) {
        printf("tcp client closed\r\n");
        tcp_recved(tpcb, p->tot_len);
        
        return tcp_close(tpcb);
    }
    else {
        if (p) {
            pbuf_t *ptmp = p;
            
            while(ptmp != NULL) {
                for (i = 0; i < p->len; i++) {
                    //printf("%c", *((char *)p->payload + i));
                }
                
                ptmp = p->next;
            }
            
            tcp_recved(tpcb, p->tot_len);
            
            /* 释放缓冲区数据 */
            pbuf_free(p);
        }
    }

    return ERR_OK;
}
static void tcp_server_init(void)
{
    tcp_pcb_t *pcb=tcp_new();

    if (pcb) {
        
        /* 绑定端口接收，接收对象为所有ip地址 */
        err_t e = tcp_bind(pcb, IP_ADDR_ANY, DEV_PORT);
        if (e == ERR_OK) {
            
            pcb = tcp_listen(pcb);
            tcp_accept(pcb, tcp_server_accepted);
            //printf("tcp server listening\r\n");
        }
        else {
            memp_free(MEMP_TCP_PCB, pcb);
            //printf("can not bind pcb\r\n");
        }
    }
}

static err_t tcp_client_connected(void *arg, tcp_pcb_t *pcb, err_t err)
{
    netHandle.connected = 1;
    tcp_pcb = pcb;

    return ERR_OK;
 }
static err_t tcp_client_recv(void *arg, tcp_pcb_t *pcb, pbuf_t *pbuf, err_t err)
{ 
    if (err != ERR_OK) {
        tcp_close(pcb);
        tcp_client_init();
    }
    else {
        if (pbuf) {
            tcp_recved(pcb, pbuf->tot_len);
            pbuf_free(pbuf);
        }
    }
  
    return err;
}
static void tcp_client_connect_error(void *arg, err_t err)
{
    tcp_client_init();
}
static void tcp_client_init(void)
{
    tcp_pcb_t *pcb;

    pcb = tcp_new();
    if (pcb) {
        err_t e;
        ip_addr_t ipaddr;
        
        e = tcp_bind(pcb, IP_ADDR_ANY, 8888);
        
        ip4addr_aton(HOST_IP, &ipaddr);
        e = tcp_connect(pcb, &ipaddr, 9999, tcp_client_connected);
        
        tcp_recv(pcb, tcp_client_recv);
        tcp_err(pcb, tcp_client_connect_error);
    }
}
static void tcp_test(void)
{
    static err_t e=0;
    char echosStr[]="123456\n";
    
    tcp_client_init();
    //tcp_server_init();
    while(1) {
        if(netHandle.connected) {
            e = tcp_write(tcp_pcb, echosStr, strlen(echosStr), 1);
            delay_ms(200);
        }
    }
}

////////////////////////////////////////////////////////////////////////

static void net_link_callback(netif_t *netif)
{
     if(netif_is_link_up(netif)) {
        //connected
         netHandle.connected = 1;
     }
     else {
        //disconnected
         netHandle.connected = 0;
     }
}
static void net_data_callback(netconn_t *conn, void *data, int len)
{
    netconn_write(conn, data, len, NETCONN_NOCOPY);
}

static int net_data_recv(void *data, int len)
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
















int net2_init(void)
{
    err_t e;

    netHandle.connected = 0;
    netHandle.eth.link_cb = net_link_callback;

    set_ipaddr(&netHandle.eth.addr);
    eth_init(&netHandle.eth);
    
#ifdef OS_KERNEL
    netHandle.eth.conn = netconn_new(NETCONN_TCP);
    if(!netHandle.eth.conn) {
        return -1;
    }
    //ip_set_option(netHandle.conn->pcb.tcp, SOF_REUSEADDR);

    e = netconn_bind(netHandle.eth.conn, IP_ADDR_ANY, DEV_PORT);
    e = netconn_listen(netHandle.eth.conn);
#else
    tcp_test();
#endif

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
#ifdef OS_KERNEL
        e = netconn_accept(netHandle.eth.conn, &newconn);
        if(e==ERR_OK) {
            while(1) {
                e = netconn_recv(newconn, &rbuf);
                if(e==ERR_OK) {
                    for(p=rbuf->ptr;p!=NULL;p=p->next) {
                        r = net_data_recv(p->payload, p->len);
                        if(r<0) {
                            break;
                        }
                    }
                    
                    net_data_callback(newconn, recv_buffer, recved_len);
                    recved_len = 0;
                }
                netbuf_delete(rbuf);
            }
        }
        //rbuf->
        
#else
    //
#endif
    }
    
    return 0;
}



int net2_test(void)
{
    int i=0;
    char tmp[30];
    
    net2_init();
    net2_loop();

    
    return 0;
}






