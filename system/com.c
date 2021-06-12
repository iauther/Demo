#include "data.h"
#include "error.h"
#include "com.h"
#include "myCfg.h"
#include "paras.h"
#include "upgrade.h"
#ifdef _WIN32
#include "logx.h"
#include "win.h"
#else
#include "drv/uart.h"
#include "drv/jump.h"
#endif


#define RX_BUFLEN           1000
static U8 com_rx_buf[RX_BUFLEN];
static U8 paras_rtx_flag=0;
static handle_t comHandle=NULL;


#ifdef _WIN32
const char* typeString2[TYPE_MAX] = {
    "TYPE_CMD",
    "TYPE_STAT",
    "TYPE_ACK",
    "TYPE_SETT",
    "TYPE_PARAS",
    "TYPE_ERROR",
    "TYPE_UPGRADE",
    "TYPE_LEAP",
};
static void err_print(U8 type, U8 err)
{
    switch (err) {
    case ERROR_NONE:
        LOG("___ all is ok\n");
        break;
    case ERROR_DEV_PUMP_RW:
        LOG("___ pump read/write error\n");
        break;
    case ERROR_DEV_MS4525_RW:
        LOG("___ ms4525 read/write error\n");
        break;
    case ERROR_DEV_E2PROM_RW:
        LOG("___ eeprom read/write error\n");
        break;
    case ERROR_PKT_TYPE:
        LOG("___ pkt type is wrong\n");
        break;
    case ERROR_PKT_MAGIC:
        LOG("___ pkt magic is wrong\n");
        break;
    case ERROR_PKT_LENGTH:
        LOG("___ pkt length is wrong\n");
        break;
    case ERROR_PKT_CHECKSUM:
        LOG("___ pkt checksum is wrong\n");
        break;
    case ERROR_DAT_TIMESET:
        LOG("___ para timeset is wrong\n");
        break;
    case ERROR_DAT_VACUUM:
        LOG("___ para vacuum is wrong\n");
        break;
    case ERROR_DAT_VOLUME:
        LOG("___ para volume is wrong\n");
        break;
    case ERROR_FW_INFO_VERSION:
        LOG("___ fw version is wrong\n");
        break;
    case ERROR_FW_INFO_BLDTIME:
        LOG("___ fw buildtime is wrong\n");
        break;
    case ERROR_FW_PKT_ID:
        LOG("___ fw pkt id is wrong\n");
        break;
    case ERROR_FW_PKT_LENGTH:
        LOG("___ fw pkt length is wrong\n");
        break;
    case ERROR_ACK_TIMEOUT:
        LOG("___ %s, ack is timeout\n", typeString2[type]);
        break;
    default:
        LOG("___ unknow error: %d\n", err);
        break;
    }
}
static void pkt_print(pkt_hdr_t* p, U16 len)
{
    LOG("_____ pkt length:   %d\n", len);

    LOG("_____ pkt.magic: 0x%08x\n", p->magic);
    LOG("_____ pkt.type:  0x%02x\n", p->type);
    LOG("_____ pkt.askAck: %d\n",    p->askAck);
    LOG("_____ pkt.flag:  0x%02x\n", p->flag);
    LOG("_____ pkt.dataLen: %d\n",   p->dataLen);

    if (p->dataLen > 0) {
        switch (p->type) {
        case TYPE_CMD:
        {
            if (p->dataLen == sizeof(cmd_t)) {
                cmd_t* cmd = (cmd_t*)p->data;
                LOG("_____ cmd.cmd: 0x%08x, cmd.para: %d\n", cmd->cmd, cmd->para);
            }
        }
        break;
        case TYPE_ACK:
        {
            if (p->dataLen == sizeof(ack_t)) {
                ack_t* ack = (ack_t*)p->data;
                LOG("_____ ack.type: %d\n", ack->type);
            }
        }
        break;
        case TYPE_SETT:
        {
            if (p->dataLen == sizeof(setts_t)) {
                setts_t* setts = (setts_t*)p->data;
                LOG("_____ setts data\n");
            }
            else if (p->dataLen == sizeof(sett_t)) {
                sett_t* set = (sett_t*)p->data;
                LOG("_____ sett data\n");
            }
            else if (p->dataLen == sizeof(U8)) {
                U8* mode = (U8*)p->data;
                LOG("_____ mode: 0x%08x\n", *mode);
            }
        }
        break;
        case TYPE_PARAS:
        {
            if (p->dataLen == sizeof(paras_t)) {
                paras_t* paras = (paras_t*)p->data;
                LOG("_____ PARAS DATA\n");
            }
        }
        break;
        case TYPE_ERROR:
        {
            if (p->dataLen == sizeof(error_t)) {
                error_t* err = (error_t*)p->data;
                LOG("_____ err.error: %d\n", err->error);
            }
        }
        break;
        case TYPE_UPGRADE:
        {
            if (p->dataLen == sizeof(upgrade_pkt_t)) {
                upgrade_pkt_t* up = (upgrade_pkt_t*)p->data;
                LOG("_____ pkt.pkts: %d\n", up->pkts);
                LOG("_____ pkt.pid:  %d\n", up->pid);
                LOG("_____ pkt.dataLen: %d\n", up->dataLen);
            }
        }
        break;
        case TYPE_LEAP:
        {
            if (p->dataLen == 0) {
                LOG("_____ pkt leap\n");
            }
        }
        break;
        }
    }
}
#endif




static handle_t port_init(com_callback cb)
{
#ifdef _WIN32
    return NULL;
#else
    uart_cfg_t uc;
    
    uc.mode = MODE_DMA;
    uc.port = COM_UART_PORT;               //PA2: TX   PA3: RX
    uc.baudrate = COM_BAUDRATE;
    uc.para.rx = cb;
    uc.para.buf = com_rx_buf;
    uc.para.blen = sizeof(com_rx_buf);
    uc.para.dlen = 0;
    return uart_init(&uc);
#endif

}


int com_init(com_callback cb, int loop_period)
{
    int r;
    pkt_cfg_t cfg;
    
    comHandle = port_init(cb);
    
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
                sysState = STAT_AUTO;
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
                sysState = STAT_MANUAL;
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
                sysState = STAT_MANUAL;
                err = n950_send_cmd(CMD_PUMP_STOP, 0);
            }
            else {
                valve_set(VALVE_CLOSE);
                sysState = STAT_AUTO;
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


U8 com_data_proc(void *data, U16 len)
{
    int r;
    U8 err=0;
    pkt_hdr_t *p=(pkt_hdr_t*)data;

#ifdef _WIN32
    pkt_print(p, len);
#endif

    if (p->askAck) {
        pkt_send_ack(p->type);
    }

    err = pkt_hdr_check(p, len);
    if (err == ERROR_NONE) {
        switch (p->type) {
#ifdef OS_KERNEL
        case TYPE_SETT:
        {
            if (p->dataLen > 0) {
                if (p->dataLen == sizeof(sett_t)) {
                    sett_t* sett = (sett_t*)p->data;
                    curParas.setts.sett[sett->mode] = *sett;
                }
                else if (p->dataLen == sizeof(curParas.setts.mode)) {
                    U8* m = (U8*)p->data;
                    curParas.setts.mode = *m;
                }
                else {
                    memcpy(&curParas.setts.sett, p->data, sizeof(curParas.setts));
                }
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
                    curParas = *paras;
                }
                else {
                    err = ERROR_PKT_LENGTH;
                }
            }
            else {
                r = pkt_send(TYPE_PARAS, 0, &curParas, sizeof(curParas));
                if(r==0) {
                    paras_rtx_flag = 1;
                }
                else {
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
#endif

#ifdef _WIN32

        case TYPE_STAT:
        {
            stat_t* stat = (stat_t*)p->data;

            LOG("_____ stat temp: %f, aPres: %fkpa, dPres: %fkpa\n", stat->temp, stat->aPres, stat->dPres);
            win_stat_update(stat);
        }
        break;

        case TYPE_SETT:
        {
            if (p->dataLen > 0) {
                if (p->dataLen == sizeof(sett_t)) {
                    sett_t* sett = (sett_t*)p->data;
                    curParas.setts.sett[sett->mode] = *sett;
                }
                else if (p->dataLen == sizeof(curParas.setts.mode)) {
                    U8* m = (U8*)p->data;
                    curParas.setts.mode = *m;
                }
                else {
                    memcpy(&curParas.setts.sett, p->data, sizeof(curParas.setts.sett));
                }
            }
            else {
                pkt_send(TYPE_SETT, 0, &curParas.setts, sizeof(curParas.setts));
            }
        }
        break;

        case TYPE_PARAS:
        {
            LOG("___ paras recived\n");
            if (p->dataLen > 0) {
                if (p->dataLen == sizeof(paras_t)) {
                    paras_t* paras = (paras_t*)p->data;
                    curParas = *paras;
                    paras_rtx_flag = 1;
                }
                else {
                    err = ERROR_PKT_LENGTH;
                    LOG("___ paras length is wrong\n");
                }
            }
        }
        break;
#endif

        case TYPE_CMD:
        {
            err = cmd_proc(p->data);
        }
        break;

        case TYPE_ERROR:
        {
            err = 0;
        }
        
        case TYPE_UPGRADE:
        {
            err = upgrade_proc(p->data);
        }
        break;
        
        case TYPE_ACK:
        {
            ack_t* ack = (ack_t*)p->data;
            pkt_ack_update(ack->type);
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
    }

    if (err) {
        pkt_send_err(p->type, err);
    }

#ifdef _WIN32
    err_print(p->type, err);
#endif
    
    return err;
}


U8 com_send_data(U8 type, U8 nAck, void* data, U16 len)
{
    int r;
    r =  pkt_send(type, nAck, data, len);
    return (r == 0) ? 0 : ERROR_UART2_COM;
}



U8 com_loop_check(U8 flag)      //0: app,  1:dev
{
    int r;
    U8  tp,err=0;
    
    if(!paras_rtx_flag) {
        if (flag==0) {
            r = pkt_send(TYPE_PARAS, 1, NULL, 0);
            if (r) {
                err = ERROR_UART2_COM;
            }
        }
        else {
            r = pkt_send(TYPE_PARAS, 1, &curParas, sizeof(curParas));
            if (r == 0) {
                paras_rtx_flag = 1;
            }
            else {
                err = ERROR_UART2_COM;
            }
        }
    }

#if 1
    for(tp=0; tp<TYPE_MAX; tp++) {
        r = pkt_ack_is_timeout(tp);
        if(r==1) {  //timeout
            //show some error here
            //LOGE("_______%d no ack\n");
        }
    }
#endif
    
    return err;
}


int com_get_buf(U8** data, U16 *len)
{
    if(data) {
        *data = com_rx_buf;
    }
    
    if(len) {
        *len = RX_BUFLEN;
    }
    
    return 0;
}



