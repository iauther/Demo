#include <windows.h>
#include "serial.h"
#include "data.h"


CSerial mSerial;


int dev_open(int port)
{
    return  mSerial.Open(port, 115200, 0);
}

int dev_is_opened(void)
{
    return  mSerial.isOpened();
}

int dev_close(void)
{
    return mSerial.Close();
}


int dev_send(void* data, U16 len)
{
    return mSerial.Write(data, len);
}


int dev_recv(void* data, U16 len)
{
    return mSerial.Read(data, len);
}


static U8 get_checksum(U8* data, U16 len)
{
    U8 sum = 0;
    for (int i = 0; i < len; i++) {
        sum += data[i];
    }
    return sum;
}

static U8 dev_tx_buf[1000];
int dev_send_pkt(U8 type, U8 nAck, void* data, U16 len)
{
    int i, j, r, brk = 0;
    U8* buf = dev_tx_buf;
    pkt_hdr_t* p = (pkt_hdr_t*)buf;
    U32 totalLen = PKT_HDR_LENGTH + len;

    p->magic = PKT_MAGIC;
    p->type = type;
    p->flag = 0;
    p->askAck = nAck;
    p->dataLen = len;
    memcpy(p->data, data, len);
    buf[totalLen] = get_checksum(buf, totalLen);

    return dev_send(buf, totalLen + 1);
}


int dev_send_ack(U8 error)
{
    U8* buf = dev_tx_buf;
    pkt_hdr_t* p = (pkt_hdr_t*)buf;
    ack_t* ack = (ack_t*)p->data;
    U32 totalLen = PKT_HDR_LENGTH + sizeof(ack_t);

    p->magic = PKT_MAGIC;
    p->type = TYPE_ACK;
    p->flag = 0;
    p->askAck = 0;
    p->dataLen = sizeof(ack_t);

    ack->error = error;
    buf[totalLen] = get_checksum(buf, totalLen);

    return dev_send(buf, totalLen + 1);
}

