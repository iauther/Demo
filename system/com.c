#include "data.h"
#include "error.h"
#include "com.h"
#include "paras.h"
#include "upgrade.h"
#include "myCfg.h"
#include "log.h"
#include "task.h"

#ifdef _WIN32
#include "win.h"
#else
#include "drv/uart.h"
#include "drv/jump.h"
#endif

#include "log.h"


static handle_t comHandle=NULL;
static int paras_rxtx_flag = 0;


static handle_t pt_init(com_callback cb)
{
#ifdef _WIN32
    return NULL;
#else
    uart_cfg_t uc;
    
    uc.mode = MODE_DMA;
    uc.port = COM_UART_PORT;               //PA2: TX   PA3: RX
    uc.baudrate = COM_BAUDRATE;
    uc.para.rx = cb;
    uc.para.buf = pkt_rx_buf;
    uc.para.blen = sizeof(pkt_rx_buf);
    uc.para.dlen = 0;
    
    return uart_init(&uc);
#endif

}


int com_init(com_callback cb, int loop_period)
{
    int r;
    pkt_cfg_t cfg;
    
    comHandle = pt_init(cb);
    
    cfg.handle  = comHandle;
    cfg.period  = loop_period;
    r = pkt_init(&cfg);
    
    return r;
}


int com_deinit(void)
{
#ifdef _WIN32
    return 0;
#else
    return uart_deinit(&comHandle);
#endif
    
}


static int cmd_proc(void *data)
{
    int r;
    U8  err=0;
    cmd_t *c=(cmd_t*)data;
    
    switch(c->cmd) {
#ifdef OS_KERNEL
        case CMD_PUMP_START:
        {
            r = n950_start();
            if(r<0) {
                err = ERROR_PWM_PUMP;       //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            }
            else {
                sysState = STAT_RUNNING;
            }
        }
        break;
        
        case CMD_PUMP_STOP:
        {
            r = n950_stop();
            if(r<0) {
                err = ERROR_PWM_PUMP;       //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            }
            else {
                sysState = STAT_STOP;
            }
        }
        break;
        
        case CMD_PUMP_SPEED:
        {
            r = n950_set_speed(c->para);
            if(r<0) {
                err = (r==-2)?ERROR_CMD_PARA:ERROR_PWM_PUMP;
            }
        }
        break;
        
        case CMD_VALVE_SET:
        {
            if(c->para) {
                valve_set(VALVE_OPEN);
                adjMode = ADJ_MANUAL;
                err = n950_send_cmd(CMD_PUMP_STOP, 0);
            }
            else {
                valve_set(VALVE_CLOSE);
                adjMode = ADJ_AUTO;
            }
        }
        break;
#endif

        case CMD_SYS_RESTART:
        {
#ifndef _WIN32
            reboot();
#endif
        }
        break;
        
        case CMD_SET_FACTORY:
        {
            r = paras_reset();
            if(r==0) {
                r = pkt_send(TYPE_PARAS, 1, &curParas, sizeof(curParas));
                if(r) {
                    err = ERROR_UART2_COM;
                }
            }
            else {
                err = ERROR_I2C2_E2P;
            }
        }
        break;
        
        default:
        {
            err = ERROR_CMD_CODE;
        }
        break;
    }
    
    return err;
}


//////////////////////////////////////////
#ifdef _WIN32
U8 com_data_proc(void *data, U16 len)
{
    int r;
    U8 err=0;
    pkt_hdr_t *p=(pkt_hdr_t*)data;

    err = pkt_hdr_check(p, len);
    if (err == ERROR_NONE) {
        if (p->askAck) {
            pkt_send_ack(p->type);
        }

        switch (p->type) {
        case TYPE_STAT:
        {
            stat_t* stat = (stat_t*)p->data;
            curStat = *stat;

             //LOG("_____ sysState: %d\n", curStat.sysState);
            //LOG("_____ stat temp: %f, aPres: %fkpa, dPres: %fkpa\n", stat->temp, stat->aPres, stat->dPres);
            win_stat_update(stat);
        }
        break;

        case TYPE_SETT:
        {
            
        }
        break;

        case TYPE_PARAS:
        {
            LOG("___ paras recived\n");
            if (p->dataLen > 0) {
                if (p->dataLen == sizeof(paras_t)) {
                    paras_t* paras = (paras_t*)p->data;
                    curParas = *paras;
                    paras_rxtx_flag = 1;
                    win_paras_update(&curParas);
                }
                else {
                    err = ERROR_PKT_LENGTH;
                    LOG("___ paras length is wrong\n");
                }
            }
        }
        break;

        case TYPE_ERROR:
        {
            error_t *e=(error_t*)p->data;
            LOG("___ type_error, type: %d, error: 0x%02x\n", e->type, e->error);
            err = 0;
        }
        break;
        
        case TYPE_UPGRADE:
        {
            //err = upgrade_proc(p->data);
        }
        break;
        
        case TYPE_ACK:
        {
            ack_t* ack = (ack_t*)p->data;
            
            if (ack->type==TYPE_UPGRADE) {
                extern int upgrade_ack;
                upgrade_ack = 1;
            }

            LOG("___ type_ack, type: %d\n", ack->type);
            pkt_ack_reset(ack->type);
        }
        break;

        case TYPE_LEAP:
        {
            LOG("___ type_leap\n");
            err=0;
        }
        break;

        default:
        {
            err = ERROR_PKT_TYPE;
        }
        break;
        }
    }

    if (err) {
        pkt_send_err(p->type, err);
    }
    
    return err;
    
}
#else
 U8 com_data_proc(void *data, U16 len)
{
    int r;
    U8 err=0;
    node_t nd;
    pkt_hdr_t *p=(pkt_hdr_t*)data;

    err = pkt_hdr_check(p, len);
    if (err == ERROR_NONE) {
        if (p->askAck) {
            pkt_send_ack(p->type);
        }
        
        switch (p->type) {
#ifdef OS_KERNEL
        case TYPE_SETT:
        {
            if (p->dataLen > 0) {
                if (p->dataLen == sizeof(sett_t)) {
                    sett_t* sett = (sett_t*)p->data;
                    curParas.setts.sett[sett->mode] = *sett;
                    nd.ptr = &curParas.setts.sett[sett->mode];
                    nd.len = sizeof(curParas.setts.sett[sett->mode]);
                }
                else if (p->dataLen == sizeof(curParas.setts.mode)) {
                    U8* m = (U8*)p->data;
                    curParas.setts.mode = *m;
                    nd.ptr = &curParas.setts.mode;
                    nd.len = sizeof(curParas.setts.mode);
                }
                else {
                    setts_t* setts = (setts_t*)p->data;
                    curParas.setts = *setts;
                    nd.ptr = &curParas.setts;
                    nd.len = sizeof(curParas.setts);
                }
                
                #ifdef OS_KERNEL
                task_misc_save_paras(&nd);
                #endif
            }
            else {
                r = pkt_send(TYPE_SETT, 0, &curParas.setts, sizeof(curParas.setts));
                if(r) {
                    err = ERROR_UART2_COM;
                }
            }
        }
        break;

        case TYPE_PARAS:
        {
            if (p->dataLen > 0) {
                if (p->dataLen == sizeof(paras_t)) {
                    paras_t* paras = (paras_t*)p->data;
                    //curParas = *paras;
                }
                else {
                    err = ERROR_PKT_LENGTH;
                }
            }
            else {
                r = pkt_send(TYPE_PARAS, 0, &curParas, sizeof(curParas));
                if(r) {
                    err = ERROR_UART2_COM;
                }
            }
        }
        break;
        
        case TYPE_STAT:
        {
            r = pkt_send(TYPE_STAT, 0, &curStat, sizeof(curStat));
            if(r) {
                err = ERROR_UART2_COM;
            }
        }
        break;
        
        case TYPE_UPGRADE:
        {
            err = upgrade_proc(p->data);
        }
        break;
        
        case TYPE_ACK:
        {
            ack_t* ack = (ack_t*)p->data;
            if (ack->type==TYPE_PARAS) {
                paras_rxtx_flag = 1;
            }

            pkt_ack_reset(ack->type);
        }
        break;
        
        case TYPE_CMD:
        {
            err = cmd_proc(p->data);
        }
        break;
        
        case TYPE_LEAP:
        {
            err=0;
        }
        break;
#else
        case TYPE_STAT:
        {
            curStat.sysState = sysState;
            r = pkt_send(TYPE_STAT, 0, &curStat, sizeof(curStat));
            if(r) {
                err = ERROR_UART2_COM;
            }
        }
        break;
        
        case TYPE_UPGRADE:
        {
            err = upgrade_proc(p->data);
        }
        break;
        
        case TYPE_ACK:
        {
            ack_t* ack = (ack_t*)p->data;
            if (ack->type==TYPE_PARAS) {
                paras_rxtx_flag = 1;
            }

            pkt_ack_reset(ack->type);
        }
        break;
        
        case TYPE_CMD:
        case TYPE_LEAP:
        case TYPE_SETT:
        case TYPE_PARAS:
        {
            err = ERROR_PKT_TYPE_UNSUPPORT;
        }
        break;
#endif

        default:
        {
            err = ERROR_PKT_TYPE;
        }
        break;
        }
    }

    if (err) {
        pkt_send_err(p->type, err);
    }
    
    return err;
}
#endif


U8 com_send_paras(U8 flag)
{
    int r;
    U8  err=0;
    
    if (flag == 0) {      //ask paras
        r = pkt_send(TYPE_PARAS, 0, NULL, 0);
        if (r) {
            err = ERROR_UART2_COM;
        }
    }
    else {
        r = pkt_send(TYPE_PARAS, 1, &curParas, sizeof(curParas));
        if (r ) {
            err = ERROR_UART2_COM;
        }
    }
    
    return err;
}


U8 com_send_data(U8 type, U8 nAck, void* data, U16 len)
{
    int r;
    r =  pkt_send(type, nAck, data, len);
    return (r == 0) ? 0 : ERROR_UART2_COM;
}


int com_check_timeout(U8 type)
{
    return  pkt_check_timeout(type);
}


int com_get_buf(U8** data, U16 *len)
{
    if(data) {
        *data = pkt_rx_buf;
    }
    
    if(len) {
        *len = PKT_BUFLEN;
    }
    
    return 0;
}


int com_get_paras_flag(void)
{
    return paras_rxtx_flag;
}

U8 com_ack_poll(void)
{
    int i,r;
    U8 err = 0;

    for (i = 0; i < TYPE_MAX; i++) {
        r = com_check_timeout(i);
        if (r > 0) { //timeout
            r = pkt_resend(i);
            if (r) {
                err = ERROR_UART2_COM;
            }
        }
    }

    return err;
}

