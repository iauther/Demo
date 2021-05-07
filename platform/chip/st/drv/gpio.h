#ifndef __GPIO_Hx__
#define __GPIO_Hx__

#include "types.h"


typedef void (*gpio_callback)(void *user_data);

enum {
    INPUT=0,
    OUTPUT,
};

enum {
    MODE_INPUT=0,
    MODE_OUTPUT,
    
    MODE_IT_RISING,
    MODE_IT_FALLING,
    MODE_IT_BOTH,
    
    MODE_EVT_RISING,
    MODE_EVT_FALLING,
    MODE_EVT_BOTH,
    
    GPIO_MODE_MAX
};


typedef struct {
    
    GPIO_TypeDef    *grp;
    U32              pin;
}gpio_pin_t;

int gpio_init(gpio_pin_t *pin, U8 mode);
int gpio_deinit(gpio_pin_t *pin);

int gpio_set_dir(gpio_pin_t *pin, U8 dir);
int gpio_get_dir(gpio_pin_t *pin, U8 *dir);
int gpio_set_hl(gpio_pin_t *pin, U8 hl);
U8 gpio_get_hl(gpio_pin_t *pin);
int gpio_pull(gpio_pin_t *pin, U8 type);
int gpio_irq_en(gpio_pin_t *pin, U8 on);
int gpio_set_callback(gpio_pin_t *pin, gpio_callback cb);

int gpio_clk_en(gpio_pin_t *pin, U8 on);

#endif

