#ifndef __DEV_Hx__
#define __DEV_Hx__

#include "bio/bio.h"
#include "tca6424a.h"
#include "sim89xx.h"
#include "bmi160/bmi160.h"


enum {
    DEV_BMX160=0,
    DEV_TCA6424A,
    DEV_I2CBUS,
    DEV_BIOSIG,
};

int dev_check(U8 dev);


#endif

