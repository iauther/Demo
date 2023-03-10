#ifndef __PKT_Hx__
#define __PKT_Hx__

#include "types.h"
#include "protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    PKT_TYPE_TRIGGER = 0,
    PKT_TYPE_LIGHT,

    PKT_TYPE_MAX
};

enum {
    PORT_USB=0,
    PORT_ETH,
    PORT_WIFI,
    PORT_UART,
};

int pkt_check_hdr(void* data, int dlen, int buflen);

int pkt_pack_data(U8 type, U8 nAck, void* data, int dlen, U8 *buf, int blen);

int pkt_pack_ack(U8 type, U8 error, U8 *buf, int blen);

int pkt_pack_err(U8 type, U8 error, U8 *buf, int blen);


#ifdef __cplusplus
    }
#endif
                            
#endif

