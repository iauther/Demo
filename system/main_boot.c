#include "drv/jump.h"
#include "drv/uart.h"
#include "drv/delay.h"
#include "board.h"
#include "paras.h"
#include "upgrade.h"
#include "data.h"
#include "cfg.h"
#include "pkt.h"


#define BUFLEN 1000
static U8 rx_buf[BUFLEN];
static U8  upgrade_flag=0;
static U16 data_recved_len=0;
static handle_t comHandle=NULL;
static void rx_callback(U8 *data, U16 len);

static void rx_callback(U8 *data, U16 len)
{
    data_recved_len = len;
}

static void com_test(void)
{
    U8 tmp[5]={0x11,0x22,0x33,0x44,0x55};
    while(1) {
        uart_write(comHandle, tmp, sizeof(tmp));
        delay_ms(50);
    }
}

static void com_init(void)
{
    uart_cfg_t uc;
    
    uc.mode = MODE_DMA;
    uc.port = UART_2;               //PA2: TX   PA3: RX
    uc.baudrate = COM_BAUDRATE;
    uc.para.rx = rx_callback;
    uc.para.buf = rx_buf;
    uc.para.blen = sizeof(rx_buf);
    uc.para.dlen = 0;
    comHandle = uart_init(&uc);
}
static void com_deinit(void)
{
    uart_deinit(&comHandle);
}




static int need_upgrade(void)
{
    int r=0;
    U32 fwMagic;
    
    paras_get_fwmagic(&fwMagic);
    if(fwMagic!=FW_MAGIC) {
        r = 1;
    }
    
    return r;
}


static U8 upgrade_proc(void *data)
{
    int r=0;
    static U16 upg_pid=0;
    upgrade_pkt_t *upg=(upgrade_pkt_t*)data;
    
    if(upg->pid==0) {
        upg_pid = 0;
        //upgrade_init();
    }
    
    if(upg->pid!=upg_pid) {
        return ERROR_FW_PKT_ID;
    }
    
    if(upg->pid==upg->pkts-1) {
        upgrade_write(upg->data, upg->dataLen);
    }
    else {
        upg_pid++;
    }
    
    return r;
}
static U8 com_proc(pkt_hdr_t *p, U16 len)
{
    U8 err;
    
    if(p->askAck) {
        pkt_send_ack(p->type);
    }
    
    err = pkt_hdr_check(p, len);
    if(err==ERROR_NONE) {
        switch(p->type) {
            
            case TYPE_ACK:
            {
                ack_t *ack=(ack_t*)p->data;
                pkt_ack_update(ack->type);
            }
            break;
            
            case TYPE_UPGRADE:
            {
                err = upgrade_proc(p->data);
            }
            break;
            
            default:
            {
                err = ERROR_PKT_TYPE;
            }
            break;
        }
    }
    
    if(err) {
        pkt_send_err(p->type, err);
    }
    
    return err;
}
static void timer_proc(void)
{
    U8 i;
    
    for(i=0; i<TYPE_MAX; i++) {
        pkt_ack_timeout_check(i);
    }
}




int main(void)
{
    int upgrade_flag;
    pkt_hdr_t *p=(pkt_hdr_t*)rx_buf;
    
    board_init();
    com_init();
    //com_test();
    
    //if(!need_upgrade()) {
    if(1) {
        board_deinit();
        com_deinit();
        
        jump_to_app();
    }
    
    while(1) {
        if(data_recved_len>0) {
            com_proc(p, data_recved_len);
            data_recved_len = 0;
        }
    }
}


