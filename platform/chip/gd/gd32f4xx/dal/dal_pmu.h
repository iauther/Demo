#ifndef __DAL_PMU_Hx__
#define __DAL_PMU_Hx__


#include "types.h"


enum {
    PMU_SLEEP=0,
    PMU_DEEPSLEEP,
    PMU_STANDBY,
};


int dal_pmu_init(void);
int dal_pmu_set(U8 mode);

#endif

