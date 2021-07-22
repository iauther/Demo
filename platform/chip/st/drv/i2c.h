#ifndef __I2C_Hx__
#define __I2C_Hx__

#include "types.h"

enum {
    I2C_1=0,
    I2C_2,
    I2C_3,
    
    I2C_MAX
};


typedef void (*i2c_callback)(void *user_data);

typedef struct {
    GPIO_TypeDef    *grp;
    U32             pin;
}pin_t;
typedef struct {
    pin_t           scl;
    pin_t           sda;
}i2c_pin_t;

typedef struct {
    i2c_pin_t       pin;
    U32             freq;
    U8              useDMA;
    i2c_callback    callback;
    U8              finish_flag;
}i2c_cfg_t;


handle_t i2c_init(i2c_cfg_t *cfg);

int i2c_deinit(handle_t *h);
    
int i2c_read(handle_t h, U16 addr, U8 *data,  U32 len, U8 bStop);
int i2c_write(handle_t h, U16 addr, U8 *data, U32 len, U8 bStop);

int i2c_set_callback(handle_t h, i2c_callback callback);

int i2c_at_read(handle_t h, U16 devAddr, U16 memAddr, U16 memAddrSize, U8 *data, U32 len);
int i2c_at_write(handle_t h, U16 devAddr, U16 memAddr, U16 memAddrSize, U8 *data, U32 len);

int i2c_mem_read(handle_t h, U16 devAddr, U16 memAddr, U16 memAddrSize, U8 *data, U32 len);
int i2c_mem_write(handle_t h, U16 devAddr, U16 memAddr, U16 memAddrSize, U8 *data, U32 len);



#endif /*__ i2c_H */
