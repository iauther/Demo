#include "protocol.h"
#include "board.h"
#include "upgrade.h"
#include "dal_uart.h"
#include "dal_eth.h"
#include "paras.h"
#include "dal.h"
#include "net.h"
#include "comm.h"
#include "cfg.h"

#ifdef _WIN32
#include "win.h"
#endif


#define ERROR_OF(port) ((port)==PORT_UART)?ERROR_COMM_UART:(((port)==PORT_USB)?ERROR_COMM_USB:ERROR_COMM_ETH)


static int port_init(comm_handle_t *h, U8 port, rx_cb_t callback)
{
    int r;
    h->port = port;
    
#ifndef _WIN32
    switch(port) {
        
        case PORT_UART:
        {
            dal_uart_cfg_t cfg;
            
            cfg.mode = MODE_DMA;
            cfg.port = COM_UART;               //PA2: TX   PA3: RX
            cfg.baudrate = COM_BAUDRATE;
            cfg.para.callback = callback;
            cfg.para.buf = h->rxBuf;
            cfg.para.blen = sizeof(h->rxBuf);
            cfg.para.dlen = 0;
            h->h = dal_uart_init(&cfg);
            if(!h->h) {
                return -1;
            }
            h->chkID = CHK_SUM;
        }
        break;
        
        case PORT_NET:
        {
            net_cfg_t cfg;
            cfg.callback = callback;
            h->h = net_init(&cfg);
            if(!h->h) {
                return -1;
            }
            h->chkID = CHK_NONE;
        }
        break;
        
        case PORT_USB:
        {
            
            //h->chkID = CHK_NONE;
        }
        break;
        
        default:
            return -1;
    }
#endif
    
    return 0;
}


static int port_deinit(comm_handle_t *h)
{
    int r=0;
    
#ifndef _WIN32
    switch(h->port) {
        
        case PORT_UART:
        {
            r = dal_uart_deinit(h->h);
        }
        break;
        
        case PORT_NET:
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



static int port_send(comm_handle_t *h, void *addr, void *data, int len)
{
    int r=0;
    
#ifdef _WIN32
    return port_send(buf, totalLen + 1);
#else
    switch(h->port) {
        case PORT_UART:
        r = dal_uart_write(h->h, data, len);
        break;
        
        case PORT_NET:
        r = net_write(h->h, addr, data, len);
        break;
    }
#endif
    
    return r;
}

static int send_data(comm_handle_t *h, void *addr, U8 type, U8 nAck, void *data, int dlen, int chkID)
{
    int len;
    
    len = pkt_pack_data(type, nAck, data, dlen, h->txBuf, sizeof(h->txBuf), chkID);
    if(len<=0) {
        return len;
    }
    
    return port_send(h, addr, h->txBuf, len);
}
static int send_ack(comm_handle_t *h, void *addr, U8 type, U8 err, int chkID)
{
    int len;
    
    len = pkt_pack_ack(type, err, h->txBuf, sizeof(h->txBuf), chkID);
    if(len<=0) {
        return -1;
    }
    
    return port_send(h, addr, h->txBuf, len);
}
static int send_err(comm_handle_t *h, void *addr, U8 type, U8 err, int chkID)
{
    int len;
    
    len = pkt_pack_err(type, err, h->txBuf, sizeof(h->txBuf), chkID);
    if(len<=0) {
        return -1;
    }
    
    return port_send(h, addr, h->txBuf, len);
}
//////////////////////////////////////////////////////////////

handle_t comm_init(U8 port, rx_cb_t cb)
{
    int r;
    comm_handle_t *h=calloc(1, sizeof(comm_handle_t));
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


int comm_deinit(handle_t h)
{
    comm_handle_t *ch=(comm_handle_t*)h;
    if(!ch) {
        return -1;
    }
    
    free(ch);
    
    return 0;
}


static int cmd_proc(handle_t h, void *addr, void *data)
{
    int r;
    U8  err=0;
    command_t *c=(command_t*)data;
    
    switch(c->cmd) {

        case 0xff:
        {
#ifndef _WIN32
            dal_reboot();
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
int comm_recv_proc(handle_t h, void *addr, void *data, U16 len)
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
        
        case TYPE_ACK:
        {
            ack_t* ack = (ack_t*)p->data;
            
            LOG("___ type_ack, type: %d\n", ack->type);
            //pkt_ack_reset(ack->type);
        }
        break;

        case TYPE_HBEAT:
        {
            LOG("___ TYPE_HBEAT\n");
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

static U8 _normal_proc(comm_handle_t *h, void *addr, pkt_hdr_t *p, int chkID)
{
    int r;
    U8  err=0;
    U8 upgrade_flag=0;
    
    switch (p->type) {
        case TYPE_SETT:
        {
            
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
        
        case TYPE_HBEAT:
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
        send_ack(h, addr, p->type, err, chkID);
    }
    else {
        if (err) {
            send_err(h, addr, p->type, err, chkID);
        }
    }
    
    if(upgrade_flag==1) {
        dal_reboot();
    }
    
    return err;
}



static U8 _upgrade_proc(comm_handle_t *h, void *addr, pkt_hdr_t *p, int chkID)
{
    int r;
    U8  err=0;
    
    switch (p->type) {
        case TYPE_STAT:
        {
            
        }
        break;
        
        case TYPE_FILE:
        {
            //err = upgrade_proc(p->data);  
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

        default:
        {
            err = ERROR_PKT_TYPE;
        }
        break;
    }
    
    if (p->askAck) {
        send_ack(h, addr, p->type, err, chkID);
    }
    else {
        if (err) {
            send_err(h, addr, p->type, err, chkID);
        }
    }
    
    
    
    return err;
}


int comm_recv_proc(handle_t h, void *addr, void *data, int len)
{
    int r;
    int err=0;
    node_t nd;
    comm_handle_t *ch=(comm_handle_t*)h;
    pkt_hdr_t *p=(pkt_hdr_t*)data;

    err = pkt_check_hdr(p, len, len, ch->chkID);
    if (err == ERROR_NONE) {
        err = _normal_proc(ch, addr, p, ch->chkID);
    } 
    
    return err;
}
#endif


int com_send_paras(handle_t h, void *addr, U8 flag)
{
    int r;
    U8  err=0;
    comm_handle_t *ch=(comm_handle_t*)h;
    
    if (flag == 0) {      //ask paras
        r = send_data(ch, addr, TYPE_PARA, 0, NULL, 0, ch->chkID);
    }
    else {
        r = send_data(ch, addr, TYPE_PARA, 1, &allPara, sizeof(allPara), ch->chkID);
    }
    
    if (r ) {
        err = ERROR_OF(ch->port);
    }
    
    return err;
}


int comm_send_data(handle_t h, void *addr, U8 type, U8 nAck, void* data, int len)
{
    int r;
    comm_handle_t *ch=(comm_handle_t*)h;
    
    r =  send_data(ch, addr, type, nAck, data, len, ch->chkID);
    return (r == 0) ? 0 : ERROR_OF(ch->port);
}


int comm_pure_send(handle_t h, void *addr, void* data, int len)
{
    comm_handle_t *ch=(comm_handle_t*)h;
    return port_send(ch, addr, data, len);
}


int comm_check_timeout(handle_t h, void *addr, U8 type)
{
    comm_handle_t *ch=(comm_handle_t*)h;
    
    return 0;//pkt_check_timeout(type);
}



