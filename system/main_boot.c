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
#include "myCfg.h"


static U16 data_recved_len=0;
static int tmr_id=0;

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
    
    tmr_id = htimer_new();
    if(tmr_id>=0) {
        set.ms = ACK_POLL_MS;
        set.freq = 0;
        set.repeat = 1;
        set.callback = tmr_callback;
        htimer_set(tmr_id, &set);
        htimer_start(tmr_id);
    }
}

static void send_upgrade_stat(void)
{
    stat_t stat={0};
    
    stat.sysState = STAT_UPGRADE;
    com_send_data(TYPE_STAT, 0, &stat, sizeof(stat));
}


int main(void)
{
    
    
    board_init();
    if(!upgrade_is_need()) {
    //if(1) {
        board_deinit();
        jump_to_app();
    }
    
    com_init(rx_callback, ACK_POLL_MS);
    send_upgrade_stat();
    
    while(1) {
        if(data_recved_len>0) {
            com_data_proc(readBuffer, data_recved_len);
            data_recved_len = 0;
        }
    }
}


