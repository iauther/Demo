#include "drv/jump.h"
#include "board.h"
#include "upgrade.h"
#include "drv/uart.h"
#include "data.h"
#include "cfg.h"
#include "pkt.h"


#define BUFLEN 1000
static U8 rxBuf[BUFLEN];
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


static U8 upgrade_proc(void *data)
{
    int r=0;
    static U16 upg_pid=0;
    upgrade_pkt_t *upg=(upgrade_pkt_t*)data;
    
    if(upg->pid==0) {
        upg_pid = 0;
    }
    
    if(upg->pid!=upg_pid) {
        return ERROR_FW_PKT_ID;
    }
    
    if(upg->pid==upg->pkts-1) {
        //upgrade finished
    }
    else {
        upg_pid++;
    }
    
    return r;
}
static U8 com_proc(pkt_hdr_t *p, U16 len)
{
    U8 ack=0,err;
    
    if(p->askAck)  ack = 1;
    
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
                if(p->askAck) {
                    pkt_send_ack(p->type, 0); ack = 0;
                }
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
    
    if(ack || (err && !ack)) {
        pkt_send_ack(p->type, err);
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
    pkt_hdr_t *p=(pkt_hdr_t*)rxBuf;
    
    board_init();
    
    if(!need_upgrade()) {
        jump_to_app();
    }
    
    while(1) {
        if(data_ready_flag>0) {
            com_proc(p, data_recved_len);
            data_ready_flag = 0;
        }
    }

}


