#ifndef __TCP232_Hx__
#define __TCP232_Hx__

#include "types.h"
#include "drv/uart.h"

typedef void (*tcp232_rx_t)(U8 *data, U16 data_len);


int tcp232_init(tcp232_rx_t fn);
    
int tcp232_write(U8 *data, U32 len);


#endif

