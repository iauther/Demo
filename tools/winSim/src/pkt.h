#ifndef __PKT_Hx__
#define __PKT_Hx__

#include "types.h"


typedef struct {
    handle_t    handle;
    U16         retries;
}pkt_cfg_t;


int pkt_init(pkt_cfg_t *cfg);

U8 pkt_hdr_check(void *data, U16 len);

void pkt_cache_reset(void);

void pkt_ack_update(U8 type);

void pkt_ack_check(U8 type);

int pkt_send(U8 type, U8 nAck, void* data, U16 len);

int pkt_send_ack(U8 type, U8 error);
                            
#endif

