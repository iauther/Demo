#include "task.h"
#include "drv/adc.h"
#include "drv/uart.h"
#include "drv/delay.h"
#include "drv/htimer.h"
#include "eth.h"
#include "paras.h"
#include "com.h"
#include "pkt.h"
#include "cfg.h"


static U16 data_recved_len=0;
static handle_t tmr_handle=NULL;

static U8  rxBuffer[PKT_BUFLEN];
static U8  tmpBuffer[PKT_BUFLEN];
static handle_t urtHandle;
static void rx_callback(U8 *data, U16 len)
{
    
    if(len<=PKT_BUFLEN) {
        memcpy(rxBuffer, data, len);
        data_recved_len = len;
    }
}
static void tmr_callback(void *arg)
{
    
}
////////////////////////////////////////////////

static int urt_init(void)
{
    uart_cfg_t uc;

    uc.mode = MODE_DMA;
    uc.port = UART_3;
    uc.baudrate = 115200;
    uc.para.rx  = rx_callback;
    uc.para.buf = rxBuffer;
    uc.para.blen = sizeof(rxBuffer);
    uc.para.dlen = 0;
    urtHandle = uart_init(&uc);
    
    return 0;
}






void test_main(void)
{   
    //urt_init();
    //adc_init();
    //com_init(rx_callback, 100);
    
    while(1) {
        if(data_recved_len>0) {
            //com_data_proc(readBuffer, data_recved_len);
            data_recved_len = 0;
        }
        
        delay_ms(300);
    }
}

