#ifndef __SI2C_Hx__
#define __SI2C_Hx__

#include "types.h"
#include "gpio.h"

typedef struct {
    gpio_pin_t      scl;
    gpio_pin_t      sda;
    U32             freq;
}si2c_pin_t;

typedef struct {
    si2c_pin_t      pin;
    U32             freq;
}si2c_cfg_t;

handle_t si2c_init(si2c_cfg_t *si);

int si2c_deinit(handle_t *h);

int si2c_read(handle_t h, U8 addr, U8 *data, U32 len);

int si2c_write(handle_t h, U8 addr, U8 *data, U32 len);



#endif







