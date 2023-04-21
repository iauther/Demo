/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : ethernetif.c
  * Description        : This file provides code for the configuration
  *                      of the ethernetif.c MiddleWare.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "lwip.h"
#include "eth/lan8742.h"
#include "dal_eth.h"


//https://community.st.com/s/article/FAQ-Ethernet-not-working-on-STM32H7x3


#if defined   (__CC_ARM)
    //#pragma O0
#elif defined (__GNUC__)
    //#pragma GCC optimize ("O1")
#endif /* __CC_ARM */



#define TIME_WAITING_FOR_INPUT                 (osWaitForever)
#define INTERFACE_THREAD_STACK_SIZE            (512)

/* Network interface name */
#define IFNAME0 's'
#define IFNAME1 't'

#define ETH_DMA_TRANSMIT_TIMEOUT                (20U)
#define ETH_RX_BUFFER_SIZE                      1000U
#define ETH_TX_BUFFER_MAX                       ((ETH_TX_DESC_CNT) * 2U)
/* ETH_RX_BUFFER_SIZE parameter is defined in lwipopts.h */

/*
@Note: This interface is implemented to operate in zero-copy mode only:
        - Rx buffers are allocated statically and passed directly to the LwIP stack
          they will return back to ETH DMA after been processed by the stack.
        - Tx Buffers will be allocated from LwIP stack memory heap,
          then passed to ETH HAL driver.

@Notes:
  1.a. ETH DMA Rx descriptors must be contiguous, the default count is 4,
       to customize it please redefine ETH_RX_DESC_CNT in ETH GUI (Rx Descriptor Length)
       so that updated value will be generated in stm32xxxx_hal_conf.h
  1.b. ETH DMA Tx descriptors must be contiguous, the default count is 4,
       to customize it please redefine ETH_TX_DESC_CNT in ETH GUI (Tx Descriptor Length)
       so that updated value will be generated in stm32xxxx_hal_conf.h

  2.a. Rx Buffers number must be between ETH_RX_DESC_CNT and 2*ETH_RX_DESC_CNT
  2.b. Rx Buffers must have the same size: ETH_RX_BUFFER_SIZE, this value must
       passed to ETH DMA in the init field (heth.Init.RxBuffLen)
  2.c  The RX Ruffers addresses and sizes must be properly defined to be aligned
       to L1-CACHE line size (32 bytes).
*/

/* Data Type Definitions */
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

/* Memory Pool Declaration */
#define ETH_RX_BUFFER_CNT             12U
LWIP_MEMPOOL_DECLARE(RX_POOL, ETH_RX_BUFFER_CNT, sizeof(RxBuff_t), "Zero-copy RX PBUF pool");



/* Variable Definitions */
static uint8_t RxAllocStatus;
struct netif *ethif=NULL;

#if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma location=0x30000000
    ETH_DMADescTypeDef  DMARxDscrTab[ETH_RX_DESC_CNT]; /* Ethernet Rx DMA Descriptors */
    #pragma location=0x30000200
    ETH_DMADescTypeDef  DMATxDscrTab[ETH_TX_DESC_CNT]; /* Ethernet Tx DMA Descriptors */
    #pragma location=0x30000400
    uint8_t Rx_Buff[ETH_RX_DESC_CNT][ETH_RX_BUFFER_SIZE]; /* Ethernet Receive Buffers */
	#pragma location = 0x30000400
    extern u8_t memp_memory_RX_POOL_base[];
#elif defined ( __CC_ARM )  /* MDK ARM Compiler */
    ETH_DMADescTypeDef  DMARxDscrTab[ETH_RX_DESC_CNT]  __attribute__((at(0x30040000)));      // 以太网Rx DMA描述符(0x30040000~0x300401DF，大小0x1E0)
    ETH_DMADescTypeDef  DMATxDscrTab[ETH_TX_DESC_CNT]  __attribute__((at(0x30040200))) ;      // 以太网Tx DMA描述符(0x300401E0~0x300403BF，大小0x1E0)
    extern uint8_t memp_memory_RX_POOL_base[]          __attribute__((at(0x30040400))) ;
#elif defined ( __GNUC__ ) /* GNU Compiler */
    ETH_DMADescTypeDef DMARxDscrTab[ETH_RX_DESC_CNT] __attribute__((section(".RxDecripSection"))); /* Ethernet Rx DMA Descriptors */
    ETH_DMADescTypeDef DMATxDscrTab[ETH_TX_DESC_CNT] __attribute__((section(".TxDecripSection")));   /* Ethernet Tx DMA Descriptors */
	__attribute__((section(".Rx_PoolSection"))) extern u8_t memp_memory_RX_POOL_base[];
    uint8_t Rx_Buff[ETH_RX_DESC_CNT][ETH_RX_BUFFER_SIZE] __attribute__((section(".RxArraySection"))); /* Ethernet Receive Buffers */
#endif


/* Global Ethernet handle */
ETH_HandleTypeDef heth;
ETH_TxPacketConfig TxConfig;

/* Private function prototypes -----------------------------------------------*/
int32_t ETH_PHY_IO_Init(void);
int32_t ETH_PHY_IO_DeInit (void);
int32_t ETH_PHY_IO_ReadReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t *pRegVal);
int32_t ETH_PHY_IO_WriteReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t RegVal);
int32_t ETH_PHY_IO_GetTick(void);
static struct pbuf *low_level_input(struct netif *netif);

lan8742_Object_t LAN8742;
lan8742_IOCtx_t  LAN8742_IOCtx = {ETH_PHY_IO_Init,
                                  ETH_PHY_IO_DeInit,
                                  ETH_PHY_IO_WriteReg,
                                  ETH_PHY_IO_ReadReg,
                                  ETH_PHY_IO_GetTick};


/* Private functions ---------------------------------------------------------*/
void pbuf_free_custom(struct pbuf *p); 
                                  
#ifdef OS_KERNEL
osSemaphoreId_t RxPktSemaphore = NULL;   /* Semaphore to signal incoming packets */
osSemaphoreId_t TxPktSemaphore = NULL;   /* Semaphore to signal transmit packet complete */
                                  
void HAL_ETH_RxCpltCallback(ETH_HandleTypeDef *hEth)
{
  osSemaphoreRelease(RxPktSemaphore);
}
void HAL_ETH_TxCpltCallback(ETH_HandleTypeDef *hEth)
{
  osSemaphoreRelease(TxPktSemaphore);
}
void HAL_ETH_ErrorCallback(ETH_HandleTypeDef *hEth)
{
  if((HAL_ETH_GetDMAError(hEth) & ETH_DMACSR_RBU) == ETH_DMACSR_RBU)
  {
     osSemaphoreRelease(RxPktSemaphore);
  }
}
void dal_eth_input(void *argument)
{
    struct pbuf *p = NULL;
    struct netif *netif = (struct netif *)argument;

    for(;;) {
        if (osSemaphoreAcquire(RxPktSemaphore, TIME_WAITING_FOR_INPUT) == osOK)
        {
            do {
                p = low_level_input(netif);
                if (p) {
                    if (netif->input(p, netif) != ERR_OK ) {
                        pbuf_free(p);
                    }
                }
            } while(p);
        }
    }
}
#else
void dal_eth_input(struct netif *netif)
{
  struct pbuf *p = NULL;

    do {
        p = low_level_input( netif );
        if (p) {
            if (netif->input( p, netif) != ERR_OK ) {
                pbuf_free(p);
            }
        }
    } while(p);
}
#endif





void ETH_IRQHandler(void)
{
    HAL_ETH_IRQHandler(&heth);
}



static void LAN8742_Reset(void)
{
    GPIO_InitTypeDef init = {0}; 
    
    __HAL_RCC_GPIOH_CLK_ENABLE();		// 开启GPIOH时钟
    
    // PH2(ETH_RESET)
    init.Pin = GPIO_PIN_2;
    init.Mode = GPIO_MODE_OUTPUT_PP;  	// 推挽输出
    init.Pull = GPIO_NOPULL;			// 不带上下拉
    init.Speed = GPIO_SPEED_FREQ_LOW;	// 低速
    HAL_GPIO_Init(GPIOH, &init);
    
    HAL_GPIO_WritePin(GPIOH, GPIO_PIN_2, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(GPIOH, GPIO_PIN_2, GPIO_PIN_SET);
    HAL_Delay(10);
}
static void eth_mpu_config(void)
{
	MPU_Region_InitTypeDef init = {0};
    
    HAL_MPU_Disable();
    
    init.Enable = MPU_REGION_ENABLE;
    init.BaseAddress = 0x30040000;
    init.Size = MPU_REGION_SIZE_32KB;
    init.AccessPermission = MPU_REGION_FULL_ACCESS;
    init.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    init.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    init.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    init.Number = MPU_REGION_NUMBER5;
    init.TypeExtField = MPU_TEX_LEVEL1;
    init.SubRegionDisable = 0x00;
    init.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    HAL_MPU_ConfigRegion(&init);

    init.Enable = MPU_REGION_ENABLE;
    init.BaseAddress = 0x30040000;
    init.Size = MPU_REGION_SIZE_1KB;
    init.AccessPermission = MPU_REGION_FULL_ACCESS;
    init.IsBufferable = MPU_ACCESS_BUFFERABLE;
    init.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    init.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    init.Number = MPU_REGION_NUMBER6;
    init.TypeExtField = MPU_TEX_LEVEL0;
    init.SubRegionDisable = 0x00;
    init.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    HAL_MPU_ConfigRegion(&init);

    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

static uint8_t MACAddr[6]={0x00,0x80,0xE1,0x00,0x00,0x00};
static void low_level_init(struct netif *netif)
{
    HAL_StatusTypeDef hal_eth_init_status = HAL_OK;

#ifdef OS_KERNEL
    osThreadAttr_t attributes;
    uint32_t duplex, speed = 0;
    int32_t PHYLinkState = 0;
    ETH_MACConfigTypeDef MACConf = {0}; 
#endif

    LAN8742_Reset();
    eth_mpu_config();
 
    
    heth.Instance = ETH;
    heth.Init.MACAddr = MACAddr;
    heth.Init.MediaInterface = HAL_ETH_RMII_MODE;
    heth.Init.TxDesc = DMATxDscrTab;
    heth.Init.RxDesc = DMARxDscrTab;
    heth.Init.RxBuffLen = ETH_RX_BUFFER_SIZE;
    HAL_ETH_Init(&heth);


    LWIP_MEMPOOL_INIT(RX_POOL);

#if LWIP_ARP || LWIP_ETHERNET
    netif->hwaddr_len = ETH_HWADDR_LEN;

    netif->hwaddr[0] =  heth.Init.MACAddr[0];
    netif->hwaddr[1] =  heth.Init.MACAddr[1];
    netif->hwaddr[2] =  heth.Init.MACAddr[2];
    netif->hwaddr[3] =  heth.Init.MACAddr[3];
    netif->hwaddr[4] =  heth.Init.MACAddr[4];
    netif->hwaddr[5] =  heth.Init.MACAddr[5];

    /* maximum transfer unit */
    netif->mtu = ETH_MAX_PAYLOAD;

    /* Accept broadcast address and ARP traffic */
    /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
#if LWIP_ARP
    netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;
#else
    netif->flags |= NETIF_FLAG_BROADCAST;
#endif /* LWIP_ARP */
    /* Set Tx packet config common parameters */
    memset(&TxConfig, 0 , sizeof(ETH_TxPacketConfig));
    TxConfig.Attributes = ETH_TX_PACKETS_FEATURES_CSUM | ETH_TX_PACKETS_FEATURES_CRCPAD;
    TxConfig.ChecksumCtrl = ETH_CHECKSUM_IPHDR_PAYLOAD_INSERT_PHDR_CALC;
    TxConfig.CRCPadCtrl = ETH_CRC_PAD_INSERT;

#ifdef OS_KERNEL
    /* create a binary semaphore used for informing ethernetif of frame reception */
    RxPktSemaphore = osSemaphoreNew(1, 1, NULL);

    /* create a binary semaphore used for informing ethernetif of frame transmission */
    TxPktSemaphore = osSemaphoreNew(1, 1, NULL);

    /* create the task that handles the ETH_MAC */
    /* USER CODE BEGIN OS_THREAD_NEW_CMSIS_RTOS_V2 */
    memset(&attributes, 0x0, sizeof(osThreadAttr_t));
    attributes.name = "ethIf";
    attributes.stack_size = INTERFACE_THREAD_STACK_SIZE;
    attributes.priority = osPriorityRealtime;
    osThreadNew(dal_eth_input, netif, &attributes);
#endif

    /* Set PHY IO functions */
    LAN8742_RegisterBusIO(&LAN8742, &LAN8742_IOCtx);

    LAN8742_Init(&LAN8742);
    
    dal_eth_link_check(netif);
#endif /* LWIP_ARP || LWIP_ETHERNET */

}

/**
 * This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet could be sent
 *         an err_t value if the packet couldn't be sent
 *
 * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
 *       strange results. You might consider waiting for space in the DMA queue
 *       to become available since the stack doesn't retry to send a packet
 *       dropped because of memory failure (except for the TCP timers).
 */

static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
  uint32_t i = 0U;
  struct pbuf *q = NULL;
  err_t errval = ERR_OK;
  ETH_BufferTypeDef Txbuffer[ETH_TX_DESC_CNT];

  memset(Txbuffer, 0 , ETH_TX_DESC_CNT*sizeof(ETH_BufferTypeDef));

  for(q = p; q != NULL; q = q->next)
  {
    if(i >= ETH_TX_DESC_CNT)
      return ERR_IF;

    Txbuffer[i].buffer = q->payload;
    Txbuffer[i].len = q->len;

    if(i>0)
    {
      Txbuffer[i-1].next = &Txbuffer[i];
    }

    if(q->next == NULL)
    {
      Txbuffer[i].next = NULL;
    }

    i++;
  }

  TxConfig.Length = p->tot_len;
  TxConfig.TxBuffer = Txbuffer;
  TxConfig.pData = p;
  pbuf_ref(p);

  SCB_CleanInvalidateDCache();
  
#ifdef OS_KERNEL
  HAL_ETH_Transmit_IT(&heth, &TxConfig);
  while(osSemaphoreAcquire(TxPktSemaphore, TIME_WAITING_FOR_INPUT)!=osOK);
  HAL_ETH_ReleaseTxPacket(&heth);
#else
  HAL_ETH_Transmit(&heth, &TxConfig, ETH_DMA_TRANSMIT_TIMEOUT);
#endif
  
  return errval;
}

/**
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return a pbuf filled with the received packet (including MAC header)
 *         NULL on memory error
   */
static struct pbuf * low_level_input(struct netif *netif)
{
    struct pbuf *p = NULL;
    
    //SCB_CleanInvalidateDCache();
    if(RxAllocStatus == RX_ALLOC_OK) {
        HAL_ETH_ReadData(&heth, (void **)&p);
    }
    //SCB_InvalidateDCache_by_Addr((uint32_t *)Rx_Buff, (ETH_RX_DESC_CNT*ETH_RX_BUFFER_SIZE));

    return p;
}


#if !LWIP_ARP
/**
 * This function has to be completed by user in case of ARP OFF.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if ...
 */
static err_t low_level_output_arp_off(struct netif *netif, struct pbuf *q, const ip4_addr_t *ipaddr)
{
  err_t errval;
  errval = ERR_OK;

/* USER CODE BEGIN 5 */

/* USER CODE END 5 */

  return errval;

}
#endif /* LWIP_ARP */

/**
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
err_t dal_eth_init(struct netif *netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));

    ethif = netif;
    
#if LWIP_NETIF_HOSTNAME
    /* Initialize interface hostname */
    netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

    /*
    * Initialize the snmp variables and counters inside the struct netif.
    * The last argument should be replaced with your link speed, in units
    * of bits per second.
    */
    //MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, LINK_SPEED_OF_YOUR_NETIF_IN_BPS);

    netif->name[0] = IFNAME0;
    netif->name[1] = IFNAME1;
    /* We directly use etharp_output() here to save a function call.
    * You can instead declare your own function an call etharp_output()
    * from it if you have to do some checks before sending (e.g. if link
    * is available...) */

#if LWIP_IPV4
#if LWIP_ARP || LWIP_ETHERNET
#if LWIP_ARP
    netif->output = etharp_output;
#else
    /* The user should write its own code in low_level_output_arp_off function */
    netif->output = low_level_output_arp_off;
#endif /* LWIP_ARP */
#endif /* LWIP_ARP || LWIP_ETHERNET */
#endif /* LWIP_IPV4 */

#if LWIP_IPV6
    netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */

    netif->linkoutput = low_level_output;

    /* initialize the hardware */
    low_level_init(netif);

    return ERR_OK;
}

/**
  * @brief  Custom Rx pbuf free callback
  * @param  pbuf: pbuf to be freed
  * @retval None
  */
void pbuf_free_custom(struct pbuf *p)
{
  struct pbuf_custom* custom_pbuf = (struct pbuf_custom*)p;
  LWIP_MEMPOOL_FREE(RX_POOL, custom_pbuf);

  /* If the Rx Buffer Pool was exhausted, signal the ethernetif_input task to
   * call HAL_ETH_GetRxDataBuffer to rebuild the Rx descriptors. */

  if (RxAllocStatus == RX_ALLOC_ERROR)
  {
    RxAllocStatus = RX_ALLOC_OK;
      
#ifdef OS_KERNEL
    osSemaphoreRelease(RxPktSemaphore);
#endif
  }
}

/* USER CODE BEGIN 6 */

/**
* @brief  Returns the current time in milliseconds
*         when LWIP_TIMERS == 1 and NO_SYS == 1
* @param  None
* @retval Current Time value
*/
u32_t sys_now(void)
{
  return HAL_GetTick();
}


void HAL_ETH_MspInit(ETH_HandleTypeDef* ethHandle)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  
    if(ethHandle->Instance==ETH) {
		__HAL_RCC_ETH1MAC_CLK_ENABLE();
        __HAL_RCC_ETH1TX_CLK_ENABLE();
        __HAL_RCC_ETH1RX_CLK_ENABLE();
		__HAL_RCC_D2SRAM3_CLK_ENABLE();
		
		__HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE();
        __HAL_RCC_GPIOG_CLK_ENABLE();
        
	    // PA1 PA2 PA7
        GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_7;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        
        // PC1 PC4 PC5
        GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
        
        // PG11 PG13 PG14
        GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_13 | GPIO_PIN_14;
        HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

        /* Peripheral interrupt init */
        HAL_NVIC_SetPriority(ETH_IRQn, 8, 0);
        HAL_NVIC_EnableIRQ(ETH_IRQn);
    }
}

void HAL_ETH_MspDeInit(ETH_HandleTypeDef* ethHandle)
{
    if(ethHandle->Instance==ETH) {
        /* Disable Peripheral clock */
        __HAL_RCC_ETH1MAC_CLK_DISABLE();
        __HAL_RCC_ETH1TX_CLK_DISABLE();
        __HAL_RCC_ETH1RX_CLK_DISABLE();
        
		HAL_GPIO_DeInit(GPIOA, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_7);
        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_1|GPIO_PIN_4|GPIO_PIN_5);
        HAL_GPIO_DeInit(GPIOG, GPIO_PIN_11|GPIO_PIN_13|GPIO_PIN_14);

        /* Peripheral interrupt Deinit*/
        HAL_NVIC_DisableIRQ(ETH_IRQn);
    }
}

/*******************************************************************************
                       PHI IO Functions
*******************************************************************************/
/**
  * @brief  Initializes the MDIO interface GPIO and clocks.
  * @param  None
  * @retval 0 if OK, -1 if ERROR
  */
int32_t ETH_PHY_IO_Init(void)
{
  /* We assume that MDIO GPIO configuration is already done
     in the ETH_MspInit() else it should be done here
  */

  /* Configure the MDIO Clock */
  HAL_ETH_SetMDIOClockRange(&heth);

  return 0;
}

/**
  * @brief  De-Initializes the MDIO interface .
  * @param  None
  * @retval 0 if OK, -1 if ERROR
  */
int32_t ETH_PHY_IO_DeInit (void)
{
  return 0;
}

/**
  * @brief  Read a PHY register through the MDIO interface.
  * @param  DevAddr: PHY port address
  * @param  RegAddr: PHY register address
  * @param  pRegVal: pointer to hold the register value
  * @retval 0 if OK -1 if Error
  */
int32_t ETH_PHY_IO_ReadReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t *pRegVal)
{
  if(HAL_ETH_ReadPHYRegister(&heth, DevAddr, RegAddr, pRegVal) != HAL_OK)
  {
    return -1;
  }

  return 0;
}

/**
  * @brief  Write a value to a PHY register through the MDIO interface.
  * @param  DevAddr: PHY port address
  * @param  RegAddr: PHY register address
  * @param  RegVal: Value to be written
  * @retval 0 if OK -1 if Error
  */
int32_t ETH_PHY_IO_WriteReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t RegVal)
{
  if(HAL_ETH_WritePHYRegister(&heth, DevAddr, RegAddr, RegVal) != HAL_OK)
  {
    return -1;
  }

  return 0;
}

/**
  * @brief  Get the time in millisecons used for internal PHY driver process.
  * @retval Time value
  */
int32_t ETH_PHY_IO_GetTick(void)
{
  return HAL_GetTick();
}


/**
  * @brief  Check the ETH link state then update ETH driver and netif link accordingly.
  * @param  argument: netif
  * @retval None
  */
void dal_eth_link_check(struct netif *netif)
{
    ETH_MACConfigTypeDef MACConf = {0};
    int32_t PHYLinkState = 0;
    uint32_t linkchanged = 0U, speed = 0U, duplex = 0U;

    PHYLinkState = LAN8742_GetLinkState(&LAN8742);

    if(netif_is_link_up(netif) && (PHYLinkState <= LAN8742_STATUS_LINK_DOWN))
    {
        HAL_ETH_Stop_IT(&heth);
        netif_set_down(netif);
        netif_set_link_down(netif);
    }
    else if(!netif_is_link_up(netif) && (PHYLinkState > LAN8742_STATUS_LINK_DOWN))
    {
        switch (PHYLinkState)
        {
            case LAN8742_STATUS_100MBITS_FULLDUPLEX:
            duplex = ETH_FULLDUPLEX_MODE;
            speed = ETH_SPEED_100M;
            linkchanged = 1;
            break;
            
            case LAN8742_STATUS_100MBITS_HALFDUPLEX:
            duplex = ETH_HALFDUPLEX_MODE;
            speed = ETH_SPEED_100M;
            linkchanged = 1;
            break;
            
            case LAN8742_STATUS_10MBITS_FULLDUPLEX:
            duplex = ETH_FULLDUPLEX_MODE;
            speed = ETH_SPEED_10M;
            linkchanged = 1;
            break;
            
            case LAN8742_STATUS_10MBITS_HALFDUPLEX:
            duplex = ETH_HALFDUPLEX_MODE;
            speed = ETH_SPEED_10M;
            linkchanged = 1;
            break;
            
            default:
            break;
        }

        if(linkchanged)
        {
            /* Get MAC Config MAC */
            HAL_ETH_GetMACConfig(&heth, &MACConf);
            MACConf.DuplexMode = duplex;
            MACConf.Speed = speed;
            HAL_ETH_SetMACConfig(&heth, &MACConf);
            HAL_ETH_Start_IT(&heth);
            netif_set_up(netif);
            netif_set_link_up(netif);
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



