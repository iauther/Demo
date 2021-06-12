#include "drv/jump.h"
#include "drv/uart.h"
#include "drv/delay.h"
#include "board.h"
#include "com.h"
#include "cfg.h"

#define TIMER_MS            100
#define RD_BUFLEN           1000


static U16 data_recved_len=0;
static U8  readBuffer[RD_BUFLEN];
static void rx_callback(U8 *data, U16 len)
{
    
    if(len<=RD_BUFLEN) {
        memcpy(readBuffer, data, len);
        data_recved_len = len;
    }
}


int main(void)
{
    board_init();
    //timer_init();
    
    com_init(rx_callback, TIMER_MS);
    //if(!upgrade_is_need()) {
    if(1) {
        board_deinit();
        com_deinit();
        
        jump_to_app();
    }
    
    while(1) {
        if(data_recved_len>0) {
            com_data_proc(readBuffer, data_recved_len);
            data_recved_len = 0;
        }
    }
}


