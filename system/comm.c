#include "comm.h"
#include "cfg.h"
#include "pkt.h"
#include "paras.h"
#include "upgrade.h"
#include "log.h"
#include "net.h"


#ifdef _WIN32
static all_para_t* p_all_para = NULL;
#else
#include "task.h"
#include "dal_uart.h"
#include "dal_eth.h"
#include "dal.h"
#include "dac.h"
static all_para_t* p_all_para = &allPara;
#endif


static comm_handle_t* port_handle[PORT_MAX]={NULL};
static comm_handle_t *get_handle(handle_t h)
{
    int i;
    
    for(i=0; i<PORT_MAX; i++) {
        if(h==port_handle[i]) {
            return (comm_handle_t*)h;
        }
    }
    
    return port_handle[PORT_NET];
}

static int port_init(comm_handle_t *h, void *para)
{
    if(h->port!=PORT_NET) {
        return 0;
    }

    net_cfg_t *cfg=(net_cfg_t*)para;
    int r = net_init(cfg);
    if(r) {
        return -1;
    }
    h->chkID = CHK_NONE;
    
    return 0;
        
}

static int port_deinit(comm_handle_t *h)
{
    if(h->port!=PORT_NET) {
        return 0;
    }
    
#ifndef _WIN32
    return net_deinit();
#endif

    return 0;
}


static handle_t port_open(comm_handle_t *h, void *para)
{
    handle_t x=h;

    switch(h->port) {
        
        case PORT_UART:
        {
#ifdef _WIN32
            h->mSerial.open((char *)para);
#else
            
            h->h = log_get_handle();
            h->chkID = CHK_SUM;
#endif
        }
        break;
        
        case PORT_NET:
        {
            x = net_conn((conn_para_t*)para);
        }
        break;
        
        case PORT_USB:
        {
#ifdef _WIN32

#else
            //h->chkID = CHK_NONE;
#endif
            
        }
        break;

        default:
            return NULL;
    }
    port_handle[h->port] = h;
    
    return x;
}


static int port_close(comm_handle_t *h, handle_t conn)
{
    int i,r=-1;
    
    switch(h->port) {
    
        case PORT_UART:
        {
#ifdef _WIN32
            r = h->mSerial.close();
#else
            
#endif
        }
        break;

        case PORT_NET:
        {
            r = net_disconn(conn);
        }
        break;

        case PORT_USB:
        {
#ifdef _WIN32

#else
           
#endif
        }
        break;
    }
    
    return r;
}


static int port_read(comm_handle_t *h, handle_t conn, void *para, void *data, int len)
{
    int r=0;

    switch(h->port) {
        case PORT_UART:
        {
#ifdef _WIN32
            r = h->mSerial.read(data, len);
#else
            r = dal_uart_write(h->h, data, len);
#endif
        }
        break;
        
        case PORT_NET:
        {
            r = net_read(conn, para, data, len);
        }
        break;

        case PORT_USB:
        {
#ifdef _WIN32

#else
            //r = usb_write(h->h, data, len);
#endif
        }
        break;

        default:
            return -1;
    }
    
    return r;
}


static int port_write(comm_handle_t *h, handle_t conn, void *para, void *data, int len)
{
    int r=0;

    switch(h->port) {
        case PORT_UART:
        {
#ifdef _WIN32
            r = h->mSerial.write(data, len);
#else
            r = dal_uart_write(h->h, data, len);
#endif
        }
        break;
        
        case PORT_NET:
        {
            r = net_write(conn, para, data, len);
        }
        break;

        case PORT_USB:
        {
#ifdef _WIN32

#else
            
#endif
        }
        break;

        default:
            return -1;
    }
    
    return r;
}

static int send_data(handle_t h, void *para, U8 type, U8 nAck, void *data, int dlen)
{
    int len;
    comm_handle_t *ch=NULL;
    
    ch = get_handle(h);
    if(!ch) {
        return -1;
    }
    
    len = pkt_pack_data(type, nAck, data, dlen, ch->txBuf, sizeof(ch->txBuf), ch->chkID);
    if(len<=0) {
        return len;
    }
    
    return port_write(ch, h, para, ch->txBuf, len);
}
static int send_ack(handle_t h, void *para, U8 type, U8 err)
{
    int len;
    comm_handle_t *ch=NULL;
    
    ch = get_handle(h);
    if(!ch) {
        return -1;
    }
    
    len = pkt_pack_ack(type, err, ch->txBuf, sizeof(ch->txBuf), ch->chkID);
    if(len<=0) {
        return -1;
    }
    
    return port_write(ch, h, para, ch->txBuf, len);
}

//////////////////////////////////////////////////////////////

handle_t comm_init(U8 port, void *para)
{
    int r;
    comm_handle_t *h=(comm_handle_t*)malloc(sizeof(comm_handle_t));
    if(!h) {
        return NULL;
    }
    
    h->port = port;
    
    r = port_init(h, para);
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


handle_t comm_open(handle_t h, void *para)
{
    comm_handle_t* ch = (comm_handle_t*)h;
    if(!ch) {
        return NULL;
    }
    
    return port_open(ch, para);
}

int comm_close(handle_t conn)
{
    comm_handle_t *h=NULL;
    
    h = get_handle(conn);
    if(!h) {
        return -1;
    }
    
    return port_close(h, conn);
}


//////////////////////////////////////////
int comm_recv_proc(handle_t h, void *para, void *data, int len)
{
    int r;
    int err=0;
    U8 param=DATO_USR;
    pkt_hdr_t *hdr=(pkt_hdr_t*)data;
    comm_handle_t *ch=NULL;
    
    ch = get_handle(h);
    if(!ch) {
        return -1;
    }
    
    err = pkt_check_hdr(hdr, len, len, ch->chkID);
    if (err == ERROR_NONE) {
        switch (hdr->type) {
        case TYPE_SETT:
        {
            
        }
        break;
        
        case TYPE_DFLT:
        {
#ifndef _WIN32
            paras_factory();
            dal_reboot();
#endif
        }
        break;
        
        case TYPE_REBOOT:
        {
#ifndef _WIN32
            dal_reboot();
#endif
        }
        break;
        
        case TYPE_FACTORY:
        {
#ifndef _WIN32
            paras_factory();
            upgrade_info_erase();
            dal_reboot();
#endif
        }
        break;

        case TYPE_PARA:
        {
#ifndef _WIN32
            
            
            if(hdr->dataLen>=sizeof(all_para_t) && memcmp(p_all_para, hdr->data, sizeof(all_para_t))) {
                all_para_t *pa = (all_para_t*)hdr->data;
                
                p_all_para->usr = pa->usr;
                
                //need save the para
                task_trig(TASK_NVM, EVT_SAVE);
            }
            
            //send para back to update
            err = send_data(h, &param, TYPE_PARA, 1, p_all_para, sizeof(all_para_t));
#endif
        }
        break;

        
        case TYPE_ACK:
        {
            ack_t* ack = (ack_t*)hdr->data;
            if(ack->type==TYPE_PARA) {
                //para_send_flag = 1;
            }
        }
        break;
        
        case TYPE_DAC:
        {
#ifndef _WIN32
            extern dac_param_t dac_param;
            dac_para_t* dac = (dac_para_t*)hdr->data;
            
            dac_param.enable = dac->enable;
            dac_param.fdiv   = dac->fdiv;
            dac_set(&dac_param);
#endif
        }
        break;
        
        
        case TYPE_CAP:
        {
#ifndef _WIN32
            capture_t *cap=(capture_t*)hdr->data;
            if(cap->enable) {
                api_cap_start(cap->ch);
                
            }
            else {
                api_cap_stop(cap->ch);
            }
#endif
        }
        break;
        
        
        case TYPE_CALI:
        {
            cali_sig_t *sig= (cali_sig_t*)hdr->data;
#ifndef _WIN32
            paras_set_cali_sig(sig->ch, sig);
            api_cap_start(sig->ch);
#endif
        }
        break;
        
        case TYPE_DATO:
        {
            U8 *dato=&p_all_para->usr.smp.dato;
            *dato = ((*dato)==DATO_ALI)?DATO_USR:DATO_ALI;
        }
        break;
        
        case TYPE_MODE:
        {
#ifndef _WIN32
            U8 mode=*((U8*)hdr->data);
            U8 cur_mode=paras_get_mode();
            if(mode!=cur_mode) {
                paras_set_mode(mode);
                paras_save();
                
                LOGD("___ switch mode to %d, paras saved, reboot now...\n", mode);
                dal_reboot();       //重启使生效
            }
#endif
            err=0;
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
    
    if (hdr->askAck || err) {
        send_ack(h, &param, hdr->type, err);
    }
    } 
    
    return err;
}


int comm_send_data(handle_t h, void *para, U8 type, U8 nAck, void* data, int len)
{
    int r;
    
    r =  send_data(h, para, type, nAck, data, len);
    return (r == 0) ? 0 : -1;
}


int comm_pure_send(handle_t h, void *para, void* data, int len)
{
    comm_handle_t *ch=get_handle(h);
    
    if(!ch) {
        return -1;
    }
    
    return port_write(ch, h, para, data, len);
}


int comm_check_timeout(handle_t h, void *addr, U8 type)
{
    comm_handle_t *ch=(comm_handle_t*)h;
    
    return 0;//pkt_check_timeout(type);
}


int comm_recv_data(handle_t h, void *para, void* data, int len)
{
    comm_handle_t* ch = get_handle(h);

    if (!ch) {
        return -1;
    }
    
    return port_read(ch, h, para, data, len);
}


