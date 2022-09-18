#include "net/ethernetif.h"
#include "netif/etharp.h" 
#include "lan8720.h"



typedef enum
{
  RX_ALLOC_OK       = 0x00,
  RX_ALLOC_ERROR    = 0x01
} RxAllocStatusTypeDef;

typedef struct
{
  struct pbuf_custom pbuf_custom;
  uint8_t buff[(ETH_RX_BUFFER_SIZE + 31) & ~31] __ALIGNED(32);
} RxBuff_t;


#define IFNAME0 's'
#define IFNAME1 't'
#define ETH_RX_BUFFER_CNT             12U
#define SIZEOF_ETHARP_PACKET (SIZEOF_ETH_HDR + SIZEOF_ETHARP_HDR)
LWIP_MEMPOOL_DECLARE(RX_POOL, ETH_RX_BUFFER_CNT, sizeof(RxBuff_t), "Zero-copy RX PBUF pool");

__IO uint32_t TxPkt = 0;
__IO uint32_t RxPkt = 0;
static uint8_t RxAllocStatus;
ETH_TxPacketConfig TxConfig;
ETH_HandleTypeDef heth = {0};

// SRAM3：0x30040000，大小32KB(0x8000)，主要用于以太网和USB的缓冲
__attribute__((at(0x30040000))) ETH_DMADescTypeDef  DMARxDscrTab[ETH_RX_DESC_CNT];      // 以太网Rx DMA描述符(0x30040000~0x300401DF，大小0x1E0)
__attribute__((at(0x300401E0))) ETH_DMADescTypeDef  DMATxDscrTab[ETH_TX_DESC_CNT];      // 以太网Tx DMA描述符(0x300401E0~0x300403BF，大小0x1E0)
__attribute__((at(0x30040400))) uint8_t Rx_Buff[ETH_RX_DESC_CNT][ETH_MAX_PACKET_SIZE];  // 以太网接收缓冲区(0x30040400~0x30047B5F，大小0x7760)




static void enable_int(void)
{
	//asm("CPSIE I");
	//asm("BX LR");
}
static void disable_int(void)
{
	//__asm("CPSID I");
	//__asm("BX LR");
}
// 设置网络所使用的0x30040000的RAM内存保护
static void eth_mpu_config(void)
{
    MPU_Region_InitTypeDef MPU_InitStruct = {0};
    HAL_MPU_Disable();

    MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.BaseAddress = 0x30040000;
    MPU_InitStruct.Size = MPU_REGION_SIZE_32KB;
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    MPU_InitStruct.Number = MPU_REGION_NUMBER5;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
    MPU_InitStruct.SubRegionDisable = 0x00;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);

    MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.BaseAddress = 0x30040000;
    MPU_InitStruct.Size = MPU_REGION_SIZE_1KB;
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
    MPU_InitStruct.Number = MPU_REGION_NUMBER6;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
    MPU_InitStruct.SubRegionDisable = 0x00;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);

    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}


static void pbuf_free_custom(struct pbuf *p)
{
  struct pbuf_custom* custom_pbuf = (struct pbuf_custom*)p;
  LWIP_MEMPOOL_FREE(RX_POOL, custom_pbuf);

  /* If the Rx Buffer Pool was exhausted, signal the ethernetif_input task to
   * call HAL_ETH_GetRxDataBuffer to rebuild the Rx descriptors. */

  if (RxAllocStatus == RX_ALLOC_ERROR)
  {
    RxAllocStatus = RX_ALLOC_OK;
    RxPkt = 1 ;
  }
}

static uint8_t MACAddr[6]={0x00,0x80,0xE1,0x00,0x00,0x00};
static void low_level_init(struct netif *netif)
{
    uint32_t idx = 0;
    ETH_MACConfigTypeDef MACConf;
    u32 PHYLinkState;
    u32 speed=0,duplex=0;
    
    heth.Instance=ETH;                             //ETH
    heth.Init.MACAddr=MACAddr;                  //mac地址
    heth.Init.MediaInterface=HAL_ETH_RMII_MODE;    //RMII接口
    heth.Init.RxDesc=DMARxDscrTab;                 //发送描述符
    heth.Init.TxDesc=DMATxDscrTab;                 //接收描述符
    heth.Init.RxBuffLen=ETH_MAX_PACKET_SIZE;       //接收长度
    HAL_ETH_Init(&heth);                           //初始化ETH
    HAL_ETH_SetMDIOClockRange(&heth);
    
    eth_mpu_config();
    lan8720_init();
    //__enable_irq();
    
    netif->hwaddr_len=ETHARP_HWADDR_LEN;  //设置MAC地址长度,为6个字节
	netif->hwaddr[0] = heth.Init.MACAddr[0];
    netif->hwaddr[1] = heth.Init.MACAddr[1];
    netif->hwaddr[2] = heth.Init.MACAddr[2];
    netif->hwaddr[3] = heth.Init.MACAddr[3];
    netif->hwaddr[4] = heth.Init.MACAddr[4];
    netif->hwaddr[5] = heth.Init.MACAddr[5];
    netif->mtu=ETH_MAX_PAYLOAD;         //最大允许传输单元,允许该网卡广播和ARP功能
  
    netif->flags|=NETIF_FLAG_BROADCAST|NETIF_FLAG_ETHARP|NETIF_FLAG_LINK_UP;
    
    memset(&TxConfig,0,sizeof(ETH_TxPacketConfig));
    TxConfig.Attributes=ETH_TX_PACKETS_FEATURES_CSUM|ETH_TX_PACKETS_FEATURES_CRCPAD;
    TxConfig.ChecksumCtrl=ETH_CHECKSUM_IPHDR_PAYLOAD_INSERT_PHDR_CALC;
    TxConfig.CRCPadCtrl=ETH_CRC_PAD_INSERT;
    PHYLinkState=lan8720_get_link_state();    //获取连接状态
    switch (PHYLinkState)
    {
        case LAN8720_STATUS_100MBITS_FULLDUPLEX:    //100M全双工
            duplex=ETH_FULLDUPLEX_MODE;
            speed=ETH_SPEED_100M;
            break;
        case LAN8720_STATUS_100MBITS_HALFDUPLEX:    //100M半双工
            duplex=ETH_HALFDUPLEX_MODE;
            speed=ETH_SPEED_100M;
            break;
        case LAN8720_STATUS_10MBITS_FULLDUPLEX:     //10M全双工
            duplex=ETH_FULLDUPLEX_MODE;
            speed=ETH_SPEED_10M;
            break;
        case LAN8720_STATUS_10MBITS_HALFDUPLEX:     //10M半双工
            duplex=ETH_HALFDUPLEX_MODE;
            speed=ETH_SPEED_10M;
            break;
        default:
            duplex=ETH_FULLDUPLEX_MODE;
            speed=ETH_SPEED_100M;
            break;
    }
    
    HAL_ETH_GetMACConfig(&heth, &MACConf); 
    MACConf.DuplexMode=duplex;
    MACConf.Speed=speed;
    HAL_ETH_SetMACConfig(&heth,&MACConf);  //设置MAC
    
    HAL_ETH_Start_IT(&heth);
    netif_set_up(netif);                        //打开网卡
    netif_set_link_up(netif);                   //开启网卡连接
    
}

static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
    uint32_t i=0, framelen = 0;
    struct pbuf *q;
    err_t errval=ERR_OK;
    ETH_BufferTypeDef Txbuffer[ETH_TX_DESC_CNT];
    
    for(q=p;q!=NULL;q=q->next)
    {
        if(i>=ETH_TX_DESC_CNT)
            return ERR_IF;
    
        Txbuffer[i].buffer=q->payload;
        Txbuffer[i].len=q->len;
        framelen+=q->len;
    
        if(i>0)
        {
            Txbuffer[i-1].next=&Txbuffer[i];
        }
    
        if(q->next == NULL)
        {
            Txbuffer[i].next=NULL;
        }
        i++;
    }

    TxConfig.Length=framelen;
    TxConfig.TxBuffer=Txbuffer;

    SCB_CleanInvalidateDCache();    //无效化并清除Dcache
    HAL_ETH_Transmit(&heth,&TxConfig,0);
    
    return errval;
}

//用于接收数据包的最底层函数
//neitif:网卡结构体指针
//返回值:pbuf数据结构体指针
static struct pbuf *low_level_input(struct netif *netif)
{
    struct pbuf *p = NULL;

  if(RxAllocStatus == RX_ALLOC_OK)
  {
    HAL_ETH_ReadData(&heth, (void **)&p);
  }

  return p;
    
    return p;
}

//网卡接收数据(lwip直接调用)
//netif:网卡结构体指针
//返回值:ERR_OK,发送正常
//       ERR_MEM,发送失败
err_t ethernetif_input(struct netif *netif)
{
    err_t err;
    struct pbuf *p;
    
    p = low_level_input(netif);
    if (p == NULL) 
        return ERR_MEM;
    err = netif->input(p, netif);
    if (err != ERR_OK)
    {
        LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
        pbuf_free(p);
        p=NULL;
    }
    
    return err;
}

struct netif *pnetif=NULL;
err_t ethernetif_init(struct netif *netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));
  
#if LWIP_NETIF_HOSTNAME
    netif->hostname="lwip"; //主机名字
#endif
    
    pnetif = netif;
    
    netif->name[0]=IFNAME0;
    netif->name[1]=IFNAME1;
    netif->output=etharp_output;
    netif->linkoutput=low_level_output;
    low_level_init(netif);
    return ERR_OK;
}


//网络连接状态检查
void ethernet_link_check_state(struct netif *netif)
{
    int link_up;
    ETH_MACConfigTypeDef MACConf;
    u32 PHYLinkState;
    u32 linkchanged=0,speed=0,duplex=0;
    
    PHYLinkState=lan8720_get_link_state();    //获取连接状态
    link_up = netif_is_link_up(netif);
    
    //如果检测到连接断开或者不正常就关闭网口
    if(link_up && (PHYLinkState<=LAN8720_STATUS_LINK_DOWN))
    {
        HAL_ETH_Stop_IT(&heth);
        netif_set_down(netif);              //关闭网口
        netif_set_link_down(netif);         //连接关闭
    }
    else if(!link_up && (PHYLinkState>LAN8720_STATUS_LINK_DOWN))
    {
        switch (PHYLinkState)
        {
            case LAN8720_STATUS_100MBITS_FULLDUPLEX:    //100M全双工
                duplex=ETH_FULLDUPLEX_MODE;
                speed=ETH_SPEED_100M;
                linkchanged=1;
                break;
            case LAN8720_STATUS_100MBITS_HALFDUPLEX:    //100M半双工
                duplex=ETH_HALFDUPLEX_MODE;
                speed=ETH_SPEED_100M;
                linkchanged=1;
                break;
            case LAN8720_STATUS_10MBITS_FULLDUPLEX:     //10M全双工
                duplex=ETH_FULLDUPLEX_MODE;
                speed=ETH_SPEED_10M;
                linkchanged=1;
                break;
            case LAN8720_STATUS_10MBITS_HALFDUPLEX:     //10M半双工
                duplex=ETH_HALFDUPLEX_MODE;
                speed=ETH_SPEED_10M;
                linkchanged=1;
                break;
            default:
                break;
        }
    
        if(linkchanged)                                 //连接正常
        {
            HAL_ETH_GetMACConfig(&heth,&MACConf);
            MACConf.DuplexMode=duplex;
            MACConf.Speed=speed;
            HAL_ETH_SetMACConfig(&heth,&MACConf);  //设置MAC
            HAL_ETH_Start_IT(&heth);
            netif_set_up(netif);                        //打开网卡
            netif_set_link_up(netif);                   //开启网卡连接
        }
    }
}


void HAL_ETH_RxAllocateCallback(uint8_t **buff)
{
/* USER CODE BEGIN HAL ETH RxAllocateCallback */

  struct pbuf_custom *p = LWIP_MEMPOOL_ALLOC(RX_POOL);
  if (p)
  {
    /* Get the buff from the struct pbuf address. */
    *buff = (uint8_t *)p + offsetof(RxBuff_t, buff);
    p->custom_free_function = pbuf_free_custom;
    /* Initialize the struct pbuf.
    * This must be performed whenever a buffer's allocated because it may be
    * changed by lwIP or the app, e.g., pbuf_free decrements ref. */
    pbuf_alloced_custom(PBUF_RAW, 0, PBUF_REF, p, *buff, ETH_RX_BUFFER_SIZE);
  }
  else
  {
    RxAllocStatus = RX_ALLOC_ERROR;
    *buff = NULL;
  }
/* USER CODE END HAL ETH RxAllocateCallback */
}

void HAL_ETH_RxLinkCallback(void **pStart, void **pEnd, uint8_t *buff, uint16_t Length)
{
/* USER CODE BEGIN HAL ETH RxLinkCallback */

  struct pbuf **ppStart = (struct pbuf **)pStart;
  struct pbuf **ppEnd = (struct pbuf **)pEnd;
  struct pbuf *p = NULL;

  /* Get the struct pbuf from the buff address. */
  p = (struct pbuf *)(buff - offsetof(RxBuff_t, buff));
  p->next = NULL;
  p->tot_len = 0;
  p->len = Length;

  /* Chain the buffer. */
  if (!*ppStart)
  {
    /* The first buffer of the packet. */
    *ppStart = p;
  }
  else
  {
    /* Chain the buffer to the end of the packet. */
    (*ppEnd)->next = p;
  }
  *ppEnd  = p;

  /* Update the total length of all the buffers of the chain. Each pbuf in the chain should have its tot_len
   * set to its own length, plus the length of all the following pbufs in the chain. */
  for (p = *ppStart; p != NULL; p = p->next)
  {
    p->tot_len += Length;
  }

  /* Invalidate data cache because Rx DMA's writing to physical memory makes it stale. */
  SCB_InvalidateDCache_by_Addr((uint32_t *)buff, Length);
}

void HAL_ETH_TxFreeCallback(uint32_t * buff)
{
  pbuf_free((struct pbuf *)buff);
}


u32_t sys_now(void)
{
  return HAL_GetTick();
}



void ETH_IRQHandler(void)
{
    ethernetif_input(pnetif);
    __HAL_ETH_DMA_CLEAR_IT(&heth, ETH_DMA_NORMAL_IT);
    __HAL_ETH_DMA_CLEAR_IT(&heth, ETH_DMA_RX_IT);
    __HAL_ETH_DMA_CLEAR_IT(&heth, ETH_DMA_TX_IT);
    
}
void HAL_ETH_MspInit(ETH_HandleTypeDef *heth)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_GPIOA_CLK_ENABLE();		// 开启GPIOA时钟
    __HAL_RCC_GPIOC_CLK_ENABLE();		// 开启GPIOC时钟
    __HAL_RCC_GPIOG_CLK_ENABLE();		// 开启GPIOG时钟
	__HAL_RCC_GPIOH_CLK_ENABLE();		// 开启GPIOH时钟
    __HAL_RCC_ETH1MAC_CLK_ENABLE();		// 使能ETH1 MAC时钟
    __HAL_RCC_ETH1TX_CLK_ENABLE();		// 使能ETH1发送时钟
    __HAL_RCC_ETH1RX_CLK_ENABLE();		// 使能ETH1接收时钟

	// PH2(ETH_RESET)
	GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;  	// 推挽输出
    GPIO_InitStruct.Pull = GPIO_NOPULL;				// 不带上下拉
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;	// 低速
    HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);
	HAL_GPIO_WritePin(GPIOH, GPIO_PIN_2, GPIO_PIN_RESET);

	// PA1 PA2 PA7
    GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;			// 推挽复用
    GPIO_InitStruct.Pull = GPIO_NOPULL;				// 不带上下拉
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;	// 高速
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;		// 复用为ETH功能
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // PC1 PC4 PC5
    GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    // PG11 PG13 PG14
    GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_13 | GPIO_PIN_14;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(ETH_IRQn, 6, 0); // 网络中断优先级应该高一点
    HAL_NVIC_EnableIRQ(ETH_IRQn);
}







