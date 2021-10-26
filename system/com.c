#include "data.h"
#include "error.h"
#include "com.h"
#include "paras.h"
#include "upgrade.h"
#include "myCfg.h"
#include "log.h"
#include "task.h"
#include "power.h"
#include "board.h"

#ifdef _WIN32
#include "win.h"
#else
#include "drv/uart.h"
#include "drv/jump.h"
#endif

#include "log.h"


static handle_t comHandle=NULL;


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
    stat_t stat={0};
    pkt_cfg_t cfg;
    
    comHandle = pt_init(cb);
    
    cfg.handle  = comHandle;
    cfg.period  = loop_period;
    r = pkt_init(&cfg);

#ifndef _WIN32    
    stat.sysState = STAT_UPGRADE;
    com_send_data(TYPE_STAT, 0, &stat, sizeof(stat));
#endif
    
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
            if(sysState!=STAT_RUNNING) {
                r = n950_start();
                if(r<0) {
                    err = ERROR_PWM_PUMP;       //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                }
                else {
                    sysState = STAT_RUNNING;
                }
            }
        }
        break;
        
        case CMD_PUMP_STOP:
        {
            if(sysState!=STAT_STOP) {
                r = n950_stop();
                if(r<0) {
                    err = ERROR_PWM_PUMP;       //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                }
                else {
                    sysState = STAT_STOP;
                }
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
                sysState = STAT_STOP;
            }
            else {
                valve_set(VALVE_CLOSE);
                adjMode = ADJ_AUTO;
            }
        }
        break;
        
        case CMD_SYS_FACTORY:
        {
            r = paras_reset();
            if(r==0) {
                r = pkt_send(TYPE_PARA, 1, &curPara, sizeof(curPara));
                if(r) {
                    err = ERROR_UART2_COM;
                }
            }
            else {
                err = ERROR_I2C2_E2P;
            }
        }
        break;
        
        case CMD_SYS_POWEROFF:
        {
        #ifndef _WIN32
            sysState = STAT_POWEROFF;
            notice_stop(DEV_LED|DEV_BUZZER);led_set_color(BLANK); buzzer_stop();
            power_on(PDEV_PAD, 0);
        #endif
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
extern int paras_rx_flag;
extern int upgrade_ack_error;
U8 com_data_proc(void *data, U16 len)
{
    int r;
    U8 err=0;
    pkt_hdr_t *p=(pkt_hdr_t*)data;

    err = pkt_hdr_check(p, len);
    if (err == ERROR_NONE) {

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

        case TYPE_PARA:
        {
            LOG("___ paras recived\n");
            if (p->dataLen > 0) {
                if (p->dataLen == sizeof(para_t)) {
                    para_t* para = (para_t*)p->data;
                    curPara = *para;
                    paras_rx_flag = 1;
                    win_paras_update(&curPara);
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
            error_t *err=(error_t*)p->data;
            LOG("___ type_error, type: %d, error: 0x%02x\n", err->type, err->error);
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
                if (ack->error==0) {
                    upgrade_ack_error = 1;
                }
                else {
                    if (ack->error== ERROR_FW_PKT_ID) {
                        upgrade_ack_error = 2;
                    }
                    else {
                        upgrade_ack_error = 3;
                    }
                }
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

    if (p->askAck) {
        pkt_send_ack(p->type, err);
    }
    else {
        if (err) {
            //pkt_send_err(p->type, err);
        }
    }
    
    return err;
    
}
#else
static U8 normal_data_proc(pkt_hdr_t *p)
{
    int r;
    U8  err=0;
    U8 upgrade_flag=0;
    
    switch (p->type) {
        case TYPE_SETT:
        {
            node_t nd;
            if (p->dataLen > 0) {
                if (p->dataLen == sizeof(sett_t)) {
                    sett_t* sett = (sett_t*)p->data;
                    
                    if(sett->mode>=MODE_MAX) {
                        err = ERROR_DAT_MODE;
                    }
                    else if(sett->pres<=1.0F || sett->pres>110.0F) {
                        err = ERROR_DAT_VACUUM;
                    }
                    else {
                        sett_t *s1=sett;
                        sett_t *s2=&curPara.setts.sett[sett->mode];
                        
                        if(s1->pres != s2->pres) {
                            vacuum_reached = 0;
                            air_act = (s1->pres>s2->pres)?AIR_PUMP:AIR_LEAK;
                        }
                        
                        curPara.setts.sett[sett->mode] = *sett;
                        nd.ptr = &curPara.setts.sett[sett->mode];
                        nd.len = sizeof(curPara.setts.sett[sett->mode]);
                    }   
                }
                else if (p->dataLen == sizeof(curPara.setts.mode)) {
                    U8 mode = *((U8*)p->data);
                    
                    if(mode>=MODE_MAX) {
                        err = ERROR_DAT_MODE;
                    }
                    else if(curPara.setts.mode!=mode){
                        sett_t *s1=&curPara.setts.sett[mode];
                        sett_t *s2=&curPara.setts.sett[curPara.setts.mode];
                        
                        if(s1->pres != s2->pres) {
                            vacuum_reached = 0;
                            air_act = (s1->pres>s2->pres)?AIR_PUMP:AIR_LEAK;
                        }
                        
                        curPara.setts.mode = mode;
                        nd.ptr = &curPara.setts.mode;
                        nd.len = sizeof(curPara.setts.mode);
                    }
                }
                else if (p->dataLen == sizeof(setts_t)){
                    setts_t* setts = (setts_t*)p->data;
                    
                    if(setts->mode>=MODE_MAX) {
                        err = ERROR_DAT_MODE;
                    }
                    else {
                        
                        sett_t *s1=setts[setts->mode].sett;
                        sett_t *s2=&curPara.setts.sett[curPara.setts.mode];
                        
                        if(s1->pres != s2->pres) {
                            vacuum_reached = 0;
                            air_act = (s1->pres>s2->pres)?AIR_PUMP:AIR_LEAK;
                        }
                        
                        curPara.setts = *setts;
                        nd.ptr = &curPara.setts;
                        nd.len = sizeof(curPara.setts);
                    }
                }
                
                paras_write_node(&nd);
            }
            else {
                r = pkt_send(TYPE_SETT, 0, &curPara.setts, sizeof(curPara.setts));
                if(r) {
                    err = ERROR_UART2_COM;
                }
            }
        }
        break;

        case TYPE_PARA:
        {
            if (p->dataLen > 0) {
                if (p->dataLen == sizeof(para_t)) {
                    paras_t* paras = (paras_t*)p->data;
                    //curParas = *paras;
                }
                else {
                    err = ERROR_PKT_LENGTH;
                }
            }
            else {
                r = pkt_send(TYPE_PARA, 0, &curPara, sizeof(curPara));
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
            upgrade_flag = 1;
        }
        break;
        
        case TYPE_ACK:
        {
            ack_t* ack = (ack_t*)p->data;

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
        
        default:
        {
            err = ERROR_PKT_TYPE;
        }
        break;
    }
    
    if (p->askAck) {
        pkt_send_ack(p->type, err);
    }
    else {
        if (err) {
            pkt_send_err(p->type, err);
        }
    }
    
    if(upgrade_flag==1) {
        paras_set_upg();
        reboot();
    }
    
    return err;
}


static void jumpto_app(void)
{
    com_deinit();
    board_deinit();
    jump_to_app();
}
static U8 upgrade_data_proc(pkt_hdr_t *p)
{
    int r;
    U8  err=0;
    
    switch (p->type) {
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

            pkt_ack_reset(ack->type);
        }
        break;
        
        case TYPE_CMD:
        {
            err = cmd_proc(p->data);
        }
        break;
        
        case TYPE_LEAP:
        case TYPE_SETT:
        case TYPE_PARA:
        case TYPE_ERROR:
        {
            err = ERROR_PKT_TYPE_UNSUPPORT;
        }
        break;

        default:
        {
            err = ERROR_PKT_TYPE;
        }
        break;
    }
    
    if (p->askAck) {
        pkt_send_ack(p->type, err);
    }
    else {
        if (err) {
            pkt_send_err(p->type, err);
        }
    }
    
    if(upgrade_is_finished()) {
        if(upgrade_is_app()) {
            jumpto_app();
        }
        else {
            paras_clr_upg();
            reboot();
        }
    }
    
    return err;
}


U8 com_data_proc(void *data, U16 len)
{
    int r;
    U8 err=0;
    node_t nd;
    pkt_hdr_t *p=(pkt_hdr_t*)data;

    err = pkt_hdr_check(p, len);
    if (err == ERROR_NONE) {
        if(sysState==STAT_UPGRADE) {
            err = upgrade_data_proc(p);
        }
        else {
            err = normal_data_proc(p);
        }
    } 
    
    return err;
}
#endif


U8 com_send_paras(U8 flag)
{
    int r;
    U8  err=0;
    
    if (flag == 0) {      //ask paras
        r = pkt_send(TYPE_PARA, 0, NULL, 0);
        if (r) {
            err = ERROR_UART2_COM;
        }
    }
    else {
        r = pkt_send(TYPE_PARA, 1, &curPara, sizeof(curPara));
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



U8 com_ack_poll(void)
{
    int i,r;
    U8 err = 0;

    if(curStat.sysState!=STAT_UPGRADE) {
        for (i = 0; i < TYPE_MAX; i++) {
            r = com_check_timeout(i);
            if (r > 0) { //timeout
                r = pkt_resend(i);
                if (r) {
                    err = ERROR_UART2_COM;
                }
            }
        }
    }

    return err;
}

