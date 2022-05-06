#ifndef __PKT_Hx__
#define __PKT_Hx__

#include "types.h"

enum {
    PKT_TYPE_PUMP=0,
    PKT_TYPE_LIGHT,


    PKT_TYPE_MAX
};




typedef struct {
    handle_t    handle;
    int         period;
}pkt_cfg_t;


#define PKT_BUFLEN           100
extern U8 pkt_rx_buf[PKT_BUFLEN];
extern U8 pkt_tx_buf[PKT_BUFLEN];


int pkt_init(U8 ptype, pkt_cfg_t *cfg);

U8 pkt_hdr_check(U8 ptype, void *data, U16 len);

void pkt_cache_reset(U8 ptype);

int pkt_ack_reset(U8 ptype, U8 type);

int pkt_check_timeout(U8 ptype, U8 type);

int pkt_send(U8 ptype, U8 type, U8 nAck, void* data, U16 len);

int pkt_resend(U8 ptype, U8 type);

int pkt_send_ack(U8 ptype, U8 type, U8 error);

int pkt_send_err(U8 ptype, U8 type, U8 error);
                            
#endif

