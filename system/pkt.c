#include "pkt.h"
#include "paras.h"
#include "data.h"
#include "log.h"
#include "cfg.h"
#include "drv/uart.h"


#ifdef _WIN32
#include "port.h"
#else
#include "drv/uart.h"
#endif


U8 pkt_rx_buf[PKT_BUFLEN];
U8 pkt_tx_buf[PKT_BUFLEN];
static pkt_cfg_t myCfg;


static U8 get_sum(void* data, U16 len)
{
    U8 sum = 0;
    U8 *buf=(U8*)data;
    
    for (int i = 0; i < len; i++) {
        sum += buf[i];
    }
    return sum;
}
static handle_t port_init(U8 port, port_callback_t cb)
{
#ifdef _WIN32
    return NULL;
#else
    switch(port) {
        
        case PORT_UART:
            uart_cfg_t uc;
            
            uc.mode = MODE_DMA;
            uc.port = COM_UART_PORT;               //PA2: TX   PA3: RX
            uc.baudrate = COM_BAUDRATE;
            uc.para.rx = cb;
            uc.para.buf = pkt_rx_buf;
            uc.para.blen = sizeof(pkt_rx_buf);
            uc.para.dlen = 0;
            
            return uart_init(&uc);
    }
#endif

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
            //LOG("____checksum: 0x%02x, p8 0x%02x\n", sum1, sum2);
            #endif
            err = ERROR_PKT_CHECKSUM;
        }
    }
    
    return err;
}


int pkt_send(U8 type, U8 nAck, void* data, U16 len)
{
    int r=0;
    
    if (sysState == STAT_UPGRADE) {
        r = send_pkt(type, nAck, data, len);
    }
    else {
        r = send_pkt(type, nAck, data, len);
    }
    
    return r;
}


int pkt_send_ack(U8 type, U8 error)
{
    return send_ack(type, error);
}


int pkt_send_err(U8 type, U8 error)
{
    return send_err(type, error);
}







