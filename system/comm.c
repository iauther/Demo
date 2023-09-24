#include "comm.h"
#include "cfg.h"
#include "pkt.h"
#include "paras.h"
#include "protocol.h"
#include "log.h"

#ifdef _WIN32
all_para_t allPara;
#else
#include "task.h"
#include "dal_uart.h"
#include "dal_eth.h"
#include "dal.h"
#include "net.h"
#endif

static all_para_t* p_all_para = &allPara;

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

static int port_init(comm_handle_t *h, void *para, rx_cb_t callback)
{
    if(h->port!=PORT_NET) {
        return 0;
    }

    if (!para) {
        return -1;
    }

#ifndef _WIN32
    net_cfg_t *cfg=para;
    int r = net_init(cfg);
    if(r) {
        return -1;
    }
    h->chkID = CHK_NONE;
#endif
    
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
            int idx = *(int*)para;
            h->mSerial.open(idx);
#else
            
            h->h = log_get_handle();
            h->chkID = CHK_SUM;
#endif
        }
        break;
        
        case PORT_NET:
        {
#ifdef _WIN32
            net_para_t* np = (net_para_t*)para;
            x = h->mSock.conn(np->para.tcp.ip, np->para.tcp.port);
#else
            x = net_conn(para);
#endif
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
#ifdef _WIN32
            r = h->mSerial.close();
#else
            r = net_disconn(conn);
#endif
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
#ifdef _WIN32
            r = h->mSock.read(conn, data, len);
#else
            r = net_read(conn, para, data, len);
#endif
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
#ifdef _WIN32
            r = h->mSock.write(conn, data, len);
#else
            r = net_write(conn, para, data, len);
#endif
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
static int send_err(handle_t h, void *para, U8 type, U8 err)
{
    int len;
    comm_handle_t *ch=NULL;
    
    ch = get_handle(h);
    if(!ch) {
        return -1;
    }
    
    len = pkt_pack_err(type, err, ch->txBuf, sizeof(ch->txBuf), ch->chkID);
    if(len<=0) {
        return -1;
    }
    
    return port_write(ch, h, para, ch->txBuf, len);
}
//////////////////////////////////////////////////////////////

handle_t comm_init(U8 port, void *para, rx_cb_t cb)
{
    int r;
    comm_handle_t *h=(comm_handle_t*)calloc(1, sizeof(comm_handle_t));
    if(!h) {
        return NULL;
    }
    
    h->port = port;
    
    r = port_init(h, para, cb);
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


handle_t comm_open(comm_handle_t *h, void *para)
{
    if(!h) {
        return NULL;
    }
    
    return port_open(h, para);
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
    node_t nd;
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

        case TYPE_PARA:
        {
            all_para_t* pa = (all_para_t*)hdr->data;
#ifdef _WIN32
            *p_all_para = *pa;
#else

#endif
            //pkt_ack_reset(ack->type);
        }
        break;

        
        case TYPE_ACK:
        {
            ack_t* ack = (ack_t*)hdr->data;

            //pkt_ack_reset(ack->type);
        }
        break;        
        
        case TYPE_CAP:
        {
            
#ifdef _WIN32
            all_para_t* pa = p_all_para;
            ch_data_t *cd=(ch_data_t*)hdr->data;
            ch_para_t* pr = &pa->usr.ch[cd->ch];
            F32* ev, *s = (F32*)(cd->data + cd->wavlen);
            
            int cnt = cd->evlen / (4 * pr->n_ev);
            for (int i = 0; i < cnt; i++) {
                ev = s + i * pr->n_ev;
                LOGD("ev0: %f\n", ev[0]);
                LOGD("ev1: %f\n", ev[1]);
                LOGD("ev2: %f\n", ev[2]);
                LOGD("ev3: %f\n", ev[3]);
            }
#else
            capture_t *cap=(capture_t*)hdr->data;
            if(cap->enable) {
                api_cap_start(CH_0);
                paras_set_state(STAT_CAP);
            }
            else {
                api_cap_stop(CH_0);
                paras_set_state(STAT_STOP);
            }
#endif
            
            err = 0;
        }
        break;
        
        
        case TYPE_CALI:
        {
            cali_sig_t *sig= (cali_sig_t*)hdr->data;
#ifdef _WIN32    


#else
            paras_set_cali_sig(sig->ch, sig);
            api_cap_start(CH_0);
            paras_set_state(STAT_CALI);
#endif
            err = 0;
        }
        break;
        
        case TYPE_STAT:
        {
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
    
    if (hdr->askAck) {
        send_ack(h, para, hdr->type, err);
    }
    else {
        if (err) {
            send_err(h, para, hdr->type, err);
        }
    }
    } 
    
    return err;
}


int com_send_paras(handle_t h, void *para, U8 flag)
{
    int r;
    U8  err=0;
    
    if (flag == 0) {      //ask paras
        r = send_data(h, para, TYPE_PARA, 0, NULL, 0);
    }
    else {
        r = send_data(h, para, TYPE_PARA, 1, p_all_para, sizeof(allPara));
    }
    
    if (r ) {
        err = 0;//ERROR_OF(ch->port);
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
    comm_handle_t *ch=(comm_handle_t*)h;
    return port_write(ch, h, para, data, len);
}


int comm_check_timeout(handle_t h, void *addr, U8 type)
{
    comm_handle_t *ch=(comm_handle_t*)h;
    
    return 0;//pkt_check_timeout(type);
}


int comm_recv_data(handle_t h, void *para, void* data, int len)
{
    comm_handle_t *ch=(comm_handle_t*)h;
    
    return port_read(ch, h, para, data, len);
}

