#include "incs.h"
#include "lock.h"

#if defined(__CC_ARM)
#pragma diag_suppress       1296
#endif


#define BSP_USART_TX_RCU        RCU_GPIOA
#define BSP_USART_RX_RCU        RCU_GPIOA
#define BSP_USART_RCU           RCU_USART0

#define BSP_USART_TX_PORT       GPIOA
#define BSP_USART_RX_PORT       GPIOA
#define BSP_USART_AF 			GPIO_AF_7
#define BSP_USART_TX_PIN  G     PIO_PIN_9
#define BSP_USART_RX_PIN        GPIO_PIN_10

#define BSP_USART 				USART0

handle_t uart_init(uart_cfg_t *cfg)
{
    HAL_StatusTypeDef st;
    uart_map_t *map;
    UART_InitTypeDef  init;
    
    uart_handle_t *h=calloc(1, sizeof(uart_handle_t));
    
    if(!h || !cfg) {
        return NULL;
    }

    /* 开启时钟 */
    rcu_periph_clock_enable(BSP_USART_TX_RCU); 
    rcu_periph_clock_enable(BSP_USART_RX_RCU); 
    rcu_periph_clock_enable(BSP_USART_RCU); 

    /* 配置GPIO复用功能 */
    gpio_af_set(BSP_USART_TX_PORT,BSP_USART_AF,BSP_USART_TX_PIN);	
    gpio_af_set(BSP_USART_RX_PORT,BSP_USART_AF,BSP_USART_RX_PIN);	

    /* 配置GPIO的模式 */
    /* 配置TX为复用模式 上拉模式 */
    gpio_mode_set(BSP_USART_TX_PORT,GPIO_MODE_AF,GPIO_PUPD_PULLUP,BSP_USART_TX_PIN);
    /* 配置RX为复用模式 上拉模式 */
    gpio_mode_set(BSP_USART_RX_PORT, GPIO_MODE_AF,GPIO_PUPD_PULLUP,BSP_USART_RX_PIN);

    /* 配置TX为推挽输出 50MHZ */
    gpio_output_options_set(BSP_USART_TX_PORT,GPIO_OTYPE_PP,GPIO_OSPEED_50MHZ,BSP_USART_TX_PIN);
    /* 配置RX为推挽输出 50MHZ */
    gpio_output_options_set(BSP_USART_RX_PORT,GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, BSP_USART_RX_PIN);

    /* 配置串口的参数 */
    usart_deinit(BSP_USART);
    usart_baudrate_set(BSP_USART,band_rate);
    usart_parity_config(BSP_USART,USART_PM_NONE);
    usart_word_length_set(BSP_USART,USART_WL_8BIT);
    usart_stop_bit_set(BSP_USART,USART_STB_1BIT);

    /* 使能串口 */
    usart_enable(BSP_USART);
    usart_transmit_config(BSP_USART,USART_TRANSMIT_ENABLE);		
		
		
    h->lock = lock_dynamic_new();
    
    return h;
}


int uart_deinit(handle_t h)
{
    uart_handle_t *uh=(uart_handle_t*)h;
    
    if(!uh) {
        return -1;
    }
    
    
    lock_dynamic_free(uh->lock);
    free(uh);
    
    return 0;
}



int uart_read(handle_t h, U8 *data, U32 len)
{
    int r;
    uart_handle_t *uh=(uart_handle_t*)h;
    
    if(!h || !data || !len) {
        return -1;
    }
    
    lock_dynamic_hold(uh->lock);
    
    
    lock_dynamic_release(uh->lock);
    
    return r;
}


int uart_write(handle_t h, U8 *data, U32 len)
{
    int r;
    uart_handle_t *uh=(uart_handle_t*)h;
    
    if(!h || !data || !len) {
        return -1;
    }

    lock_dynamic_hold(uh->lock);

	usart_data_transmit(BSP_USART,(uint8_t)ucch);
	while(RESET == usart_flag_get(BSP_USART,USART_FLAG_TBE));
    
    
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
        r = HAL_UART_Transmit(&uh->huart, data, len, HAL_MAX_DELAY);
    }
    else {
        r = HAL_UART_Receive(&uh->huart, data, len, HAL_MAX_DELAY);
        
    }
    lock_dynamic_release(uh->lock);
    
    return r;
}




