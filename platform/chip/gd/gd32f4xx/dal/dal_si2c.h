#ifndef __DAL_SI2C_Hx__
#define __DAL_SI2C_Hx__

#include "types.h"
#include "dal_gpio.h"

typedef struct {
    dal_gpio_t      scl;
    dal_gpio_t      sda;
}si2c_pin_t;

typedef struct {
    si2c_pin_t      pin;
    U32             freq;
}si2c_cfg_t;

handle_t dal_si2c_init(si2c_cfg_t *si);
int dal_si2c_deinit(handle_t h);
int dal_si2c_read(handle_t h, U8 addr, U8 *data, U32 len, U8 bstop);
int dal_si2c_write(handle_t h, U8 addr, U8 *data, U32 len, U8 bstop);

#endif







