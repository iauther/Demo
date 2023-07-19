#ifndef __RS485_Hx__
#define __RS485_Hx__

#include "types.h"
#include "comm.h"

typedef struct {
    U8        port;
    U32       baudrate;
    rx_cb_t   callback;
}rs485_cfg_t;

int rs485_init(rs485_cfg_t *cfg);
int rs485_deinit(void);
int rs485_write(U8 *data, int len);

#endif

