#include "tcp232.h"
#include "drv/uart.h"
#include "drv/gpio.h"
#include "task.h"
#include "myCfg.h"

#define TCP232_BUF_LEN          1000
#define TCP232_PORT             UART_1
#define TCP232_BAUDRATE         115200


static handle_t urtHandle=NULL;
static handle_t cfgHandle=NULL;
static handle_t rstHandle=NULL;

static U8 tcp232_buf[TCP232_BUF_LEN];

static void io_init(void)
{
    gpio_cfg_t cfg;
    gpio_pin_t rstPin=TCP232_RST_PIN;
    gpio_pin_t cfgPin=TCP232_CFG_PIN;
    
    cfg.mode = MODE_OUTPUT;
    
    cfg.pin = rstPin;
    rstHandle = gpio_init(&cfg);
    gpio_set_hl(rstHandle, 1);
    
    cfg.pin = cfgPin;
    cfgHandle = gpio_init(&cfg);
    gpio_set_hl(cfgHandle, 1);
}


static void port_init(tcp232_rx_t fn)
{
    uart_cfg_t cfg;
    
    cfg.baudrate = TCP232_BAUDRATE;
    cfg.mode = MODE_DMA;
    cfg.para.buf = tcp232_buf;
    cfg.para.blen = sizeof(tcp232_buf);
    cfg.para.dlen = 0;
    cfg.para.rx = fn;
    
    cfg.port = TCP232_PORT;
    cfg.useDMA = 0;
    urtHandle = uart_init(&cfg);
}





int tcp232_init(tcp232_rx_t fn)
{
    io_init();
    port_init(fn);
    
    return 0;
}


int tcp232_reset(void)
{
    gpio_set_hl(rstHandle, 0);
    delay_ms(200);
    gpio_set_hl(rstHandle, 1);
    
    return 0;
}


    
int tcp232_write(U8 *data, U32 len)
{
    return uart_write(urtHandle, data, len);
}

