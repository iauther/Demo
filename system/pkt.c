#include "pkt.h"
#include "log.h"

static chkcode_t get_chkcode(int chkID, U8* cc)
{
    chkcode_t chk={0};
    
    switch(chkID) {
        
        case CHK_SUM:
        {
            chk.sum = cc[0];
        }
        break;
        
        default:
        break;
    }
    
    return chk;
}
static void set_chkcode(int chkID, chkcode_t *ccode, U8* cc)
{
    chkcode_t chk={0};
    
    switch(chkID) {
        
        case CHK_SUM:
        {
            cc[0] = ccode->sum;
        }
        break;
        
        default:
        break;
    }
}

static chkcode_t cal_chkcode(int chkID, void* data, U16 len)
{
    chkcode_t chk={0};
    U8 sum = 0;
    U8 *buf=(U8*)data;
    
    switch(chkID) {
        
        case CHK_SUM:
        {
            chk.sum = 0;
            for (int i = 0; i < len; i++) {
                sum += buf[i];
            }
        }
        break;
        
        default:
        break;
    }
    
    return chk;
}
static int get_chkcode_len(int chkID)
{
    switch(chkID) {
        case CHK_SUM:
        case CHK_CRC8:      return 1;
        case CHK_CRC16:     return 2;
        case CHK_CRC32:     return 4;
        default:            return 0;
    }
}


///////////////////////////////////////////////////////////////////////////////
int pkt_check_hdr(void *data, int dlen, int buflen, int chkID)
{
    chkcode_t cc1,cc2;
    U8 sum1,sum2,err=ERROR_NONE;
    U8 *p8=(U8*)data;
    pkt_hdr_t *hdr=(pkt_hdr_t*)data;
    int head_data_len=hdr->dataLen+PKT_HDR_LENGTH;
    int total_len=head_data_len+get_chkcode_len(chkID);
    
    if (hdr->magic != PKT_MAGIC) {
        err = ERROR_PKT_MAGIC;
    }
    
    //if (total_len != dlen || total_len>buflen) {
    //    err = ERROR_PKT_LENGTH;
    //}

    if (err==ERROR_NONE && chkID!=CHK_NONE) {
        cc1 = get_chkcode(chkID, &p8[head_data_len]);
        cc2 = cal_chkcode(chkID, hdr, hdr->dataLen+PKT_HDR_LENGTH);
        if (memcmp(&cc1, &cc2, sizeof(chkcode_t))) {
            #ifdef _WIN32
            //LOG("____checksum: 0x%02x, p8 0x%02x\n", sum1, sum2);
            #endif
            err = ERROR_PKT_CHECKSUM;
        }
    }
    
    return err;
}


static int do_pack(U8 type, U8 error, U8 nAck, void *data, int dlen, U8 *buf, int blen, int chkID)
{
    chkcode_t cc;
    pkt_hdr_t* hdr=(pkt_hdr_t*)buf;
    int dataLen=PKT_HDR_LENGTH+dlen;
    int totalLen=dataLen+get_chkcode_len(chkID);

    if (totalLen>blen) {
        //LOGE("_____ do_pack len wrong, need: %d, max: %d\n", totalLen, blen);
        return -1;
    }

    hdr->magic = PKT_MAGIC;
    hdr->type = type;
    hdr->flag = 0;
    
    if(type==TYPE_ACK) {
        ack_t* ack = (ack_t*)hdr->data;
        hdr->askAck = 0;
        hdr->dataLen = sizeof(ack_t);
        ack->type = type;
        ack->error = error;
    }
    else if(type==TYPE_ERROR) {
        error_t* err = (error_t*)hdr->data;
        hdr->askAck = 0;
        hdr->dataLen = sizeof(error_t);
        err->type = type;
        err->error = error;
    }
    else {
        hdr->askAck = nAck;
        hdr->dataLen = dlen;
        memcpy(hdr->data, data, dlen);
    }
    
    cc = cal_chkcode(chkID, buf, dataLen);
    set_chkcode(chkID, &cc, &buf[dataLen]);
        
    return totalLen;
}


int pkt_pack_data(U8 type, U8 nAck, void *data, int dlen, U8 *buf, int blen, int chkID)
{
    return do_pack(type, 0, nAck, data, dlen, buf, blen, chkID);
}



int pkt_unpack_cap(void *data, int dlen, pkt_callback_t callback, int chkID)
{
    int tlen=0;
    ch_data_t *cd=NULL;
    pkt_hdr_t *p=(pkt_hdr_t*)data;
    
    if(p->type!= TYPE_CAP) {
        return -1;
    }
    
    while(1) {
        if(tlen>=p->dataLen) {
            break;
        }
        
        cd = (ch_data_t*)(p->data+tlen);
        if(callback) {
            callback(cd);
        }
        //tlen += cd->wavlen+sizeof(ch_data_t);
    }
    
    return 0;
}


int pkt_pack_ack(U8 type, U8 error, U8 *buf, int blen, int chkID)
{
    return do_pack(type, error, 0, NULL, 0, buf, blen, chkID);
}


int pkt_pack_err(U8 type, U8 error, U8 *buf, int blen, int chkID)
{
    return do_pack(type, error, 0, NULL, 0, buf, blen, chkID);
}


