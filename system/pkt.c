#include "pkt.h"


static U8 get_sum(void* data, U16 len)
{
    U8 sum = 0;
    U8 *buf=(U8*)data;
    
    for (int i = 0; i < len; i++) {
        sum += buf[i];
    }
    return sum;
}

///////////////////////////////////////////////////////////////////////////////
int pkt_check_hdr(void *data, int dlen, int buflen)
{
    U8 sum1,sum2,err=ERROR_NONE;
    U8 *p8=(U8*)data;
    pkt_hdr_t *p=(pkt_hdr_t*)data;
    
    if (p->magic != PKT_MAGIC) {
        err = ERROR_PKT_MAGIC;
    }
    
    if (p->dataLen + PKT_HDR_LENGTH+1 != dlen || p->dataLen + PKT_HDR_LENGTH + 1>buflen) {
        err = ERROR_PKT_LENGTH;
    }

    if (err==ERROR_NONE) {
        sum1 = p8[p->dataLen + PKT_HDR_LENGTH];
        sum2 = get_sum(p, p->dataLen + PKT_HDR_LENGTH);
        if (sum1 != sum2) {
            #ifdef _WIN32
            //LOG("____checksum: 0x%02x, p8 0x%02x\n", sum1, sum2);
            #endif
            err = ERROR_PKT_CHECKSUM;
        }
    }
    
    return err;
}


int pkt_pack_data(U8 type, U8 nAck, void *data, int dlen, U8 *buf, int blen)
{
    pkt_hdr_t* p = (pkt_hdr_t*)buf;
    U32 totalLen = PKT_HDR_LENGTH + dlen;

    if (totalLen>blen-1) {
        //log("_____ send_pkt len is wrong, %d, max: %d\n", totalLen+1, BUFLEN);
        return -1;
    }

    p->magic = PKT_MAGIC;
    p->type = type;
    p->flag = 0;
    p->askAck = nAck;
    p->dataLen = dlen;
    memcpy(p->data, data, dlen);
    buf[totalLen] = get_sum(buf, totalLen);
    
    return (totalLen+1);
}


int pkt_pack_ack(U8 type, U8 error, U8 *buf, int blen)
{
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
    
    return (totalLen+1);
}


int pkt_pack_err(U8 type, U8 error, U8 *buf, int blen)
{
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

    return (totalLen+1);
}







