#include "dal_uart.h"
#include "dal_gpio.h"
#include "rs485.h"

#define RX_BUF_LEN  1000
typedef struct {
    handle_t  hurt;
    handle_t  hde;
    
    int       rxLen;
    
    rs485_cfg_t cfg;
}rs485_handle_t;

static rs485_handle_t* rs485Handle=NULL;


static int rs485_recv_callback(handle_t h, void *addr, U32 evt, void *data, int len)
{
    rs485_handle_t *rh=rs485Handle;
    
    
    if(rh) {
        rh->rxLen = len;
        if(rh->cfg.callback) {
            rh->cfg.callback(h, addr, evt, data, len);
        }
    }
    
    return 0;
}


int rs485_init(rs485_cfg_t *cfg)
{
    dal_uart_cfg_t ucfg;
    dal_gpio_cfg_t gcfg;
    rs485_handle_t *h=NULL;
    
    if(!cfg) {
        return -1;
    }
    
    h = malloc(sizeof(rs485_handle_t));
    if(!h) {
        return -1;
    }
    h->cfg = *cfg;
    
    ucfg.baudrate = cfg->baudrate;
    ucfg.mode = MODE_IT;
    ucfg.port = cfg->port;
    ucfg.msb  = 0;
    ucfg.para.callback = rs485_recv_callback;
    h->hurt = dal_uart_init(&ucfg);
    
    gcfg.gpio.grp = GPIOB;
    gcfg.gpio.pin = GPIO_PIN_0;
    gcfg.mode = MODE_OUT_PP;
    gcfg.pull = PULL_NONE;
    h->hde = dal_gpio_init(&gcfg);
    dal_gpio_set_hl(h->hde, 0);         //recv mode
    
    h->rxLen = 0;
    rs485Handle = h;
    
    return 0;
}


int rs485_deinit(void)
{
    rs485_handle_t *h=rs485Handle;
    
    if(!h) {
        return -1;
    }
    
    dal_gpio_deinit(h->hde);
    dal_uart_deinit(h->hurt);
    free(h);
    rs485Handle = NULL;
    
    return 0;
}


int rs485_write(U8 *data, int len)
{
    rs485_handle_t *h=rs485Handle;
    if(!h) {
        return -1;
    }
    
    dal_gpio_set_hl(h->hde, 1);
    dal_uart_write(h->hurt, data, len);
    dal_gpio_set_hl(h->hde, 0);
    
    return 0;
}


