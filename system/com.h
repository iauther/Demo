#ifndef __COM_Hx__
#define __COM_Hx__

#include "types.h"
#include "pkt.h"



int com_init(U8 port, port_callback_t cb, int loop_period);
int com_deinit(void);
U8 com_data_proc(void *data, U16 len);
U8 com_send_paras(U8 flag);
U8 com_send_data(U8 type, U8 nAck, void* data, U16 len);
int com_check_timeout(U8 type);
int com_get_buf(U8** data, U16 *len);
int com_get_paras_flag(void);
U8 com_ack_poll(void);

#endif

