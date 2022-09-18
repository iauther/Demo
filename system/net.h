#ifndef __NET_Hx__
#define __NET_Hx__

#include "types.h"
#include "eth.h"

enum {
    NET_ETH=0,
    NET_WIFI,

    NET_MAX,
};



int net2_init(void);

int net2_conn(void);

int net2_read(U8 *data, int len);

int net2_write(U8 *data, int len);

int net2_test(void);

#endif
