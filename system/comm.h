#ifndef __COMM_Hx__
#define __COMM_Hx__

#include "types.h"
#include "pkt.h"
#include "cfg.h"


typedef struct {
    U8          port;
    handle_t    h;
    
    U8          *rxBuf;
    U8          *txBuf;
    
    int         chkID;
}comm_handle_t;

typedef int (*rx_cb_t)(handle_t h, void *addr, U32 evt, void *data, int len);


handle_t comm_init(U8 port, rx_cb_t cb);
int comm_deinit(handle_t h);
int comm_data_proc(handle_t h, void *addr, void *data, int len);
int comm_send_paras(handle_t h, void *addr, U8 flag);
int comm_send_data(handle_t h, void *addr, U8 type, U8 nAck, void *data, int len);
int comm_check_timeout(handle_t h, void *addr, U8 type);
int comm_recv_proc(handle_t h, void *addr, void *data, int len);
int comm_get_buf(handle_t h, U8** data, int *len);
int comm_get_paras_flag(handle_t h, void *addr);
int comm_ack_poll(handle_t h, void *addr);

int comm_pure_send(handle_t h, void *addr, void* data, int len);

#endif

