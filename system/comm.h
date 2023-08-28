#ifndef __COMM_Hx__
#define __COMM_Hx__

#include "types.h"


#ifdef _WIN32
#include "mySock.h"
#include "mySerial.h"
#endif

#define COMM_BUF_LEN   1000

typedef struct {
    U8          port;
    handle_t    h;
    
    U8          rxBuf[COMM_BUF_LEN];
    U8          txBuf[COMM_BUF_LEN];
    
    int         chkID;
    
#ifdef _WIN32
    mySock      mSock;
    mySerial    mSerial;
#endif
}comm_handle_t;


#ifdef __cplusplus
extern "C" {
#endif

handle_t comm_init(U8 port, rx_cb_t cb);
int comm_deinit(handle_t h);
int comm_data_proc(handle_t h, void* addr, void* data, int len);
int comm_send_paras(handle_t h, void* addr, U8 flag);
int comm_send_data(handle_t h, void* addr, U8 type, U8 nAck, void* data, int len);
int comm_check_timeout(handle_t h, void* addr, U8 type);
int comm_recv_proc(handle_t h, void* addr, void* data, int len);
int comm_get_buf(handle_t h, U8** data, int* len);
int comm_get_paras_flag(handle_t h, void* addr);
int comm_ack_poll(handle_t h, void* addr);

int comm_pure_send(handle_t h, void* addr, void* data, int len);
int comm_recv_data(handle_t h, void* data, int len);

#ifdef __cplusplus
}
#endif

#endif

