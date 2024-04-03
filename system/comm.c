#include "comm.h"
#include "cfg.h"
#include "pkt.h"
#include "log.h"
#include "net.h"
#include "mem.h"
#include "cmd.h"
#include "rtc.h"
#include "paras.h"
#include "upgrade.h"

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
    
    b->blen = len+sizeof(pkt_hdr_t)+32;
    b->buf  = (U8*)eMalloc(b->blen);
    if(!b->buf) {
        b->blen = 0;
    }
    b->dlen = 0;
}

static int port_read(comm_handle_t *h, void *para, void *data, int len)
{
    int r=0;

    switch(h->port) {
        case PORT_LOG:
        {
#ifdef _WIN32
            r = h->mSerial.read(data, len);
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
        case PORT_LOG:
        {
#ifdef _WIN32
            r = h->mSerial.write(data, len);
#else
            r = log_write(data, len);
#endif
        }
        break;
        
        case PORT_485:
        {
#ifndef _WIN32
            r = rs485_write(data, len);
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
            r = -1;
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
    
    if(ch->tx.blen==0) {
        int xlen=PKT_HDR_LENGTH*2+((dlen>0)?dlen:0);
        buf_init(&ch->tx, xlen);
    }
    else {
        if(dlen>ch->tx.blen) {
            buf_init(&ch->tx, dlen);
        }
    }
    
    len = pkt_pack_data(type, nAck, data, dlen, ch->tx.buf, ch->tx.blen, ch->chkID);
    if(len<=0) {
        LOGE("___ pkt_pack_data failed, tx.buf: 0x%x, tx.blen: %d\n", ch->tx.buf, ch->tx.blen);
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
    int r;
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
        
        case PORT_LOG:
        {
#ifdef _WIN32
            r = h->mSerial.open((char *)p->para, 115200);
            if (r) {
                free(h); return NULL;
            }
#else
            h->handle = log_get_handle();
            h->chkID = CHK_SUM;
#endif
        }
        break;
        
        case PORT_485:
        {
#ifndef _WIN32
            rs485_cfg_t *rc=(rs485_cfg_t*)p->para;
            
            r = rs485_init(rc);
            if (r) {
                free(h); return NULL;
            }
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
    
        case PORT_LOG:
        {
#ifdef _WIN32
            r = ch->mSerial.close();
#else
            
#endif
        }
        break;

        case PORT_485:
        {
#ifndef _WIN32
            r = rs485_deinit();
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
static int file_proc(file_hdr_t *hdr)
{
    int r=0;
    
    switch(hdr->ftype) {
        case FILE_PARA:
        {
            
        }
        break;
        
        case FILE_UPG:
        {
            
        }
        break;
    }
    
    return r;
}
static int cali_set(cali_sig_t *sig)
{
    int r=0;
  
#ifdef OS_KERNEL
    LOGD("cali para, ch:%hhu, lv:%hhu, max:%hhu, seq:%hhu, volt:%.1fmv, freq:%uhz, bias:%.1fmv\n",
                     sig->ch, sig->lv, sig->max, sig->seq, sig->volt,   sig->freq,  sig->bias);
    
    r = paras_set_cali_sig(sig->ch, sig);
    if(r==0) {
        LOGD("___ stop cap\n");
        api_cap_stop(sig->ch);
        
        LOGD("___ disconn network\n");
        api_comm_disconnect();
        
        LOGD("___ start cap\n");
        api_cap_start(sig->ch);
        LOGD("___ cali set end\n");
    }
    else {
        LOGE("___ paras_set_cali_sig failed\n");
    }
#endif

    return r;
}
//////////////////////////////////////////
int comm_recv_proc(handle_t h, void *para, void *data, int len)
{
    int r;
    int err=0;
    
#ifndef _WIN32
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
                paras_factory();
                dal_reboot();
            }
            break;
            
            case TYPE_REBOOT:
            {
                dal_reboot();
            }
            break;
            
            case TYPE_FACTORY:
            {
                paras_factory();
                upgrade_info_erase();
                dal_reboot();
            }
            break;

            case TYPE_PARA:
            {
                if(hdr->dataLen>=sizeof(all_para_t) && memcmp(p_all_para, hdr->data, sizeof(all_para_t))) {
                    all_para_t *pa = (all_para_t*)hdr->data;
                    r = paras_check(pa);
                    if(r==0) {
                        p_all_para->usr = pa->usr;
                        p_all_para->usr.takeff = 0;
                        
                        paras_save();
                        //send para back to update
                        err = send_data(h, &pub_para, TYPE_PARA, 1, p_all_para, sizeof(all_para_t));
                        if(pa->usr.takeff) {
                            dal_reboot();
                        }
                    }
                    else {
                        err = ERROR_PKT_PARA;
                    }
                }
                else {
                    err = send_data(h, &pub_para, TYPE_PARA, 1, p_all_para, sizeof(all_para_t));
                }
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
            
            case TYPE_FILE:
            {
                file_hdr_t* fhdr = (file_hdr_t*)hdr->data;
                r = file_proc(fhdr);
            }
            break;
            
            case TYPE_RTMIN:
            {
                U32 rtmin=*((U32*)hdr->data);
                paras_set_rtmin(rtmin);
            }
            break;
            
            case TYPE_DAC:
            {
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
            }
            break;
            
            case TYPE_CAP:
            {
                capture_t *cap=(capture_t*)hdr->data;
         #ifdef OS_KERNEL
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
                cali_set(sig);
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
                U8 mode=*((U8*)hdr->data);
                U8 cur_mode=paras_get_mode();
                if(mode!=cur_mode) {
                    paras_set_mode(mode);
                    paras_save();
                    
                    LOGD("___ switch mode to %d, paras saved, reboot now...\n", mode);
                    dal_reboot();       //重启使生效
                }
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
#endif
    
    return err;
}


int comm_cmd_proc(void* data, int len)
{
    int r=0;
    cmd_t cmd;

#ifndef _WIN32
    r = cmd_get((char*)data, &cmd);
    if(r) {
        return -1;
    }
    
    switch(cmd.ID) {
        case CMD_USER:
        {
            //cmd->str
            
        }
        break;
        
        case CMD_DFLT:
        {
            paras_factory();
            dal_reboot();
        }
        break;
        
        case CMD_BOOT:
        {
            dal_reboot();
        }
        break;
        
        case CMD_CAP:
        {
            U8 flag=0;
            sscanf(cmd.str, "%hhu", &flag);
            
            paras_set_mode(MODE_NORM);
            if(flag>0) {
                api_cap_start_all();
            }
            else {
                api_cap_stop_all();
            }
        }
        break;
        
        case CMD_CALI:
        {
            /*
                u8	                                %d
                s8	                                %d
                u16	                                %d or %hu
                s16	                                %d or %hd
                u32	                                %u
                s32	                                %d
                u64	                                %llu
                s64	                                %lld
                int	                                %d
                unsigned int	                    %u
                short int	                        %d or %hd
                long	                            %ld
                unsigned long	                    %lu
                long long	                        %lld
                unsigned long long	                %llu
                char	                            %c
                char*	                            %s
                bool            	                %d
                unsigned int/int        	        %0x
                unsigned long/long                  %0lx
                long long/unsigned long long        %0llx
                unsigned int/int                    %0o
                unsigned long/long          	    %0lo
                long long/unsigned long long        %0llo
                float	                            %f
                double	                            %f or %lf
                科学技术类型(必须转化为double类型)	%e
                限制输出字段宽度	                    %x.yf (x:整数长度,y:小数点长度)
            */
            cali_sig_t sig;
            LOGD("___ cali para: %s\n", cmd.str);
            
            r = sscanf(cmd.str, "[%hhu,%u,%f,%hhu,%f,%hhu,%hhu]", &sig.ch, &sig.freq, &sig.bias, &sig.lv, &sig.volt, &sig.max, &sig.seq);
            if(r!=7) {
                LOGE("___ cali para cnt is wrong, %d\n", r);
                return -1;
            }
            cali_set(&sig);
        }
        break;
        
        case CMD_DAC:
        {
            dac_param_t param;
            ch_para_t *para = paras_get_ch_para(CH_0);
            
            param.enable = 0;
            param.fdiv = 1;
            
            sscanf(cmd.str, "%d,%d", &param.enable, &param.fdiv);
            LOGD("___ config dac, en: %d, fdiv: %d\n", param.enable, param.fdiv);
            
            int points = (para->smpFreq/1000)*SAMPLE_INT_INTERVAL;
            param.freq = para->smpFreq;
            param.buf.blen = points*sizeof(U16);
            param.buf.buf  = eMalloc(param.buf.blen);
            param.buf.dlen = 0;
            dac_set(&param);
        }
        break;
        
        case CMD_PWR:
        {
            int r;
            U32 countdown=1;
            
            sscanf(cmd.str, "%d", &countdown);
            
            LOGD("___ powerdown now, restart %d seconds later!\n", countdown);
            r = rtc2_set_countdown(countdown);
            LOGD("___ rtc2_set_countdown result: %d\n", r);
        }
        break;
        
        default:
        {
            r = -1;
        }
        break;
    }
#endif

    return 0;
}



int comm_send_data(handle_t h, void *para, U8 type, U8 nAck, void* data, int len)
{
    int r;
    comm_handle_t *ch=(comm_handle_t*)h;
    
    if(!ch) {
        return -1;
    }
    
    r = send_data(ch, para, type, nAck, data, len);
    
    return (r == 0) ? 0 : -1;
}


int comm_pure_send(handle_t h, void *para, void* data, int len)
{
    int r;
    comm_handle_t *ch=(comm_handle_t*)h;
    
    if(!ch) {
        return -1;
    }

    if (ch->port == PORT_NET && !ch->hconn) {
        LOGE("___ pure send, not connected\n");
        return -1;
    }
    
    r = port_write(ch, para, data, len);
    
    return r;
}


int comm_recv_data(handle_t h, void *para, void* data, int len)
{
    int r;
    comm_handle_t *ch=(comm_handle_t*)h;
    
    if (!ch) {
        return -1;
    }
    
    r =  port_read(ch, para, data, len);
    
    return r;
}


int comm_req(handle_t h, int req)
{
    int r=-1;
    comm_handle_t *ch=(comm_handle_t*)h;
    
    if (!ch) {
        return -1;
    }
    
    if (ch->port == PORT_NET && ch->hconn) {
        r =  net_req(ch->hconn, req);
    }
    
    return r;
}


