#include "pkt.h"
#include "paras.h"
#include "data.h"
#include "log.h"

#ifdef _WIN32
#include "port.h"
#else
#include "drv/uart.h"
#endif

typedef struct {
    int             askAck;
    int             cnt;
    int             retries;
    U16             dataLen;
    union {
        cmd_t       cmd;
        stat_t      stat;
        ack_t       ack;
        sett_t      sett;
        error_t     err;
        para_t      para;
        fw_info_t   fwinfo;
    }data;
}cache_t;



U8 pkt_rx_buf[PKT_BUFLEN];
U8 pkt_tx_buf[PKT_BUFLEN];
static pkt_cfg_t myCfg;
static cache_t myCache[TYPE_MAX]={0};



static U8 get_sum(void* data, U16 len)
{
    U8 sum = 0;
    U8 *buf=(U8*)data;
    
    for (int i = 0; i < len; i++) {
        sum += buf[i];
    }
    return sum;
}
static int send_pkt(U8 type, U8 nAck, void* data, U16 len)
{
    U8* buf = pkt_tx_buf;
    pkt_hdr_t* p = (pkt_hdr_t*)buf;
    U32 totalLen = PKT_HDR_LENGTH + len;

    if (totalLen>PKT_BUFLEN-1) {
        //log("_____ send_pkt len is wrong, %d, max: %d\n", totalLen+1, BUFLEN);
        return -1;
    }

    p->magic = PKT_MAGIC;
    p->type = type;
    p->flag = 0;
    p->askAck = nAck;
    p->dataLen = len;
    memcpy(p->data, data, len);
    buf[totalLen] = get_sum(buf, totalLen);

#ifdef _WIN32
    return port_send(buf, totalLen + 1);
#else
    return uart_write(myCfg.handle, buf, totalLen + 1);
#endif
}
static int send_ack(U8 type, U8 error)
{
    U8* buf = pkt_tx_buf;
    pkt_hdr_t* p = (pkt_hdr_t*)buf;
    ack_t* ack = (ack_t*)p->data;
    U32 totalLen = PKT_HDR_LENGTH + sizeof(ack_t);

    p->magic = PKT_MAGIC;
    p->type = TYPE_ACK;
    p->flag = 0;
    p->askAck = 0;
    p->dataLen = sizeof(ack_t);

    ack->type = type;
    ack->error = error;
    buf[totalLen] = get_sum(buf, totalLen);
#ifdef _WIN32
    return port_send(buf, totalLen + 1);
#else
    return uart_write(myCfg.handle, buf, totalLen + 1);
#endif
}
static int send_err(U8 type, U8 error)
{
    U8* buf = pkt_tx_buf;
    pkt_hdr_t* p = (pkt_hdr_t*)buf;
    error_t* err = (error_t*)p->data;
    U32 totalLen = PKT_HDR_LENGTH + sizeof(error_t);

    p->magic = PKT_MAGIC;
    p->type = TYPE_ERROR;
    p->flag = 0;
    p->askAck = 0;
    p->dataLen = sizeof(error_t);

    err->type = type;
    err->error = error;
    buf[totalLen] = get_sum(buf, totalLen);
#ifdef _WIN32
    return port_send(buf, totalLen + 1);
#else
    return uart_write(myCfg.handle, buf, totalLen + 1);
#endif
}




int pkt_init(pkt_cfg_t *cfg)
{
    int i;
    if(!cfg) {
        return -1;
    }
    
    myCfg = *cfg;
    for (i = 0; i < TYPE_MAX; i++) {
        myCache[i].askAck = -1;
    }
    
    return 0;
}


U8 pkt_hdr_check(void *data, U16 len)
{
    U8 sum1,sum2,err=ERROR_NONE;
    U8 *p8=(U8*)data;
    pkt_hdr_t *p=(pkt_hdr_t*)data;
    
    if (p->magic != PKT_MAGIC) {
        err = ERROR_PKT_MAGIC;
    }

#if 0
    if (p->dataLen + PKT_HDR_LENGTH+1 != len || p->dataLen + PKT_HDR_LENGTH + 1>PKT_BUFLEN) {
        #ifdef _WIN32
        LOG("____dataLen: %d, PKT_HDR_LEN: %d, len: %d\n", p->dataLen, PKT_HDR_LENGTH, len);
        #endif
        err = ERROR_PKT_LENGTH;
    }
#endif

    if (err==ERROR_NONE) {
        sum1 = p8[p->dataLen + PKT_HDR_LENGTH];
        sum2 = get_sum(p, p->dataLen + PKT_HDR_LENGTH);
        if (sum1 != sum2) {
            #ifdef _WIN32
            LOG("____checksum: 0x%02x, p8 0x%02x\n", sum1, sum2);
            #endif
            err = ERROR_PKT_CHECKSUM;
        }
    }
    
    return err;
}


void pkt_cache_reset(void)
{
    memset(myCache, 0, sizeof(myCache));
}


static void cache_update(void)
{
    pkt_hdr_t *p=(pkt_hdr_t*)pkt_tx_buf;
    cache_t *c=&myCache[p->type];
    c->askAck = p->askAck;
    c->cnt = 0;
    c->retries = 0;
    c->dataLen = 0;
    if(p->dataLen<=sizeof(c->data)) {
        memcpy(&c->data, p->data, p->dataLen);
        c->dataLen = p->dataLen;
    }
}


int pkt_ack_reset(U8 type)
{
    cache_t *c;
    
    if(type>=TYPE_MAX) {
        return -1;
    }
    
    c = &myCache[type];
    c->askAck = -1;
    c->cnt = 0;
    c->retries = 0;
    
    return 0;
}


static int need_wait_ack(U8 type)
{
    int r=0;

    if (myCache[type].askAck > 0) {
        if (ackTimeout.set[type].enable) {
            if (myCache[type].retries < ackTimeout.set[type].retryTimes) {
                r = 1;
            }
        }
    }
    
    return r;
}


int pkt_check_timeout(U8 type)
{
    int r=0;
    U8 send=0;
    cache_t *c=&myCache[type];
    
    if(c->askAck>0) {
        if(ackTimeout.set[type].enable) {
            if((c->cnt*myCfg.period) % ackTimeout.set[type].resendIvl==0) {

                if (ackTimeout.set[type].retryTimes > 0) {
                    if (c->retries < ackTimeout.set[type].retryTimes) {
                        c->retries++;
                    }
                    else {
                        r = 1;
                    }
                }
                else {
                    r = 1;
                }
            }
            c->cnt++;
        }
    }  

    return r;
}


int pkt_send(U8 type, U8 nAck, void* data, U16 len)
{
    int r=0;
    
    if (sysState == STAT_UPGRADE) {
        r = send_pkt(type, nAck, data, len);
    }
    else {
        if (!need_wait_ack(type)) {
            r = send_pkt(type, nAck, data, len);
            if (r == 0) {
                cache_update();
            }
        }
    }
    
    return r;
}


int pkt_resend(U8 type)
{
    cache_t* c=NULL;

    if (type>=TYPE_MAX) {
        return 1;
    }

    c = &myCache[type];
    return send_pkt(type, 1, &c->data, c->dataLen);
}


int pkt_send_ack(U8 type, U8 error)
{
    return send_ack(type, error);
}


int pkt_send_err(U8 type, U8 error)
{
    return send_err(type, error);
}







