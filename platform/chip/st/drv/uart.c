#include "drv/uart.h"
#include "drv/delay.h"
#include "log.h"
#include "lock.h"

#if defined(__CC_ARM)
#pragma diag_suppress       1296
#endif


typedef struct {
    U8                      mode;
    U8                      port;
    uart_para_t             para;
    UART_HandleTypeDef      huart;
    DMA_HandleTypeDef       hdmaRx;
    DMA_HandleTypeDef       hdmaTx;
    
    U8                      txFinished;
    handle_t                lock;
}uart_handle_t;

typedef struct {
    USART_TypeDef           *usart;
    IRQn_Type               uartIRQ;
    
    U32                     channel;
    DMA_Stream_TypeDef      *dmaRx;
    DMA_Stream_TypeDef      *dmaTx;
    IRQn_Type               dmaRxIRQ;
    IRQn_Type               dmaTxIRQ;
}uart_map_t;

uart_map_t UART_MAP[UART_MAX]={
    {USART1, USART1_IRQn, DMA_CHANNEL_4, DMA2_Stream2, DMA2_Stream7, DMA2_Stream2_IRQn, DMA2_Stream7_IRQn},          
    {USART2, USART2_IRQn, DMA_CHANNEL_4, DMA1_Stream5, DMA1_Stream6, DMA1_Stream5_IRQn, DMA1_Stream6_IRQn},          
    {USART3, USART3_IRQn, DMA_CHANNEL_4, DMA1_Stream1, DMA1_Stream3, DMA1_Stream1_IRQn, DMA1_Stream3_IRQn},          
    {USART6, USART6_IRQn, DMA_CHANNEL_5, DMA2_Stream1, DMA2_Stream6, DMA2_Stream1_IRQn, DMA2_Stream6_IRQn},
};

uart_handle_t *uart_handle[UART_MAX]={NULL};


static U8 get_uart(UART_HandleTypeDef *huart)
{
    U8 uart;

    switch((U32)huart->Instance) {
        case (U32)USART1:
        {
            uart = UART_1;
        }
        break;

        case (U32)USART2:
        {
            uart = UART_2;
        }
        break;

        case (U32)USART3:
        {
            uart = UART_3;
        }
        break;

        case (U32)USART6:
        {
            uart = UART_6;
        }
        break;
    }
    return uart;
}


void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef init = {0};

    switch((U32)huart->Instance) {
        case (U32)USART1:
        __HAL_RCC_USART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        init.Pin = GPIO_PIN_9|GPIO_PIN_10;
        init.Mode = GPIO_MODE_AF_OD;
        init.Pull = GPIO_NOPULL;
        init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        init.Alternate = GPIO_AF7_USART1;
        HAL_GPIO_Init(GPIOA, &init);
        break;
        
        case (U32)USART2:
        __HAL_RCC_USART2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        init.Pin = GPIO_PIN_2|GPIO_PIN_3;
        init.Mode = GPIO_MODE_AF_PP;
        init.Pull = GPIO_NOPULL;
        init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        init.Alternate = GPIO_AF7_USART2;
        HAL_GPIO_Init(GPIOA, &init);
        break;
        
        case (U32)USART3:
        __HAL_RCC_USART3_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();

        init.Pin = GPIO_PIN_5;
        init.Mode = GPIO_MODE_AF_PP;
        init.Pull = GPIO_NOPULL;
        init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        init.Alternate = GPIO_AF7_USART3;
        HAL_GPIO_Init(GPIOC, &init);

        init.Pin = GPIO_PIN_10;
        init.Mode = GPIO_MODE_AF_PP;
        init.Pull = GPIO_NOPULL;
        init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        init.Alternate = GPIO_AF7_USART3;
        HAL_GPIO_Init(GPIOB, &init);
        break;
        
        case (U32)USART6:
        __HAL_RCC_USART6_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE();

        init.Pin = GPIO_PIN_6|GPIO_PIN_7;
        init.Mode = GPIO_MODE_AF_PP;
        init.Pull = GPIO_NOPULL;
        init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        init.Alternate = GPIO_AF8_USART6;
        HAL_GPIO_Init(GPIOC, &init);
        break;
        
    }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
    switch((U32)huart->Instance) {
        case (U32)USART1:
        __HAL_RCC_USART1_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9|GPIO_PIN_10);
        HAL_DMA_DeInit(huart->hdmarx);
        HAL_DMA_DeInit(huart->hdmatx);
        break;
        
        case (U32)USART2:
        __HAL_RCC_USART2_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2|GPIO_PIN_3);
        break;
        
        case (U32)USART3:
        __HAL_RCC_USART3_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_5);
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10);
        break;
        
        case (U32)USART6:
        __HAL_RCC_USART6_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_6|GPIO_PIN_7);
        break;
        
    }
    HAL_DMA_DeInit(huart->hdmarx);
    HAL_DMA_DeInit(huart->hdmatx);
}
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    U8 uart = get_uart(huart);
    uart_handle_t *h=uart_handle[uart];
    
    h->txFinished = 1;
}

static void HAL_UART_IDLE_CALLBACK(uart_handle_t *h)
{
    U16 data_len,remain;
    uart_map_t *map=&UART_MAP[h->port];
    
    if(__HAL_UART_GET_FLAG(&h->huart, UART_FLAG_IDLE)!=RESET)  {//发生IDLE
        __HAL_UART_CLEAR_IDLEFLAG(&h->huart);
        
        //HAL_NVIC_DisableIRQ(map->dmaRxIRQ);		//如果不关中断、那么__HAL_DMA_DISABLE会触发DMA2_Stream2_IRQHandler
		//__HAL_DMA_DISABLE(h->huart.hdmarx);
        
        if(h->mode==MODE_DMA) {
            HAL_UART_DMAStop(&h->huart);
            remain = __HAL_DMA_GET_COUNTER(h->huart.hdmarx);
        }
        else if(h->mode==MODE_IT) {
            remain = h->huart.RxXferCount;
            HAL_UART_AbortReceive_IT(&h->huart);
        }
        
        data_len = h->huart.RxXferSize - remain;
        if(h->para.rx && data_len) {
            h->para.rx(h->huart.pRxBuffPtr, data_len);
            //h->para.rx(h->para.buf, data_len);
        }
        
        if(h->mode==MODE_DMA) {
            __HAL_DMA_SET_COUNTER(h->huart.hdmarx, h->para.blen);
            HAL_UART_Receive_DMA(&h->huart, h->para.buf, h->para.blen);
        }
        else if(h->mode==MODE_IT) {
            HAL_UART_Receive_IT(&h->huart, h->para.buf, h->para.blen);
        }
    }
}
void uart_handler(int uart)
{
    uart_handle_t *h=uart_handle[uart];
    
    if(!h) return;
    
    HAL_UART_IRQHandler(&h->huart);
    HAL_UART_IDLE_CALLBACK(h);

}

void uart_dma_handler(int uart)
{
    uart_handle_t *h=uart_handle[uart];
    
    if(!h) return;
    
    HAL_DMA_IRQHandler(&h->hdmaRx);
}



static void uart_clk_en(U8 uart, int on)
{
    if(on) {
        if(uart==UART_1 || uart==UART_6) {
            __HAL_RCC_DMA2_CLK_ENABLE();
        }
        else {
            __HAL_RCC_DMA1_CLK_ENABLE();
        }
    }
    else {
        if(uart==UART_1 || uart==UART_6) {
            __HAL_RCC_DMA2_CLK_DISABLE();
        }
        else {
            __HAL_RCC_DMA1_CLK_DISABLE();
        }
    }
}
static int uart_irq_en(uart_handle_t *h, int on)
{
    uart_map_t *map=&UART_MAP[h->port];
    
    if(on) {

        HAL_NVIC_SetPriority(map->uartIRQ, 0, 0);
        HAL_NVIC_SetPriority(map->dmaRxIRQ, 0, 1);
        //HAL_NVIC_SetPriority(map->dmaTxIRQ, 0, 1);
        
        HAL_NVIC_EnableIRQ(map->uartIRQ);
        HAL_NVIC_EnableIRQ(map->dmaRxIRQ);
        //HAL_NVIC_EnableIRQ(map->dmaTxIRQ);
        
        __HAL_UART_ENABLE_IT(&h->huart, UART_IT_IDLE);
    }
    else {
        HAL_NVIC_DisableIRQ(map->dmaRxIRQ);
        //HAL_NVIC_DisableIRQ(map->dmaTxIRQ);
        HAL_NVIC_DisableIRQ(map->uartIRQ);
        
        __HAL_UART_DISABLE_IT(&h->huart, UART_IT_IDLE);
    }
    
    return 0;
}

static int uart_dma_init(uart_handle_t *h)
{
    uart_map_t *map=&UART_MAP[h->port];
    
    h->hdmaRx.Instance = map->dmaRx;
    h->hdmaRx.Init.Channel = map->channel;
    h->hdmaRx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    h->hdmaRx.Init.PeriphInc = DMA_PINC_DISABLE;
    h->hdmaRx.Init.MemInc = DMA_MINC_ENABLE;
    h->hdmaRx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    h->hdmaRx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    h->hdmaRx.Init.Mode = DMA_NORMAL;
    h->hdmaRx.Init.Priority = DMA_PRIORITY_HIGH;
    h->hdmaRx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&h->hdmaRx) != HAL_OK) {
        return -1;
    }
    __HAL_LINKDMA(&h->huart, hdmarx, h->hdmaRx);
    HAL_NVIC_SetPriority(map->dmaRxIRQ, 0, 1);
    HAL_NVIC_EnableIRQ(map->dmaRxIRQ);
    
#if 0
    h->hdmaTx.Instance = map->dmaTx;
    h->hdmaTx.Init.Channel = map->channel;
    h->hdmaTx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    h->hdmaTx.Init.PeriphInc = DMA_PINC_DISABLE;
    h->hdmaTx.Init.MemInc = DMA_MINC_ENABLE;
    h->hdmaTx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    h->hdmaTx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    h->hdmaTx.Init.Mode = DMA_NORMAL;
    h->hdmaTx.Init.Priority = DMA_PRIORITY_HIGH;
    h->hdmaTx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&h->hdmaTx) != HAL_OK) {
        return -1;
    }
    __HAL_LINKDMA(&h->huart, hdmarx, h->hdmaTx);
    HAL_NVIC_SetPriority(map->dmaTxIRQ, 0, 1);
    HAL_NVIC_EnableIRQ(map->dmaTxIRQ);
#endif    
    
    return 0;
}
handle_t uart_init(uart_cfg_t *cfg)
{
    HAL_StatusTypeDef st;
    uart_map_t *map;
    UART_InitTypeDef  init;
    
    uart_handle_t *h=calloc(1, sizeof(uart_handle_t));
    
    if(!h || !cfg) {
        return NULL;
    }

    map = &UART_MAP[cfg->port];
    uart_clk_en(cfg->port, 1);
    
    init.BaudRate   = cfg->baudrate;
    init.StopBits   = UART_STOPBITS_1;
    init.WordLength = UART_WORDLENGTH_8B;
    init.Parity     = UART_PARITY_NONE;
    init.Mode       = UART_MODE_TX_RX;
    init.HwFlowCtl  = UART_HWCONTROL_NONE;
    init.OverSampling = UART_OVERSAMPLING_16;
    
    h->mode  = cfg->mode;
    h->port  = cfg->port;
    h->huart.Instance = map->usart;
    h->huart.Init = init;
    h->para = cfg->para;
    HAL_UART_DeInit(&h->huart);
    st = HAL_UART_Init(&h->huart);
    if(st!=HAL_OK) {
        free(h); return NULL;
    }

    if(h->mode!=MODE_POLL) {
        uart_dma_init(h);
        
        HAL_NVIC_SetPriority(map->uartIRQ, 0, 0);
        HAL_NVIC_EnableIRQ(map->uartIRQ);
        __HAL_UART_ENABLE_IT(&h->huart, UART_IT_IDLE);
    }
    
    if(h->mode==MODE_DMA) {
        HAL_UART_Receive_DMA(&h->huart, h->para.buf, h->para.blen);
    }
    else if(h->mode==MODE_IT) {
        HAL_UART_Receive_IT(&h->huart, h->para.buf, h->para.blen);
    }
    h->lock = lock_dynamic_new();
    
    uart_handle[h->port] = h;
    
    return h;
}


int uart_deinit(handle_t *h)
{
    uart_handle_t **uh=(uart_handle_t**)h;
    
    if(!uh || !(*uh)) {
        return -1;
    }
    
    if((*uh)->mode!=MODE_POLL) {
        uart_irq_en(*uh, 0);
    }
    HAL_UART_DeInit(&(*uh)->huart);
    lock_dynamic_free(&(*uh)->lock);
    free(*uh);
    
    return 0;
}


int uart_read(handle_t h, U8 *data, U32 len)
{
    int r;
    uart_handle_t *uh=(uart_handle_t*)h;
    
    if(!h) {
        return -1;
    }
    
    lock_dynamic_hold(uh->lock);
    if(uh->mode==MODE_POLL) {
        r = HAL_UART_Receive(&uh->huart, data, len, HAL_MAX_DELAY);
    }
    if(uh->mode==MODE_IT) {
        r = HAL_UART_Receive_IT(&uh->huart, data, len);
    }
    else {
        r = HAL_UART_Receive_DMA(&uh->huart, data, len);
    }
    lock_dynamic_release(uh->lock);
    
    return r;
}

static void uart_wait(uart_handle_t *h)
{
    while(h->txFinished==0) delay_ms(1);
}
int uart_write(handle_t h, U8 *data, U32 len)
{
    int r;
    uart_handle_t *uh=(uart_handle_t*)h;
    
    if(!h) {
        return -1;
    }

    lock_dynamic_hold(uh->lock);
#if 0
    if(uh->mode==MODE_DMA) {
        uh->txFinished = 0;
        r = HAL_UART_Transmit_DMA(&uh->huart, data, len);
        uart_wait(uh);
    }
    if(uh->mode==MODE_IT) {
        uh->txFinished = 0;
        r = HAL_UART_Transmit_IT(&uh->huart, data, len);
        uart_wait(uh);
    }
    else 
#endif
    {
        r = HAL_UART_Transmit(&uh->huart, data, len, HAL_MAX_DELAY);
    }
    lock_dynamic_release(uh->lock);
    
    return r;
}


int uart_rw(handle_t h, U8 *data, U32 len, U8 rw)
{
    int r;
    uart_handle_t *uh=(uart_handle_t*)h;
   
    if(!h) {
        return -1;
    }
    
    lock_dynamic_hold(uh->lock);
    if(rw) {
        //r = HAL_UART_Transmit(&uh->huart, data, len, HAL_MAX_DELAY);
        r = uart_write(h, data, len);
    }
    else {
        r = uart_read(h, data, len);
    }
    lock_dynamic_release(uh->lock);
    
    return r;
}




