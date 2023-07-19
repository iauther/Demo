#ifndef __POWER_Hx__
#define __POWER_Hx__

#include "types.h"

enum {
    POWER_DEV_PMU=0,
    POWER_DEV_ECXXX,
    POWER_DEV_ADS9120,
};


enum {
    POWER_MODE_ON=0,
    POWER_MODE_OFF,
    POWER_MODE_SLEEP,
    POWER_MODE_DEEPSLEEP,
    POWER_MODE_STANDBY,
};



int power_init(void);
int power_set_dev(U8 dev, U8 mode);
int power_set(U8 mode);

int power_off(U32 wakeup_ms);
#endif
