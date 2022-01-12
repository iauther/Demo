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
    
    DMA_Channel_TypeDef     *dmaRxCh;
    IRQn_Type               dmaRxIRQ;
    
    DMA_Channel_TypeDef     *dmaTxCh;
    IRQn_Type               dmaTxIRQ;
}uart_map_t;


uart_map_t UART_MAP[UART_MAX]={
    {USART1, USART1_IRQn, DMA1_Channel5, DMA1_Channel5_IRQn, DMA1_Channel4, DMA1_Channel4_IRQn},          
    {USART2, USART2_IRQn, DMA1_Channel6, DMA1_Channel6_IRQn, DMA1_Channel7, DMA1_Channel7_IRQn},          
    {USART3, USART3_IRQn, DMA1_Channel3, DMA1_Channel3_IRQn, DMA1_Channel3, DMA1_Channel3_IRQn},          
    {UART4,  UART4_IRQn,  DMA2_Channel3, DMA2_Channel3_IRQn, DMA2_Channel5, DMA2_Channel4_5_IRQn},
};

uart_handle_t *uart_handle[UART_MAX]={NULL};
static int uart_dma_init(uart_handle_t *h);
static int uart_irq_en(uart_handle_t *h, int on);


static void HAL_UART_IDLE_CALLBACK(uart_handle_t *h)
{
    U32 st = 0;
    U16 data_len,remain;
    uart_map_t *map=&UART_MAP[h->port];
    
    if(__HAL_UART_GET_FLAG(&h->huart, UART_FLAG_IDLE)!=RESET)  {//发生IDLE
        __HAL_UART_CLEAR_IDLEFLAG(&h->huart);
        
//st = h->huart.Instance->SR;
//st = h->huart.Instance->DR;
        //HAL_NVIC_DisableIRQ(map->dmaRxIRQ);		//如果不关中断、那么__HAL_DMA_DISABLE会触发DMA2_Stream2_IRQHandler
		//
        
        if(h->mode==MODE_DMA) {
            HAL_UART_DMAStop(&h->huart);
            //__HAL_DMA_DISABLE(h->huart.hdmarx);
            
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
            //__HAL_DMA_SET_COUNTER(h->huart.hdmarx, h->para.blen);
            HAL_UART_Receive_DMA(&h->huart, h->para.buf, h->para.blen);
            //__HAL_DMA_ENABLE(h->huart.hdmarx);
        }
        else if(h->mode==MODE_IT) {
            HAL_UART_Receive_IT(&h->huart, h->para.buf, h->para.blen);
        }
    }
}

static void uart_handler(int uart)
{
    uart_handle_t *h=uart_handle[uart];
    
    if(!h) return;
    
    HAL_UART_IRQHandler(&h->huart);
    HAL_UART_IDLE_CALLBACK(h);

}

enum{RX=0, TX};
static void uart_dma_handler(char rxtx, int uart)
{
    uart_handle_t *h=uart_handle[uart];
    
    if(!h) return;
    
    if(rxtx==RX) {
        HAL_DMA_IRQHandler(&h->hdmaRx);
    }
    else if(rxtx==TX){
        HAL_DMA_IRQHandler(&h->hdmaTx);
    }
}

   
void USART1_IRQHandler(void){uart_handler(UART_1);}
void USART2_IRQHandler(void){uart_handler(UART_2);}
void USART3_IRQHandler(void){uart_handler(UART_3);}
void UART4_IRQHandler(void) {uart_handler(UART_4);}
void DMA1_Channel2_IRQHandler(void){uart_dma_handler(TX,   UART_3);}
void DMA1_Channel3_IRQHandler(void){uart_dma_handler(RX,   UART_3);}
void DMA1_Channel4_IRQHandler(void){uart_dma_handler(TX,   UART_1);}
void DMA1_Channel5_IRQHandler(void){uart_dma_handler(RX,   UART_1);}
void DMA1_Channel6_IRQHandler(void){uart_dma_handler(RX,   UART_2);}
void DMA1_Channel7_IRQHandler(void){uart_dma_handler(TX,   UART_2);}
void DMA2_Channel3_IRQHandler(void){uart_dma_handler(RX,   UART_4);}
void DMA2_Channel4_5_IRQHandler(void){uart_dma_handler(TX, UART_4);}


//////////////////////////////////////////////////////////

static U8 get_uart(UART_HandleTypeDef *huart)
{
    U8 uart;

    switch((U32)huart->Instance) {
        case (U32)USART1: uart = UART_1; break;
        case (U32)USART2: uart = UART_2; break;
        case (U32)USART3: uart = UART_3; break;
        case (U32)UART4:  uart = UART_4; break;
    }
    return uart;
}


void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    U8 port=0;
    GPIO_InitTypeDef init = {0};

    
    init.Pull = GPIO_NOPULL;
    init.Speed = GPIO_SPEED_FREQ_HIGH;
    switch((U32)huart->Instance) {
        case (U32)USART1:
        port = UART_1;
        __HAL_RCC_USART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        init.Pin = GPIO_PIN_9;
        init.Mode = GPIO_MODE_AF_PP;
        HAL_GPIO_Init(GPIOA, &init);
        
        init.Pin = GPIO_PIN_10;
        init.Mode = GPIO_MODE_INPUT;
        HAL_GPIO_Init(GPIOA, &init);
        break;
        
        case (U32)USART2:
        port = UART_2;
        __HAL_RCC_USART2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        init.Pin = GPIO_PIN_2;
        init.Mode = GPIO_MODE_AF_PP;
        HAL_GPIO_Init(GPIOA, &init);
        
        init.Pin = GPIO_PIN_3;
        init.Mode = GPIO_MODE_INPUT;
        HAL_GPIO_Init(GPIOA, &init);
        break;
        
        case (U32)USART3:
        port = UART_3;
        __HAL_RCC_USART3_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();

        init.Pin = GPIO_PIN_10;
        init.Mode = GPIO_MODE_AF_PP;
        HAL_GPIO_Init(GPIOB, &init);
        
        init.Pin = GPIO_PIN_11;
        init.Mode = GPIO_MODE_INPUT;
        HAL_GPIO_Init(GPIOB, &init);
        break;
        
        case (U32)UART4:
        port = UART_4;
        __HAL_RCC_UART4_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE();

        init.Pin = GPIO_PIN_10;
        init.Mode = GPIO_MODE_AF_PP;
        HAL_GPIO_Init(GPIOC, &init);
        
        init.Pin = GPIO_PIN_11;
        init.Mode = GPIO_MODE_INPUT;
        HAL_GPIO_Init(GPIOC, &init);
        break;     
        
        default:
        return;
    }
    
    uart_dma_init(uart_handle[port]);
    uart_irq_en(uart_handle[port], 1);
}

void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
    U8 port=0;
    
    switch((U32)huart->Instance) {
        case (U32)USART1:
        port = UART_1;
        __HAL_RCC_USART1_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9|GPIO_PIN_10);
        break;
        
        case (U32)USART2:
        port = UART_2;
        __HAL_RCC_USART2_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2|GPIO_PIN_3);
        break;
        
        case (U32)USART3:
        port = UART_3;
        __HAL_RCC_USART3_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_5);
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10);
        break;
        
        case (U32)UART4:
        port = UART_4;
        __HAL_RCC_UART4_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_6|GPIO_PIN_7);
        break;
        
        default:
        return;
    }
    
    if(uart_handle[port]->mode!=MODE_POLL) {
        HAL_DMA_DeInit(huart->hdmarx);
        HAL_DMA_DeInit(huart->hdmatx);
        uart_irq_en(uart_handle[port], 0);
    }
}


static void dma_irq_en(U8 port, int on)
{
    uart_map_t *map=&UART_MAP[port];
    
    if(on) {
        if(port==UART_4) {
            __HAL_RCC_DMA2_CLK_ENABLE();
        }
        else {
            __HAL_RCC_DMA1_CLK_ENABLE();
        }
        
        HAL_NVIC_SetPriority(map->dmaRxIRQ, 5, 0);
        HAL_NVIC_SetPriority(map->dmaTxIRQ, 5, 0);
        
        HAL_NVIC_EnableIRQ(map->dmaRxIRQ);
        HAL_NVIC_EnableIRQ(map->dmaTxIRQ);
    }
    else {
        if(port==UART_4) {
            __HAL_RCC_DMA2_CLK_DISABLE();
        }
        else {
            __HAL_RCC_DMA1_CLK_DISABLE();
        }
        
        HAL_NVIC_DisableIRQ(map->dmaRxIRQ);
        HAL_NVIC_DisableIRQ(map->dmaTxIRQ);
    }
}
static int uart_irq_en(uart_handle_t *h, int on)
{
    uart_map_t *map=&UART_MAP[h->port];
    
    if(on) {
        __HAL_UART_ENABLE_IT(&h->huart, UART_IT_IDLE);
        HAL_NVIC_EnableIRQ(map->uartIRQ);
    }
    else {
        //__HAL_UART_DISABLE_IT(&h->huart, UART_IT_IDLE);
        HAL_NVIC_DisableIRQ(map->uartIRQ);
    }
    
    return 0;
}

static int uart_dma_init(uart_handle_t *h)
{
    uart_map_t *map=&UART_MAP[h->port];
    
    if(h->mode==MODE_POLL) {
        return -1;
    }
    
    dma_irq_en(h->port, 1);
    
    h->hdmaRx.Instance = map->dmaRxCh;
    h->hdmaRx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    h->hdmaRx.Init.PeriphInc = DMA_PINC_DISABLE;
    h->hdmaRx.Init.MemInc = DMA_MINC_ENABLE;
    h->hdmaRx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    h->hdmaRx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    h->hdmaRx.Init.Mode = DMA_NORMAL;
    h->hdmaRx.Init.Priority = DMA_PRIORITY_HIGH;
    h->hdmaRx.Init.Mode = DMA_CIRCULAR;   //DMA_NORMAL
    if (HAL_DMA_Init(&h->hdmaRx) != HAL_OK) {
        return -1;
    }
    __HAL_LINKDMA(&h->huart, hdmarx, h->hdmaRx);
    
#if 1
    h->hdmaTx.Instance = map->dmaTxCh;
    h->hdmaTx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    h->hdmaTx.Init.PeriphInc = DMA_PINC_DISABLE;
    h->hdmaTx.Init.MemInc = DMA_MINC_ENABLE;
    h->hdmaTx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    h->hdmaTx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    h->hdmaTx.Init.Mode = DMA_NORMAL;
    h->hdmaTx.Init.Priority = DMA_PRIORITY_LOW;
    h->hdmaTx.Init.Mode = DMA_CIRCULAR;   //DMA_NORMAL
    if (HAL_DMA_Init(&h->hdmaTx) != HAL_OK) {
        return -1;
    }
    __HAL_LINKDMA(&h->huart, hdmatx, h->hdmaTx);
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
    
    uart_handle[h->port] = h;
    
    HAL_UART_DeInit(&h->huart);
    st = HAL_UART_Init(&h->huart);
    if(st!=HAL_OK) {
        free(h); return NULL;
    }

    if(h->mode==MODE_DMA) {
        HAL_UART_Receive_DMA(&h->huart, h->para.buf, h->para.blen);
    }
    else if(h->mode==MODE_IT) {
        HAL_UART_Receive_IT(&h->huart, h->para.buf, h->para.blen);
    }
    h->lock = lock_dynamic_new();

    
    return h;
}


int uart_deinit(handle_t *h)
{
    uart_handle_t **uh=(uart_handle_t**)h;
    
    if(!uh || !(*uh)) {
        return -1;
    }
    
    HAL_UART_DeInit(&(*uh)->huart);
    lock_dynamic_free(&(*uh)->lock);
    free(*uh);
    
    return 0;
}


static void uart_reset(uart_handle_t *h)
{
    HAL_UART_DeInit(&h->huart);
    HAL_UART_Init(&h->huart);
}

int uart_read(handle_t h, U8 *data, U32 len)
{
    int r;
    uart_handle_t *uh=(uart_handle_t*)h;
    
    if(!h || !data || !len) {
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
    
    if(r!=HAL_OK) {
        uart_reset(uh);
    }
    
    lock_dynamic_release(uh->lock);
    
    return r;
}


void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    U8 uart = get_uart(huart);
    uart_handle_t *h=uart_handle[uart];
    
    h->txFinished = 1;
}
static void uart_tx_wait(uart_handle_t *h, int ms)
{
    int i=0;
    while(h->txFinished==0) {
        if(i++<ms) {
            delay_ms(1);
        }
    }
}
int uart_write(handle_t h, U8 *data, U32 len)
{
    int r;
    uart_handle_t *uh=(uart_handle_t*)h;
    
    if(!h || !data || !len) {
        return -1;
    }

    lock_dynamic_hold(uh->lock);
#if 0
    if(uh->mode==MODE_DMA) {
        uh->txFinished = 0;
        r = HAL_UART_Transmit_DMA(&uh->huart, data, len);
        uart_tx_wait(uh, 5);
    }
    else if(uh->mode==MODE_IT) {
        uh->txFinished = 0;
        r = HAL_UART_Transmit_IT(&uh->huart, data, len);
        uart_tx_wait(uh, 5);
    }
    else
#endif
    {
        r = HAL_UART_Transmit(&uh->huart, data, len, HAL_MAX_DELAY);
    }
    
    if(r!=HAL_OK) {
        uart_reset(uh);
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




