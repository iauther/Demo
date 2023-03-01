#include "incs.h"
#ifdef _WIN32
#include "win.h"
#endif



static handle_t comHandle=NULL;


int com_init(U8 port, port_callback_t cb, int loop_period)
{
    int r;
    stat_t stat={0};
    pkt_cfg_t cfg;
    
    cfg.port = port;
    cfg.cb     = cb;
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

        case 0xff:
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

            //pkt_ack_reset(ack->type);
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

            //pkt_ack_reset(ack->type);
        }
        break;
        
        case TYPE_CMD:
        {
            err = cmd_proc(p->data);
        }
        break;
        
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
    return 0;//pkt_check_timeout(type);
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


