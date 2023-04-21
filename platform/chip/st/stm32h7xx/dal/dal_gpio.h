#ifndef __DAL_GPIO_Hx__
#define __DAL_GPIO_Hx__

#include "types.h"


typedef void (*gpio_callback)(void *user_data);

enum {
    INPUT=0,
    OUTPUT,
};


enum {
    PULL_NONE=0,
    PULL_UP,
    PULL_DOWN,
};


enum {
    MODE_IN=0,
    MODE_OUT,
    
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
}dal_gpio_pin_t;

int dal_gpio_init(dal_gpio_pin_t *pin, U8 mode);
int dal_gpio_deinit(dal_gpio_pin_t *pin);
int dal_gpio_clk_en(dal_gpio_pin_t *pin, U8 on);

int dal_gpio_set_dir(dal_gpio_pin_t *pin, U8 dir);
int dal_gpio_get_dir(dal_gpio_pin_t *pin, U8 *dir);
int dal_gpio_set_hl(dal_gpio_pin_t *pin, U8 hl);
int dal_gpio_get_hl(dal_gpio_pin_t *pin);
int dal_gpio_pull(dal_gpio_pin_t *pin, U8 type);
int dal_gpio_irq_en(dal_gpio_pin_t *pin, U8 on);
int dal_gpio_set_callback(dal_gpio_pin_t *pin, gpio_callback cb);

#endif

