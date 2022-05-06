#ifndef __NET_Hx__
#define __NET_Hx__

#include "types.h"

enum {
    NET_ETH=0,
    NET_WIFI,

    NET_MAX,
};



int net_init(void);

int net_send();

#endif