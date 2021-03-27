#ifndef __SI2C_Hx__
#define __SI2C_Hx__

#include "types.h"

typedef struct {
    hal_gpio_pin_t scl_pin;
    hal_gpio_pin_t sda_pin;
    U32            freq;
}si2c_cfg_t;

int si2c_init(si2c_cfg_t *si);

int si2c_read(U8 addr, U8 *data, U32 len);

int si2c_write(U8 addr, U8 *data, U32 len);



#endif







