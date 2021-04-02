#include "dev.h"
#include "pkt.h"
#include "data.h"
#include "log.h"

typedef struct {
    U8              askAck;
    U16             retries;
    U16             dataLen;
    union {
        cmd_t       cmd;
        stat_t      stat;
        ack_t       ack;
        sett_t      sett;
        error_t     err;
        upgrade_pkt_t upg;
        paras_t     paras;
        fw_info_t   fwinfo;
    }data;
}cache_t;

#define BUFLEN      1000
static U8 myBuf[BUFLEN];
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
    U8* buf = myBuf;
    pkt_hdr_t* p = (pkt_hdr_t*)buf;
    U32 totalLen = PKT_HDR_LENGTH + len;

    if (totalLen>BUFLEN-1) {
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

    return dev_send(buf, totalLen + 1);
}
static int send_ack(U8 type, U8 error)
{
    U8* buf = myBuf;
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

    return dev_recv(buf, totalLen + 1);
}


int pkt_init(pkt_cfg_t *cfg)
{
    if(!cfg) {
        return -1;
    }
    
    myCfg = *cfg;
    
    return 0;
}


U8 pkt_hdr_check(void *data, U16 len)
{
    U8 sum,err=ERROR_NONE;
    pkt_hdr_t *p=(pkt_hdr_t*)data;
    
    if (p->magic != PKT_MAGIC) {
        err = ERROR_PKT_MAGIC;
    }

    if (p->dataLen + PKT_HDR_LENGTH+1 != len || p->dataLen + PKT_HDR_LENGTH + 1>BUFLEN) {
        err = ERROR_PKT_LENGTH;
    }

    if (err==ERROR_NONE) {
        sum = get_sum(p, p->dataLen + PKT_HDR_LENGTH);
        if (sum != ((U8*)p)[p->dataLen + PKT_HDR_LENGTH]) {
            err = ERROR_PKT_CHECKSUM;
        }
    }
    
    return err;
}


void pkt_cache_reset(void)
{
    memset(myCache, 0, sizeof(myCache));
}


static void cache_fill(void)
{
    pkt_hdr_t *p=(pkt_hdr_t*)myBuf;
    cache_t *c=&myCache[p->type];
    c->askAck = p->askAck;
    c->retries = 0;
    c->dataLen = 0;
    if(p->dataLen<=sizeof(c->data)) {
        memcpy(&c->data, p->data, p->dataLen);
        c->dataLen = p->dataLen;
    }
}


void pkt_ack_update(U8 type)
{
    cache_t *sd=&myCache[type];
    
    sd->askAck = 0;
    sd->retries = 0;
}


static U8 ack_timeout(U8 type)
{
    if(myCache[type].askAck && myCache[type].retries<myCfg.retries) {
        return 0;
    }
    
    return 1;
}


int pkt_ack_check(U8 type)
{
    cache_t *c=&myCache[type];
    
    if(c->askAck) {
        if (c->retries < myCfg.retries) {
            send_pkt(type, 1, &c->data, c->dataLen);
            c->retries++;
        }
        else {
            return 1;
        }
    }

    return 0;
}


int pkt_send(U8 type, U8 nAck, void* data, U16 len)
{
    int r=0;
    
    if(ack_timeout(type)) {
        r = send_pkt(type, nAck, data, len);
        if(r==0) {
            cache_fill();
        }
    }
    
    return r;
}


int pkt_send_ack(U8 type, U8 error)
{
    return send_ack(type, error);
}






