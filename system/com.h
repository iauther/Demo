#ifndef __COM_Hx__
#define __COM_Hx__

#include "types.h"
#include "pkt.h"

typedef void (*com_callback)(U8 *data, U16 len);


int com_init(com_callback cb, int loop_period);
int com_deinit(void);
U8 com_data_proc(void *data, U16 len);
U8 com_send_data(U8 type, U8 nAck, void* data, U16 len);
U8 com_loop_check(U8 flag);      //0: app,  1:dev;
int com_get_buf(U8** data, U16 *len);


#endif

