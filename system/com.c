#include "protocol.h"
#include "paras.h"
#include "incs.h"
#include "lwip.h"
#include "dal/dal.h"
#include "com.h"
#include "cfg.h"

#ifdef _WIN32
#include "win.h"
#endif


static handle_t comHandle=NULL;

static int port_init(com_handle_t *h, U8 port, rx_cb_t callback)
{
     h->port = port;
    
#ifndef _WIN32
    switch(port) {
        
        case PORT_UART:
        {
            uart_cfg_t uc;
            
            uc.mode = MODE_DMA;
            uc.port = COM_UART_PORT;               //PA2: TX   PA3: RX
            uc.baudrate = COM_BAUDRATE;
            uc.para.callback = callback;
            uc.para.buf = h->rxBuf;
            uc.para.blen = sizeof(h->rxBuf);
            uc.para.dlen = 0;
            
            h->h = uart_init(&uc);
        }
        break;
        
        case PORT_ETH:
        {
            net_cfg_t nc;
            
            nc.callback = callback;
            
            h->h = net_init(&nc);
        }
        break;
        
        default:
            return -1;
    }
#endif
    
    return 0;
}


static int port_deinit(com_handle_t *h)
{
    int r=0;
    
#ifndef _WIN32
    switch(h->port) {
        
        case PORT_UART:
        {
            r = uart_deinit(h->h);
        }
        break;
        
        case PORT_ETH:
        {
            r = net_deinit(h->h);
        }
        break;
        
        default:
            return -1;
    }
#endif
    
    return r;
}



static int port_send(com_handle_t *h, void *addr, void *data, int len)
{
    int r=0;
    
#ifdef _WIN32
    return port_send(buf, totalLen + 1);
#else
    switch(h->port) {
        case PORT_UART:
        r = uart_write(h->h, data, len);
        break;
        
        case PORT_ETH:
        r = net_write(h->h, addr, data, len);
        break;
    }
#endif
    
    return r;
}

static int send_data(com_handle_t *h, void *addr, U8 type, U8 nAck, void *data, int dlen)
{
    int len;
    
    len = pkt_pack_data(type, nAck, data, dlen, h->rxBuf, sizeof(h->rxBuf));
    if(len<=0) {
        return len;
    }
    
    return port_send(h, addr, h->rxBuf, len);
}
static int send_ack(com_handle_t *h, void *addr, U8 type, U8 err)
{
    int len;
    
    len = pkt_pack_ack(type, err, h->rxBuf, sizeof(h->rxBuf));
    if(len<=0) {
        return len;
    }
    
    return port_send(h, addr, h->rxBuf, len);
}
static int send_err(com_handle_t *h, void *addr, U8 type, U8 err)
{
    int len;
    
    len = pkt_pack_err(type, err, h->rxBuf, sizeof(h->rxBuf));
    if(len<=0) {
        return len;
    }
    
    return port_send(h, addr, h->rxBuf, len);
}
//////////////////////////////////////////////////////////////

handle_t com_init(U8 port, rx_cb_t cb)
{
    int r;
    com_handle_t *h=calloc(1, sizeof(com_handle_t));
    if(!h) {
        return NULL;
    }
    
    r = port_init(h, port, cb);
    if(r) {
        free(h);
        return NULL;
    }
    
    return h;
}


int com_deinit(handle_t *h)
{
    com_handle_t **ch=(com_handle_t**)h;
    if(!*ch) {
        return -1;
    }
    
    free(*ch);
    
    return 0;
}


static int cmd_proc(handle_t h, void *addr, void *data)
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
int com_proc(handle_t h, void *addr, void *data, U16 len)
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
            //pkt_ack_reset(ack->type);
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
        send_ack(p->type, err);
    }
    else {
        if (err) {
            //send_err(p->type, err);
        }
    }
    
    return err;
    
}
#else
static void jumpto_app(handle_t *h)
{
    com_deinit(h);
    board_deinit();
    jump_to_app();
}

static U8 _normal_proc(com_handle_t *h, void *addr, pkt_hdr_t *p)
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
            err = cmd_proc(h, addr, p->data);
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
        send_ack(h, addr, p->type, err);
    }
    else {
        if (err) {
            send_err(h, addr, p->type, err);
        }
    }
    
    if(upgrade_flag==1) {
        paras_set_upg();
        reboot();
    }
    
    return err;
}



static U8 _upgrade_proc(com_handle_t *h, void *addr, pkt_hdr_t *p)
{
    int r;
    U8  err=0;
    handle_t *hh;
    *hh = (handle_t)h;
    
    switch (p->type) {
        case TYPE_STAT:
        {
            curStat.sysState = sysState;
            r = send_data(h->h, addr, TYPE_STAT, 0, &curStat, sizeof(curStat));
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

            //pkt_ack_reset(h->h, addr, ack->type);
        }
        break;
        
        case TYPE_CMD:
        {
            err = cmd_proc(h->h, addr, p->data);
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
        send_ack(h, addr, p->type, err);
    }
    else {
        if (err) {
            send_err(h, addr, p->type, err);
        }
    }
    
    if(upgrade_is_finished()) {
        if(upgrade_is_app()) {
            jumpto_app(hh);
        }
        else {
            paras_clr_upg();
            reboot();
        }
    }
    
    return err;
}


int com_proc(handle_t h, void *addr, void *data, U16 len)
{
    int r;
    int err=0;
    node_t nd;
    com_handle_t *ch=(com_handle_t*)h;
    pkt_hdr_t *p=(pkt_hdr_t*)data;

    err = pkt_check_hdr(p, len, sizeof(ch->rxBuf));
    if (err == ERROR_NONE) {
        if(sysState==STAT_UPGRADE) {
            err = _upgrade_proc(ch, addr, p);
        }
        else {
            err = _normal_proc(ch, addr, p);
        }
    } 
    
    return err;
}
#endif


int com_send_paras(handle_t h, void *addr, U8 flag)
{
    int r;
    U8  err=0;
    com_handle_t *ch=(com_handle_t*)h;
    
    if (flag == 0) {      //ask paras
        r = send_data(ch->h, addr, TYPE_PARA, 0, NULL, 0);
        if (r) {
            err = ERROR_UART2_COM;
        }
    }
    else {
        r = send_data(ch->h, addr, TYPE_PARA, 1, &curPara, sizeof(curPara));
        if (r ) {
            err = ERROR_UART2_COM;
        }
    }
    
    return err;
}


int com_send_data(handle_t h, void *addr, U8 type, U8 nAck, void* data, int len)
{
    int r;
    com_handle_t *ch=(com_handle_t*)h;
    
    r =  send_data(ch->h, addr, type, nAck, data, len);
    return (r == 0) ? 0 : ERROR_UART2_COM;
}


int com_check_timeout(handle_t h, void *addr, U8 type)
{
    com_handle_t *ch=(com_handle_t*)h;
    
    return 0;//pkt_check_timeout(type);
}



