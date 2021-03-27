#ifndef __VALVE_Hx__
#define __VALVE_Hx__

#include "types.h"

enum {
    VALVE_OPEN=0,
    VALVE_CLOSE,
};

int valve_init(void);

int valve_set(U8 v);

int valve_get(U8 *v);

#endif



