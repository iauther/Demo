#ifndef __SI2C_Hx__
#define __SI2C_Hx__

#include "types.h"
#include "drv/gpio.h"

typedef struct {
    gpio_pin_t      scl;
    gpio_pin_t      sda;
    U32            freq;
}si2c_cfg_t;

int si2c_init(si2c_cfg_t *si);

int si2c_read(U8 addr, U8 *data, U32 len, U8 stop);

int si2c_write(U8 addr, U8 *data, U32 len, U8 stop);



#endif







