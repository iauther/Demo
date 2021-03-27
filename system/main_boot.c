#include "drv/jump.h"
#include "board.h"
#include "upgrade.h"
#include "drv/uart.h"
#include "data.h"
#include "cfg.h"



static U8 rxBuf[200];
static U8 txBuf[200];
static U8  upgrade_flag=0;
static U8  data_ready_flag=0;
static U16 data_recved_len=0;
static void rx_callback(U8 *data, U16 len);


static void rx_callback(U8 *data, U16 len)
{
    data_ready_flag = 1;
    data_recved_len = len;
}


static int need_upgrade(void)
{
    int r=0;
    U32 fwMagic;
    
    upgrade_get_fwmagic(&fwMagic);
    
    if(fwMagic!=FW_MAGIC) {
        r = 1;
    }
    
    return r;
}

static void send_ack(pkt_hdr_t *h, int r)
{
    pkt_hdr_t *hdr=(pkt_hdr_t*)txBuf;
    ack_t *pAck=(ack_t*)hdr->data;
    
    *hdr = *h;
    hdr->askAck = 0;
    hdr->dataLen = sizeof(ack_t);
    pAck->error = r;
    

    //usart_write();
}


static int com_data_proc(U8 *rx, U8 *tx, U16 len)
{
    int r=0, ack=0;
    pkt_hdr_t *h1=(pkt_hdr_t*)rx;
    pkt_hdr_t *h2=(pkt_hdr_t*)tx;
    ack_t *pAck=(ack_t*)h2->data;
    
    if(!len || h1->dataLen+sizeof(pkt_hdr_t)!=len) {
        return -1;
    }
    
    if(h1->magic!=PKT_MAGIC) {
        return -1;
    }
    
    switch(h1->type) {
        case TYPE_CMD:
        {
            cmd_t *cmd=(cmd_t*)h1->data;
            
        }
        break;
        
        case TYPE_UPGRADE:
        {
            upgrade_t *upg=(upgrade_t*)h1->data;
            
        }
        break;
        
        case TYPE_FW_INFO:
        {
            fw_info_t *info=(fw_info_t*)h1->data;
            
        }
        break;
    }
    
    
    if(h1->askAck) {
        ack = 1;
    }
    else {
        if(r) ack = 1;
    }
    
    if(ack) {
        send_ack(h1, r);
    }
    
    return 0;
}




int main(void)
{
    int upgrade_flag;
    
    board_init();
    
    if(!need_upgrade()) {
        jump_to_app();
    }
    
    while(1) {
        if(data_ready_flag>0) {
            
            com_data_proc(rxBuf, txBuf, data_recved_len);
            data_ready_flag = 0;
        }
    }

}


