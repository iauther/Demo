#ifndef __MB_Hx__
#define __MB_Hx__

#include "types.h"

enum {
    MODE_MASTER=0,
    MODE_SLAVE,
};

enum {
    PORT_UART=0,
    PORT_ETH,
};


typedef struct {
    U8      mode;
    U8      port;
    U8      devID;
    U8      b485;
}mb_cfg_t;

int mbus_init(mb_cfg_t *master, mb_cfg_t *slave);




#endif

