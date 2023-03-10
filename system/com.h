#ifndef __COM_Hx__
#define __COM_Hx__

#include "types.h"
#include "pkt.h"


#define RX_BUF_LEN 512
typedef struct {
    U8          port;
    handle_t    h;
    
    U8          rxBuf[RX_BUF_LEN];
}com_handle_t;

typedef int (*rx_cb_t)(handle_t h, void *addr, U32 evt, void *data, int len);


handle_t com_init(U8 port, rx_cb_t cb);
int com_deinit(handle_t *h);
int com_data_proc(handle_t h, void *addr, void *data, int len);
int com_send_paras(handle_t h, void *addr, U8 flag);
int com_send_data(handle_t h, void *addr, U8 type, U8 nAck, void *data, int len);
int com_check_timeout(handle_t h, void *addr, U8 type);
int com_proc(handle_t h, void *addr, void *data, U16 len);
int com_get_buf(handle_t h, U8** data, int *len);
int com_get_paras_flag(handle_t h, void *addr);
int com_ack_poll(handle_t h, void *addr);

#endif

