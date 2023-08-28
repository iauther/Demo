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
#define ERROR_OF(port) ((port)==PORT_UART)?ERROR_COMM_UART:(((port)==PORT_USB)?ERROR_COMM_USB:ERROR_COMM_ETH)


static int port_init(comm_handle_t *h, U8 port, rx_cb_t callback)
{
    int r;
    h->port = port;
    

    switch(port) {
        
        case PORT_UART:
        {
#ifdef _WIN32
            int n;
            sp_info_t* spInfo;
            
            n = h->mSerial.list(&spInfo);
            h->mSerial.open(1);
#else
            dal_uart_cfg_t cfg;
            
            cfg.mode = MODE_IT;
            cfg.port = COM_UART;
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
#endif

        }
        break;
        
        case PORT_NET:
        {
#ifdef _WIN32
            h->mSock.conn((char*)"192.168.1.9", 7788);
#else
            net_cfg_t cfg;
            cfg.callback = callback;
            h->h = net_init(&cfg);
            if(!h->h) {
                return -1;
            }
            h->chkID = CHK_NONE;
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
            return -1;
    }
    
    return 0;
}


static int port_deinit(comm_handle_t *h)
{
    int r=0;
    
    switch(h->port) {
        
        case PORT_UART:
        {
#ifdef _WIN32
            r = h->mSerial.close();
#else
            r = dal_uart_deinit(h->h);
#endif
        }
        break;
        
        case PORT_NET:
        {
#ifdef _WIN32
            r = h->mSock.disconn();
#else
            r = net_deinit(h->h);
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


static int port_read(comm_handle_t *h, void *data, int len)
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
            r = h->mSock.read(data, len);
#else
            //r = net_read(h->h, addr, data, len);
#endif
        }
        break;

        case PORT_USB:
        {
#ifdef _WIN32

#else
            //r = net_write(h->h, addr, data, len);
#endif
        }
        break;

        default:
            return -1;
    }
    
    return r;
}


static int port_write(comm_handle_t *h, void *addr, void *data, int len)
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
            r = h->mSock.write(data, len);
#else
            r = net_write(h->h, addr, data, len);
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

static int send_data(comm_handle_t *h, void *addr, U8 type, U8 nAck, void *data, int dlen, int chkID)
{
    int len;
    
    len = pkt_pack_data(type, nAck, data, dlen, h->txBuf, sizeof(h->txBuf), chkID);
    if(len<=0) {
        return len;
    }
    
    return port_write(h, addr, h->txBuf, len);
}
static int send_ack(comm_handle_t *h, void *addr, U8 type, U8 err, int chkID)
{
    int len;
    
    len = pkt_pack_ack(type, err, h->txBuf, sizeof(h->txBuf), chkID);
    if(len<=0) {
        return -1;
    }
    
    return port_write(h, addr, h->txBuf, len);
}
static int send_err(comm_handle_t *h, void *addr, U8 type, U8 err, int chkID)
{
    int len;
    
    len = pkt_pack_err(type, err, h->txBuf, sizeof(h->txBuf), chkID);
    if(len<=0) {
        return -1;
    }
    
    return port_write(h, addr, h->txBuf, len);
}
//////////////////////////////////////////////////////////////

handle_t comm_init(U8 port, rx_cb_t cb)
{
    int r;
    comm_handle_t *h=(comm_handle_t*)calloc(1, sizeof(comm_handle_t));
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



//////////////////////////////////////////
int comm_recv_proc(handle_t h, void *addr, void *data, int len)
{
    int r;
    int err=0;
    node_t nd;
    comm_handle_t *hcom=(comm_handle_t*)h;
    pkt_hdr_t *hdr=(pkt_hdr_t*)data;

    err = pkt_check_hdr(hdr, len, len, hcom->chkID);
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
            adc_para_t* pr = &pa->usr.ch[cd->ch];
            F32* ev, *s = (F32*)(cd->data + cd->dlen);
            
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
                api_cap_start();
                paras_set_state(STAT_CAP);
            }
            else {
                api_cap_stop();
                paras_set_state(STAT_STOP);
            }
#endif
            
            err = 0;
        }
        break;
        
        
        case TYPE_CALI:
        {
            cali_t *ca= (cali_t*)hdr->data;
#ifdef _WIN32    


#else
            paras_set_cali(ca->ch, ca);
            api_cap_start();
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
        send_ack(hcom, addr, hdr->type, err, hcom->chkID);
    }
    else {
        if (err) {
            send_err(hcom, addr, hdr->type, err, hcom->chkID);
        }
    }
    } 
    
    return err;
}


int com_send_paras(handle_t h, void *addr, U8 flag)
{
    int r;
    U8  err=0;
    comm_handle_t *ch=(comm_handle_t*)h;
    
    if (flag == 0) {      //ask paras
        r = send_data(ch, addr, TYPE_PARA, 0, NULL, 0, ch->chkID);
    }
    else {
        r = send_data(ch, addr, TYPE_PARA, 1, p_all_para, sizeof(allPara), ch->chkID);
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
    return port_write(ch, addr, data, len);
}


int comm_check_timeout(handle_t h, void *addr, U8 type)
{
    comm_handle_t *ch=(comm_handle_t*)h;
    
    return 0;//pkt_check_timeout(type);
}


int comm_recv_data(handle_t h, void* data, int len)
{
    comm_handle_t *ch=(comm_handle_t*)h;
    
    return port_read(ch, data, len);
}

