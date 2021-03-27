#include "drv/uart.h"
#include "memory_attribute.h"


typedef struct {
    U8                  port;
    U8                  useDMA;
    S8                  event;        //0: free   1: read    2:write   -1: error
    
    U8                  r_busying;
    U8                  w_busying;
}uart_handle_t;


#define VFIFO_SIZE (256)
#define SEND_THRESHOLD_SIZE (50)
#define RECEIVE_THRESHOLD_SIZE (150)
#define RECEIVE_ALERT_SIZE (30)

ATTR_ZIDATA_IN_NONCACHED_RAM_4BYTE_ALIGN static uint8_t g_tx_buffer[VFIFO_SIZE];
ATTR_ZIDATA_IN_NONCACHED_RAM_4BYTE_ALIGN static uint8_t g_rx_buffer[VFIFO_SIZE];


static uart_cfg_t uart_cfg[HAL_UART_MAX];
static void uart_callback_fn(hal_uart_callback_event_t event, void *user_data)
{
    uart_cfg_t *cfg=(uart_cfg_t*)user_data;
    uart_handle_t *h=(uart_handle_t*)user_data;
    
    if(!cfg) return;
    
    h->event = (U8)event;
}


static hal_uart_status_t uart_dma_init(hal_uart_port_t port)
{
    hal_uart_status_t st;
    hal_uart_dma_config_t dma_config;
    
    dma_config.receive_vfifo_alert_size = RECEIVE_ALERT_SIZE;
    dma_config.receive_vfifo_buffer = g_rx_buffer;
    dma_config.receive_vfifo_buffer_size = VFIFO_SIZE;
    dma_config.receive_vfifo_threshold_size = RECEIVE_THRESHOLD_SIZE;
    dma_config.send_vfifo_buffer = g_tx_buffer;
    dma_config.send_vfifo_buffer_size = VFIFO_SIZE;
    dma_config.send_vfifo_threshold_size = SEND_THRESHOLD_SIZE;
    
    st = hal_uart_set_dma(port, &dma_config);
    st |= hal_uart_register_callback(port, uart_callback_fn, &uart_cfg[port]);
    
    return st;
}


handle_t uart_init(uart_cfg_t *cfg)
{
    hal_uart_status_t st;
    hal_uart_config_t ucfg;
    uart_handle_t *h=(uart_handle_t*)calloc(1, sizeof(uart_handle_t));
    
    if(!cfg || !h) {
        return NULL;
    }
    
    switch(cfg->port) {
        case HAL_UART_0:
        hal_gpio_init(HAL_GPIO_16);
    	hal_gpio_init(HAL_GPIO_17);
    	hal_pinmux_set_function(HAL_GPIO_16, HAL_GPIO_16_U0RXD);
    	hal_pinmux_set_function(HAL_GPIO_17, HAL_GPIO_17_U0TXD);
        break;
        
        case HAL_UART_1:
        hal_gpio_init(HAL_GPIO_4);
        hal_gpio_init(HAL_GPIO_5);
        hal_pinmux_set_function(HAL_GPIO_4, HAL_GPIO_4_U1RXD);
        hal_pinmux_set_function(HAL_GPIO_5, HAL_GPIO_5_U1TXD);
        break;
        
        case HAL_UART_2:
        hal_gpio_init(HAL_GPIO_0);
    	hal_gpio_init(HAL_GPIO_1);
        hal_pinmux_set_function(HAL_GPIO_0, HAL_GPIO_0_U2RXD);
        hal_pinmux_set_function(HAL_GPIO_1, HAL_GPIO_1_U2TXD);
        break;
        
        case HAL_UART_3:
        hal_gpio_init(HAL_GPIO_2);
    	hal_gpio_init(HAL_GPIO_3);
        hal_pinmux_set_function(HAL_GPIO_2, HAL_GPIO_2_U3RXD);
    	hal_pinmux_set_function(HAL_GPIO_3, HAL_GPIO_3_U3TXD);
        break;
        
        default:
        free(h);
        return NULL;
    }

    ucfg.baudrate = cfg->baudrate;//HAL_UART_BAUDRATE_115200;
	ucfg.word_length = HAL_UART_WORD_LENGTH_8;
	ucfg.stop_bit = HAL_UART_STOP_BIT_1;
	ucfg.parity   = HAL_UART_PARITY_NONE;
	st = hal_uart_init(cfg->port, &ucfg);
    
    if(cfg->useDMA) {
        st |= uart_dma_init(cfg->port);
    }
    
    h->port = cfg->port;
    h->event = 0;
    h->useDMA = cfg->useDMA;
    h->r_busying = 0;
    h->w_busying = 0;

    return h;
}



int uart_deinit(handle_t *h)  
{
    uart_handle_t *uh;
    
    if(!h) {
        return -1;
    }
    
    uh = *((uart_handle_t**)h);
    if(!uh) {
        return -1;
    }
    
    hal_uart_deinit(uh->port);
    free(uh);
    return 0;
}



static U32 dma_read(uart_handle_t *h, U8 *data, U32 len)
{
    U32 i,rl,tlen=0;
    
    while(1) {
        if(h->event==HAL_UART_EVENT_READY_TO_READ) {
            rl = hal_uart_get_available_receive_bytes(h->port);
            if(rl<len) {
                rl = len;
            }
            hal_uart_send_dma(h->port, data+tlen, rl);
            h->event = 0;
            tlen += rl;
            if(tlen>=len) {
                break;
            }
        }
        else if(h->event==HAL_UART_EVENT_TRANSACTION_ERROR) {
            h->event = 0;
            break;
        }
    }
    
    return tlen;
}
int uart_read(handle_t h, U8 *data, U32 len)  
{
    U32 i,length=0;
    uart_handle_t *uh=(uart_handle_t*)h;
    
    if(!h || !data || !len) {
        return -1;
    }
    
    if(uh->r_busying) return -1;
    
    uh->r_busying = 1;
    if(uh->useDMA) {
        length = dma_read(uh, data, len);
    }
    else {
        length = hal_uart_receive_polling(uh->port, data, len);
    }
    uh->r_busying = 0;
    
    return length;
}



static U32 dma_write(uart_handle_t *h, U8 *data, U32 len)
{
    U32 i,wl,tlen=0;
    
    while(1) {
        if(h->event==HAL_UART_EVENT_READY_TO_WRITE) {
            wl = hal_uart_get_available_send_space(h->port);
            if(wl>len) {
                wl = len;
            }
            hal_uart_send_dma(h->port, data+tlen, wl);
            h->event = 0;
            tlen += wl;
            if(tlen>=len) {
                break;
            }
        }
        else if(h->event==HAL_UART_EVENT_TRANSACTION_ERROR) {
            h->event = 0;
            break;
        }
    }
    
    return tlen;
}

int uart_write(handle_t h, U8 *data, U32 len)  
{
    U32 length=0;
    uart_handle_t *uh=(uart_handle_t*)h;
    
    if(!h || !data || !len) {
        return -1;
    }
    
    if(uh->w_busying) return -1;
    
    uh->w_busying = 1;
    if(uh->useDMA) {
        length = dma_write(uh, data, len);
    }
    else {
        length = hal_uart_send_polling(uh->port, data, len);
    }
    uh->w_busying = 0;
        
    return length;
}


int uart_put_char(handle_t h, char c)
{
    uart_handle_t *uh=(uart_handle_t*)h;
    
    if(!h) {
        return -1;
    }
    
    hal_uart_put_char(uh->port, c);
    return 0;
}


int uart_get_char(handle_t h, char *c, U8 block)
{
    U32 x;
    uart_handle_t *uh=(uart_handle_t*)h;
    
    if(!h) {
        return -1;
    }
    
    if(block) {
        if(c) *c = hal_uart_get_char(uh->port);
    }
    else {
        x = hal_uart_get_char_unblocking(uh->port);
        if(x==0xffffffff) {
            return -1;
        }
        
        if(c) *c = x&0xff;
    }
    
    return 0;
}


int uart_is_read_busy(handle_t h)
{
    uart_handle_t *uh=(uart_handle_t*)h;
    
    if(!h) {
        return -1;
    }
    
    return uh->r_busying;
}


int uart_is_write_busy(handle_t h)
{
    uart_handle_t *uh=(uart_handle_t*)h;
    
    if(!h) {
        return -1;
    }
    
    return uh->w_busying;
}


int uart_rw(handle_t h, U8 *data, U32 len, U8 rw)
{
    int r;
    
    if(rw) {
        r = uart_write(h, data, len);
    }
    else {
        r = uart_read(h, data, len);
    }
    
    return r;
}
