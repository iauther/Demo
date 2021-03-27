#include "drv/packet.h"
#include "log.h"

#define HEAD_CODE   0x1bdf
#define TAIL_CODE   0x9bdf

static U8 get_chksum(U8 *data, int len)
{
    int i;
    U8 chkSum=0;
    
    for(i=0; i<len; i++) {
        chkSum += data[i];
    }
    
    return chkSum;
}



int packet_cmd_fill(U8 *buf, U16 bufLen, U8 *cmd, U16 cmdLen)
{
    int i;
    U8 chkSum;
    cmd_hdr_t *hdr=(cmd_hdr_t*)buf;
    int dlen = CMD_HDR_SIZE+cmdLen;
    U16 *tail=(U16*)(buf+dlen+1);
    U8 *chksum=buf+dlen;
    
    if(!buf || !cmd || bufLen<CMD_PACKET_SIZE(cmdLen)) {
        LOGE("___ cmd packet format param error\r\n");
        return -1;
    }
    
    hdr->head   = HEAD_CODE;
    hdr->flag   = 3;
    hdr->cmdLen = cmdLen;
    memcpy(buf+CMD_HDR_SIZE, cmd, cmdLen);
    *chksum = get_chksum(buf, CMD_HDR_SIZE+cmdLen);
    *tail = TAIL_CODE;
    
    return CMD_PACKET_SIZE(cmdLen);
}


int packet_data_fill(U8 *buf, U16 bufLen, U8 *data, U16 dataLen, U8 chs, U8 bits)
{
    int i;
    U8 chkSum;
    static U16 pid=0;
    data_hdr_t *hdr=(data_hdr_t*)buf;
    int dlen = DATA_HDR_SIZE+dataLen;
    U16 *tail=(U16*)(buf+dlen+1);
    U8 *chksum=buf+dlen;
    
    if(!buf || !data || bufLen<DATA_PACKET_SIZE(dataLen)) {
        LOGE("___ data packet format param error\r\n");
        return -1;
    }
    
    hdr->head   = HEAD_CODE;
    hdr->ver    = 1;
    hdr->pid    = pid++;
    hdr->chs    = chs;
    hdr->format = 2;
    hdr->bits   = bits;
    hdr->lsb    = 1;
    hdr->ecgLen = dataLen;
    memcpy(buf+DATA_HDR_SIZE, data, dataLen);
    *chksum = get_chksum(buf, DATA_HDR_SIZE+dataLen);
    *tail = TAIL_CODE;
    
    return DATA_PACKET_SIZE(dataLen);
}








