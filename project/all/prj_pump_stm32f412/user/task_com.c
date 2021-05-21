#include "task.h"                          // CMSIS RTOS header file
#include "data.h" 
#include "log.h"
#include "rbuf.h"
#include "pkt.h"
#include "error.h"
#include "upgrade.h"
#include "evt.h"
#ifndef _WIN32
#include "drv/uart.h" 
#include "drv/jump.h"
#endif
#include "cfg.h"

#define TIMER_MS            100
#define TIMEOUT             500
#define RETRIES             (TIMEOUT/TIMER_MS)

handle_t comHandle;
static U8 paras_tx_flag=0;
static U8 com_rx_buf[MAXLEN];
static void com_rx_callback(U8 *data, U16 len);

static void com_tmr_callback(void *arg)
{
    task_msg_post(TASK_COM, EVT_TIMER, 0, NULL, 0);
}
static void com_rx_callback(U8 *data, U16 len)
{
    task_msg_post(TASK_COM, EVT_COM, 0, data, len);
}
static void port_init(void)
{
    uart_cfg_t uc;
    
    uc.mode = MODE_DMA;
    uc.port = UART_2;               //PA2: TX   PA3: RX
    uc.baudrate = COM_BAUDRATE;
    uc.para.rx = com_rx_callback;
    uc.para.buf = com_rx_buf;
    uc.para.blen = sizeof(com_rx_buf);
    uc.para.dlen = 0;
    comHandle = uart_init(&uc);
}


static void com_init(void)
{
    
    pkt_cfg_t cfg;
    
    port_init();
    
    cfg.handle  = comHandle;
    pkt_init(&cfg);
}

static U8 cmd_proc(void *data)
{
    cmd_t *c=(cmd_t*)data;
    
    switch(c->cmd) {
        case CMD_PUMP_SPEED:
        {
            n950_send_cmd(c->cmd, c->para);
        }
        break;
        
        case CMD_VALVE_SET:
        {
            if(c->para) {
                valve_set(VALVE_OPEN);
                curState = STAT_AUTO;
            }
            else {
                valve_set(VALVE_CLOSE);
                curState = STAT_MANUAL;
            }
        }
        break;
        
        case CMD_SYS_RESTART:
        {
            
        }
        break;
        
        case CMD_SET_FACTORY:
        {
            
        }
        break;
    }
    
    return 0;
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
            case TYPE_CMD:
            {
                err = cmd_proc(p);
            }
            break;
            
            case TYPE_ACK:
            {
                ack_t *ack=(ack_t*)p->data;
                pkt_ack_update(ack->type);
            }
            break;
            
            case TYPE_SETT:
            {
                if(p->dataLen>0) {
                    if(p->dataLen==sizeof(sett_t)) {
                        sett_t *sett=(sett_t*)p->data;
                        curParas.setts.sett[sett->mode] = *sett;
                    }
                    else if(p->dataLen==sizeof(curParas.setts.mode)) {
                        U8 *m=(U8*)p->data;
                        curParas.setts.mode = *m;
                    }
                    else {
                        memcpy(&curParas.setts.sett, p->data, sizeof(curParas.setts));
                    }
                }
                else {
                    if(p->askAck) {
                        pkt_send_ack(p->type, 0); ack = 0;
                    }
                    pkt_send(TYPE_SETT, 0, &curParas.setts, sizeof(curParas.setts));
                }
            }
            break;
            
            case TYPE_PARAS:
            {
                if(p->dataLen>0) {
                    if(p->dataLen==sizeof(paras_t)) {
                        paras_t *paras=(paras_t*)p->data;
                        curParas = *paras;
                    }
                    else {
                        err = ERROR_PKT_LENGTH;
                    }
                }
                else {
                    if(p->askAck) {
                        pkt_send_ack(p->type, 0); ack = 0;
                    }
                    pkt_send(TYPE_PARAS, 0, &curParas, sizeof(curParas));
                    paras_tx_flag = 1;
                }
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
    
    if(!paras_tx_flag) {
        pkt_send(TYPE_PARAS, 1, &curParas, sizeof(curParas));
        paras_tx_flag = 1;
    }
    
    for(i=0; i<TYPE_MAX; i++) {
        pkt_ack_timeout_check(i);
    }
}


void task_com_fn(void *arg)
{
    int r;
    evt_t e;
    osTimerId_t tmrId;
    U8* buf = com_rx_buf;
    pkt_hdr_t *p=(pkt_hdr_t*)buf;
    task_handle_t *h=(task_handle_t*)arg;
    
    com_init();
    tmrId = osTimerNew(com_tmr_callback, osTimerPeriodic, NULL, NULL);
    osTimerStart(tmrId, TIMER_MS);
    h->running = 1;
    
    //jump_to_boot();
    
    while(1) {
        r = msg_recv(h->msg, &e, sizeof(e));
        if(r==0) {
            switch(e.evt) {
                
                case EVT_TIMER:
                {
                    timer_proc();
                }
                break;
                
                case EVT_COM:
                {
                    r = com_proc(p, e.dLen);
                }
                break;
                
            }
        }
    }
}



