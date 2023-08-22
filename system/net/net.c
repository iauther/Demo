#include "net.h"
#include "task.h"

//https://wenku.baidu.com/view/43a00bbf52e79b89680203d8ce2f0066f53364ad.html
//https://www.shuzhiduo.com/A/LPdo0K8Ez3/
//https://blog.csdn.net/qq_31847339/article/details/95051300      �ְ��ֽ̳�
//https://shequ.stmicroelectronics.cn/forum.php?mod=viewthread&tid=615089
//https://blog.csdn.net/Motseturtle/article/details/126165780      H723ZET6�ȿӹ���
//https://www.stmcu.org.cn/module/forum/forum.php?mod=viewthread&tid=615089&page=1&extra=
//https://blog.csdn.net/qq_45607873/article/details/126843107
//https://blog.csdn.net/monei3525/article/details/108941383?spm=1001.2101.3001.6650.2&utm_medium=distribute.pc_relevant.none-task-blog-2%7Edefault%7ECTRLIST%7ERate-2-108941383-blog-121591919.pc_relevant_recovery_v2&depth_1-utm_source=distribute.pc_relevant.none-task-blog-2%7Edefault%7ECTRLIST%7ERate-2-108941383-blog-121591919.pc_relevant_recovery_v2&utm_relevant_index=5
/*
MPUʹ�ý̳̣�
https://blog.csdn.net/xuzhexing/article/details/115401554?spm=1001.2101.3001.6650.5&utm_medium=distribute.pc_relevant.none-task-blog-2%7Edefault%7ECTRLIST%7ERate-5-115401554-blog-126165780.pc_relevant_3mothn_strategy_and_data_recovery&depth_1-utm_source=distribute.pc_relevant.none-task-blog-2%7Edefault%7ECTRLIST%7ERate-5-115401554-blog-126165780.pc_relevant_3mothn_strategy_and_data_recovery&utm_relevant_index=6

Lwipʹ��DMA������Ϣ����Ӧ��DMA�ڴ涨����sram�С�H7��sram��Ϊ�ü��Σ����ٶ�Ϊcpu����ͨ�׵�˵������һ�������û���д�ĳ���ʹ�ã�
���ǲ�����DMAʹ�á�����ΪDMA������ڴ��������Ҫ�ܿ���һ�Ρ�����Lwipʹ��DMAʱ���ڽ�����ȡ���⣬�ܿ���һ�κ�Ҳ������cpu��ʹ��
��ͨcache��������ʹ�ã����򽫿��ܳ����������⡣�ܶ�����F7��H7��LwipЭ��ջ������ping��ͨ�����󣬶����ڴ�������⡣����������
��Ҫʹ���ڴ��ػ���ԪMPU��

ʹ��CubeMX����MPU�������Թ���16�Ρ�Ϊlwip���ã��������μ��ɡ�

MPU�趨�ܽ�(�ǲ�������), ��������ԭ����Ŀ�ģ�
��1��Lwip��������ʹ��cpuר�õĸ���L1���棨DTCM����ֻ����D2 Sram����
��2��cpu�����������cache��Ϊ��ֹ���������Lwip��DMA�α�����device���ͻ���Strongly-ordered���ͣ���֤����
��3��ͨ��MPU�������cache������һ������share������buffer������Ϊ256Byte����TXRX������ȡͷ������һ�β�share����buffer����cache������32k��

�������кܶ����ʱ���share/buffer/cache�������Ҫ��ϸ�˽⣬���Բο�����һƪ���ģ�
STM32H7��Cache��Buffer     https://blog.csdn.net/monei3525/article/details/109166516

������ƪ�����������ԭ�������ù�������ϸ������
https://community.st.com/s/article/FAQ-Ethernet-not-working-on-STM32H7x3
https://community.st.com/s/article/How-to-create-project-for-STM32H7-with-Ethernet-and-LwIP-stack-working
*/



//LwIP �ṩ�����ֱ�̽ӿڣ��ֱ�Ϊ RAW/Callback API�� NETCONN API�� SOCKETAPI. ���ǵ������Դ�����������ߣ���ִ��Ч�ʴ��������ν���


#define BIND_PORT        8888


static int tcp_connected=0;

///////////////////////////////////////

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

#ifdef USE_LWIP
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
#endif

#define RECV_LEN_MAX  2000
typedef struct {
    U8                  buff[RECV_LEN_MAX];
    U32                 rlen;
}conn_buf_t;
static void data_task_start(net_handle_t *h);


static int conn_data_recv(conn_buf_t *cbuf, void *data, int len)
{
    int wlen;
    
    if(cbuf->rlen>RECV_LEN_MAX) {
        cbuf->rlen = 0;
        return -1;
    }
    
    wlen = (cbuf->rlen+len>RECV_LEN_MAX)?(RECV_LEN_MAX-cbuf->rlen):len;
    memcpy(cbuf->buff+cbuf->rlen, data, len);
    
    cbuf->rlen += wlen;
    
    return 0;
}

#ifdef OS_KERNEL
static void conn_listen_task(void *arg)
{
    int r;
    
#ifdef USE_LWIP
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
                h->cfg.callback(h, newconn, NET_EVT_NEW_CONN, NULL, 0);
            }
        }
    }
#else
    
#endif
}


static void data_listen_task(void *arg)
{
    int r,quit=0;
    
#ifdef USE_LWIP
    err_t e;
    pbuf_t *p;
    netbuf_t *rbuf=NULL;
    conn_buf_t *cbuf=malloc(sizeof(conn_buf_t));
    net_handle_t *h=(net_handle_t*)arg;
    netconn_t *newconn=h->newconn;

    while(1) {
        e = netconn_recv(newconn, &rbuf);
        if(e==ERR_OK) {
            for(p=rbuf->ptr;p!=NULL;p=p->next) {
                r = conn_data_recv(cbuf, p->payload, p->len);
                if(r<0) {
                    break;
                }
            }
            
            if(h->cfg.callback && cbuf->rlen>0) {
                h->cfg.callback(h, newconn, NET_EVT_DATA_IN, cbuf->buff, cbuf->rlen);
                cbuf->rlen = 0;
            }
            
            netbuf_delete(rbuf);
        }
        else if(e==ERR_CLSD || e==ERR_RST || e==ERR_TIMEOUT || e==ERR_ABRT) {
            if(h->cfg.callback) {
                h->cfg.callback(h, newconn, NET_EVT_DIS_CONN, NULL, 0);
            }
            break;
        }
    }
    free(cbuf);
    netconn_close(newconn);
    netconn_delete(newconn);
    
    osThreadExit();
#else


#endif
    
}

static void conn_task_start(net_handle_t *h)
{
    osThreadAttr_t  attr={0};
    
    attr.name = "connListen";
    attr.stack_size = 512;
    attr.priority = osPriorityAboveNormal;
    osThreadNew(conn_listen_task, h, &attr);
}

static void data_task_start(net_handle_t *h)
{
    osThreadId_t    tid;
    osThreadAttr_t  attr={0};
    
    attr.name = "dataListen";
    attr.stack_size = 512;
    attr.priority = osPriorityBelowNormal;
    tid = osThreadNew(data_listen_task, h, &attr);
}
#endif



net_handle_t *g_net_handle=NULL;
net_cfg_t *gCfg= NULL;
handle_t net_init(net_cfg_t *cfg)
{
    s8 e;
    
    
    net_handle_t *h=calloc(1, sizeof(net_handle_t));
    
    if(!cfg || !h) {
        return NULL;
    }
    g_net_handle = h;

    h->cfg = *cfg;
    

#ifdef USE_ETH
    eth_cfg_t cfg2;
    cfg2.cb = net_link_callback;
    h->eth = eth_init(&cfg2);
    if(h->eth) {
        //eth_set_ip(h->eth, "ip", "ipmask", "gateway");
    }
#endif
    
#ifdef USE_LWIP
#ifdef OS_KERNEL
    task_attr_t attr;
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
        //tcp_test();
#endif
        
#else

#endif

    return h;
}


int net_deinit(handle_t h)
{
    net_handle_t *nh=(net_handle_t*)h;
    
    if(!nh) {
        return -1;
    }

#ifdef USE_LWIP
    //netconn_disconnect();
    netconn_close(nh->conn);
    netconn_delete(nh->conn);
#endif
    
    free(nh);
    
    return 0;
}



int net_write(handle_t h, void *conn, U8 *data, int len)
{
    if(!h || !conn || !data || !len) {
        return -1;
    }
    
#ifdef OS_KERNEL
    #ifdef USE_LWIP
    return netconn_write(conn, data, len, NETCONN_NOCOPY);
    #endif
#else
    #ifdef USE_LWIP
    return tcp_write(conn, data, len, NETCONN_NOCOPY);
    #endif
#endif
    
    return 0;
}


int net_broadcast(handle_t h, U8 *data, int len)
{
    net_handle_t *nh=(net_handle_t*)h;
    
    return 0;
}



