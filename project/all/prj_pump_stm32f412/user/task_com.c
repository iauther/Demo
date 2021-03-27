#include "task.h"                          // CMSIS RTOS header file
#include "data.h" 
#include "log.h" 
#include "upgrade.h" 
#include "drv/uart.h" 


#define COM_BAUDRATE        115200

handle_t comHandle;
static U8 com_rx_buf[MAXLEN];
static U8 com_tx_buf[MAXLEN];

static void com_rx_callback(U8 *data, U16 len)
{
    task_msg_post(TASK_COM, EVT_COM, 0, data, len);
}
static void com_init(void)
{
    uart_cfg_t uc;
    
    uc.mode = MODE_DMA;
    uc.port = UART_2;
    uc.baudrate = COM_BAUDRATE;
    uc.para.rx = com_rx_callback;
    uc.para.buf = com_rx_buf;
    uc.para.blen = sizeof(com_rx_buf);
    uc.para.dlen = 0;
    
    comHandle = uart_init(&uc);
}

static U8 get_checksum(U8 *data, U16 len)
{
    U8 sum = 0;
    for (int i = 0; i < len; i++) {
        sum += data[i];
    }
    return sum;
}
int com_send(U8 type, U8 nAck, void *data, U16 len)
{
    int r;
    U8  sum;
    U8* buf = com_tx_buf;
    pkt_hdr_t* p = (pkt_hdr_t*)buf;
    U32 totalLen=PKT_HDR_LENGTH+len;

    p->magic = PKT_MAGIC;
    p->type  = type;
    p->flag  = 0;
    p->askAck = nAck;
    p->dataLen = len;
    memcpy(p->data, data, len);
    
    sum = get_checksum(buf, totalLen);
    buf[totalLen] = sum;

    r = uart_write(comHandle, buf, totalLen+1);
    
    return r;
}
int com_ack(U8 error)
{
    U8* buf=com_tx_buf;
    pkt_hdr_t* p=(pkt_hdr_t*)buf;
    ack_t *ack=(ack_t*)p->data;
    U32 totalLen=PKT_HDR_LENGTH+sizeof(ack_t);

    p->magic = PKT_MAGIC;
    p->type  = TYPE_ACK;
    p->flag  = 0;
    p->askAck = 0;
    p->dataLen = sizeof(ack_t);
    
    ack->error = error;
    buf[totalLen] = get_checksum(buf, totalLen);

    return uart_write(comHandle, buf, totalLen+1);
}



static int cmd_proc(void *data)
{
    cmd_t *c=(cmd_t*)data;
    n950_send_cmd(c->cmd, c->para);
    
    return 0;
}

static int upgrade_proc(void *data)
{
    int r=0;
    static U16 upg_pid=0;
    upgrade_pkt_t *upg=(upgrade_pkt_t*)data;
    
    if(upg->pid==0) {
        upg_pid = 0;
    }
    
    if(upg->pid!=upg_pid) {
        return -1;
    }
    
    if(upg->pid==upg->pkts-1) {
        //upgrade finished
    }
    else {
        upg_pid++;
    }
    
    return r;
}
static int com_proc(pkt_hdr_t *p)
{
    int r;
    
    switch(p->type) {
        case TYPE_CMD:
        cmd_proc(p);
        break;
        
        case TYPE_ACK:
        break;
        
        case TYPE_SETT:
        {
            if(p->dataLen>0) {
                memcpy(&curSetts, p->data, sizeof(curSetts));
            }
            else {
                com_send(TYPE_SETT, 0, &curSetts, sizeof(curSetts));
            }
        }
        break;
        
        case TYPE_UPGRADE:
        r = upgrade_proc(p->data);
        break;
        
        case TYPE_FWINFO:
        {
            fw_info_t fwInfo;
            upgrade_get_fwinfo(&fwInfo);
            com_send(TYPE_FWINFO, 0, &fwInfo, sizeof(fwInfo));
        }
        break;
        
        default:
        return -1;
    }
    
    if(r || p->askAck) {
        com_ack(r);
    }
    
    return r;
}


void task_com_fn(void *arg)
{
    int r;
    evt_t e;
    U8 checksum;
    U8* buf = com_rx_buf;
    pkt_hdr_t *p=(pkt_hdr_t*)buf;
    task_handle_t *h=(task_handle_t*)arg;
    
    com_init();
    h->running = 1;
    
    while(1) {
        r = msg_recv(h->msg, &e, sizeof(e));
        if(r==0) {
            
            if (p->magic != PKT_MAGIC) {
                LOGE("___ pkt magic is 0x%04x, magic is wrong!\n", p->magic);
                continue;
            }
            if (p->dataLen + PKT_HDR_LENGTH+1 != e.dLen) {
                LOGE("___ pkt length is wrong\n");
                continue;
            }

            checksum = get_checksum(buf, p->dataLen+PKT_HDR_LENGTH);
            if (checksum != buf[p->dataLen + PKT_HDR_LENGTH]) {
                LOGE("___ pkt checksum is wrong!\n");
                continue;
            }
            
            r = com_proc(p);
        }
    }
}



