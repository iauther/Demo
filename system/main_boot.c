#include "drv/jump.h"
#include "drv/uart.h"
#include "drv/delay.h"
#include "drv/htimer.h"
#include "board.h"
#include "com.h"
#include "pkt.h"
#include "paras.h"
#include "notice.h"
#include "upgrade.h"
#include "power.h"
#include "wdog.h"
#include "myCfg.h"


static U16 data_recved_len=0;
static handle_t tmr_handle=NULL;

static U8  readBuffer[PKT_BUFLEN];
static void rx_callback(U8 *data, U16 len)
{
    
    if(len<=PKT_BUFLEN) {
        memcpy(readBuffer, data, len);
        data_recved_len = len;
    }
}
static void tmr_callback(void *arg)
{
    
}
////////////////////////////////////////////////
static void start_timer(void)
{
    htimer_set_t set;
    
    tmr_handle = htimer_new();
    if(tmr_handle) {
        set.ms = ACK_POLL_MS;
        set.freq = 0;
        set.repeat = 1;
        set.callback = tmr_callback;
        htimer_set(tmr_handle, &set);
        htimer_start(tmr_handle);
    }
}


int main(void)
{
    //SCB->VTOR = SRAM1_BASE;
    //__enable_irq();
    
    board_init();
    if(!upgrade_is_need()) {
    //if(0) {
        board_deinit();
        jump_to_app();
    }
    
    led_set_color(BLUE);
    com_init(rx_callback, ACK_POLL_MS);
    
    while(1) {
        if(data_recved_len>0) {
            com_data_proc(readBuffer, data_recved_len);
            data_recved_len = 0;
        }
        
        wdog_feed();
    }
}


