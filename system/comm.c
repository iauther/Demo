#include "comm.h"
#include "cfg.h"
#include "pkt.h"
#include "paras.h"
#include "upgrade.h"
#include "log.h"
#include "net.h"
#include "mem.h"


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

typedef struct {
    U8          port;
    handle_t    handle;
    handle_t    hconn;
    
    buf_t       rx;
    buf_t       tx;
    
    int         chkID;
    
#ifdef _WIN32
    mySock      mSock;
    mySerial    mSerial;
#endif
}comm_handle_t;



static void buf_init(buf_t *b, int len)
{
    if(b->buf) {
        xFree(b->buf);
    }
    
    b->blen = len;
    b->buf  = (U8*)eMalloc(b->blen);
    b->dlen = 0;
}

static int port_read(comm_handle_t *h, void *para, void *data, int len)
{
    int r=0;

    switch(h->port) {
        case PORT_UART:
        {
#ifdef _WIN32
            r = h->mSerial.read(data, len);
#else
            r = dal_uart_write(h->handle, data, len);
#endif
        }
        break;
        
        case PORT_NET:
        {
            r = net_read(h->hconn, para, data, len);
        }
        break;

        case PORT_USB:
        {
#ifdef _WIN32

#else
            //r = usb_write(h->handle, data, len);
#endif
        }
        break;

        default:
            return -1;
    }
    
    return r;
}


static int port_write(comm_handle_t *h, void *para, void *data, int len)
{
    int r=0;

    switch(h->port) {
        case PORT_UART:
        {
#ifdef _WIN32
            r = h->mSerial.write(data, len);
#else
            r = dal_uart_write(h->handle, data, len);
#endif
        }
        break;
        
        case PORT_NET:
        {
            r = net_write(h->hconn, para, data, len);
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
    comm_handle_t *ch=(comm_handle_t*)h;
    
    if(ch->port==PORT_NET && !ch->hconn) {
        LOGE("___ not connected\n");
        return -1;
    }
    
    if(dlen>ch->tx.blen) {
        buf_init(&ch->tx, dlen);
    }
    
    len = pkt_pack_data(type, nAck, data, dlen, ch->tx.buf, ch->tx.blen, ch->chkID);
    if(len<=0) {
        return len;
    }
    
    return port_write(ch, para, ch->tx.buf, len);
}
static int send_ack(handle_t h, void *para, U8 type, U8 err)
{
    int len;
    comm_handle_t *ch=(comm_handle_t*)h;
    
    len = pkt_pack_ack(type, err, ch->tx.buf, ch->tx.blen, ch->chkID);
    if(len<=0) {
        return -1;
    }
    
    return port_write(ch, para, ch->tx.buf, len);
}

//////////////////////////////////////////////////////////////

handle_t comm_open(comm_para_t *p)
{
    comm_handle_t *h=(comm_handle_t*)calloc(1,sizeof(comm_handle_t));
    if(!h) {
        return NULL;
    }

    h->port = p->port;
    if(p->rlen>0) {
        buf_init(&h->rx, p->rlen);
    }
    
    if(p->tlen>0) {
        buf_init(&h->tx, p->tlen);
    }
    
    switch(h->port) {
        
        case PORT_UART:
        {
#ifdef _WIN32
            h->mSerial.open((char *)p->para);
#else
            
            h->handle = log_get_handle();
            h->chkID = CHK_SUM;
#endif
        }
        break;
        
        case PORT_NET:
        {
            if(!h->handle) {
                h->handle = net_init(NULL);
            }
            
            if(h->handle) {
                h->hconn = net_conn(h->handle, (conn_para_t*)p->para);
                if(!h->hconn) {
                    net_deinit(h->handle);
                    return NULL;
                }
            }
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
        {
            free(h);
            return NULL;
        }
    }
    
    return h;
}

int comm_close(handle_t h)
{
    int r=-1;
    comm_handle_t* ch=(comm_handle_t*)h;
    
    if(!ch) {
        return -1;
    }
    
    switch(ch->port) {
    
        case PORT_UART:
        {
#ifdef _WIN32
            r = ch->mSerial.close();
#else
            
#endif
        }
        break;

        case PORT_NET:
        {
            if(ch->hconn) {
                r = net_disconn(ch->hconn);
            }
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


//////////////////////////////////////////
int comm_recv_proc(handle_t h, void *para, void *data, int len)
{
    int r;
    int err=0;
    pkt_hdr_t *hdr=(pkt_hdr_t*)data;
    comm_handle_t *ch=(comm_handle_t*)h;
    mqtt_pub_para_t pub_para={DATA_SETT};
    
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
                    
    #ifdef OS_KERNEL
                    //need save the para
                    task_trig(TASK_NVM, EVT_SAVE);
    #endif
                }
                
                //send para back to update
                err = send_data(h, &pub_para, TYPE_PARA, 1, p_all_para, sizeof(all_para_t));
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
                ch_para_t *para=paras_get_ch_para(CH_0);
                int points = (para->smpFreq/1000)*SAMPLE_INT_INTERVAL;
                extern dac_param_t dac_param;
                dac_para_t* dac = (dac_para_t*)hdr->data;
                
                dac_param.enable = dac->enable;
                dac_param.fdiv   = dac->fdiv;
                dac_param.buf.blen = points*sizeof(U16);
                dac_param.buf.buf  = eMalloc(dac_param.buf.blen);
                dac_param.buf.dlen = 0;
                dac_set(&dac_param);
    #endif
            }
            break;
            
            
            case TYPE_CAP:
            {
    #ifndef _WIN32
                capture_t *cap=(capture_t*)hdr->data;
         #ifdef OS_KERNEL
                if(cap->enable) {
                    api_cap_start(cap->ch);
                    
                }
                else {
                    api_cap_stop(cap->ch);
                }
         #endif
    #endif
            }
            break;
            
            
            case TYPE_CALI:
            {
                cali_sig_t *sig= (cali_sig_t*)hdr->data;
    #ifndef _WIN32
                paras_set_cali_sig(sig->ch, sig);
                
           #ifdef OS_KERNEL
                api_cap_start(sig->ch);
           #endif
    #endif
            }
            break;
            
            case TYPE_DATO:
            {
                //U8 *dato=&p_all_para->usr.smp.dato;
                //*dato = ((*dato)==DATO_ALI)?DATO_USR:DATO_ALI;
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
            send_ack(h, &pub_para, hdr->type, err);
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
    comm_handle_t *ch=(comm_handle_t*)h;
    
    if(!ch) {
        return -1;
    }

    if (ch->port == PORT_NET && !ch->hconn) {
        LOGE("___ pure send, not connected\n");
        return -1;
    }
    
    return port_write(ch, para, data, len);
}


int comm_recv_data(handle_t h, void *para, void* data, int len)
{
    comm_handle_t *ch=(comm_handle_t*)h;
    if (!ch) {
        return -1;
    }
    
    return port_read(ch, para, data, len);
}


