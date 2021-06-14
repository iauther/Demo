#include "drv/jump.h"
#include "drv/uart.h"
#include "drv/delay.h"
#include "board.h"
#include "com.h"
#include "myCfg.h"

#define TIMER_MS            100
#define RD_BUFLEN           1000


static U16 data_recved_len=0;
static U8  readBuffer[RD_BUFLEN];
static void com_rx_callback(U8 *data, U16 len)
{
    if(len<=RD_BUFLEN) {
        memcpy(readBuffer, data, len);
        data_recved_len = len;
    }
}


int main(void)
{
    U32 tick;
    board_init();
    //htimer_init(tim_callback, TIMER_MS, 1);
    
    com_init(com_rx_callback, TIMER_MS);
    //if(!upgrade_is_need()) {
    if(1) {
        board_deinit();
        com_deinit();
        
        jump_to_app();
    }
    
    tick = HAL_GetTick();
    while(1) {
        if(data_recved_len>0) {
            com_data_proc(readBuffer, data_recved_len);
            data_recved_len = 0;
        }

        if(HAL_GetTick()%100==0) {
            //
        }

    }
}


