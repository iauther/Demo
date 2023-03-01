#ifndef __PKT_Hx__
#define __PKT_Hx__

#include "types.h"

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

#define PKT_BUFLEN           500
extern U8 pkt_rx_buf[PKT_BUFLEN];
extern U8 pkt_tx_buf[PKT_BUFLEN];

typedef void (*port_callback_t)(void *arg, U32 evt, U8 *data, U16 len);

typedef struct {
    U8              port;
    handle_t        handle;
    int             period;
    port_callback_t cb;
}pkt_cfg_t;


int pkt_init(pkt_cfg_t *cfg);

U8 pkt_hdr_check(void* data, U16 len);

int pkt_send(U8 type, U8 nAck, void* data, U16 len);

int pkt_send_ack(U8 type, U8 error);

int pkt_send_err(U8 type, U8 error);


#ifdef __cplusplus
    }
#endif


                            
#endif

