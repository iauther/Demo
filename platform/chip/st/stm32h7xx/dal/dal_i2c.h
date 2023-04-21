#ifndef __DAL_I2C_Hx__
#define __DAL_I2C_Hx__

#include "types.h"
#include "dal_gpio.h"

enum {
    I2C_1=0,
    I2C_2,
    I2C_3,
    I2C_4,
    
    I2C_MAX
};


typedef void (*dal_i2c_callback)(void *user_data);


typedef struct {
    dal_gpio_pin_t      scl;
    dal_gpio_pin_t      sda;
}dal_i2c_pin_t;

typedef struct {
    dal_i2c_pin_t       pin;
    U32                 freq;
    U8                  useDMA;
    dal_i2c_callback    callback;
    U8                  finish_flag;
}dal_i2c_cfg_t;


handle_t dal_i2c_init(dal_i2c_cfg_t *cfg);

int dal_i2c_deinit(handle_t h);
    
int dal_i2c_read(handle_t h, U16 addr, U8 *data,  U32 len, U8 bStop, U32 timeout);
int dal_i2c_write(handle_t h, U16 addr, U8 *data, U32 len, U8 bStop, U32 timeout);

int dal_i2c_set_callback(handle_t h, dal_i2c_callback callback);

int dal_i2c_at_read(handle_t h, U16 devAddr, U16 memAddr, U16 memAddrSize, U8 *data, U32 len, U32 timeout);
int dal_i2c_at_write(handle_t h, U16 devAddr, U16 memAddr, U16 memAddrSize, U8 *data, U32 len, U32 timeout);

int dal_i2c_mem_read(handle_t h, U16 devAddr, U16 memAddr, U16 memAddrSize, U8 *data, U32 len, U32 timeout);
int dal_i2c_mem_write(handle_t h, U16 devAddr, U16 memAddr, U16 memAddrSize, U8 *data, U32 len, U32 timeout);



#endif /*__ i2c_H */
