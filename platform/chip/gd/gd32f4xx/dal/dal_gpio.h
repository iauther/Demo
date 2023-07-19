#ifndef __DAL_GPIO_Hx__
#define __DAL_GPIO_Hx__

#include "types.h"


typedef void (*dal_gpio_callback)(void *user_data);

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
    MODE_OUT_PP,
    MODE_OUT_OD,
    
    MODE_IT_RISING,
    MODE_IT_FALLING,
    MODE_IT_BOTH,
    
    MODE_EVT_RISING,
    MODE_EVT_FALLING,
    MODE_EVT_BOTH,
    
    GPIO_MODE_MAX
};


typedef struct {
    U32         grp;
    U32         pin;
}dal_gpio_t;

typedef struct {
    dal_gpio_t  gpio;
    U8          mode;
    U8          pull;
    U8          hl;
}dal_gpio_cfg_t;


handle_t dal_gpio_init(dal_gpio_cfg_t *cfg);
int dal_gpio_deinit(handle_t h);
int dal_gpio_set_dir(handle_t h, U8 dir);
int dal_gpio_get_dir(handle_t h, U8 *dir);
int dal_gpio_set_hl(handle_t h, U8 hl);
int dal_gpio_get_hl(handle_t h);
int dal_gpio_pull(handle_t h, U8 type);
int dal_gpio_irq_en(handle_t h, U8 on);
int dal_gpio_set_callback(handle_t h, dal_gpio_callback cb);
int dal_gpio_clk_en(handle_t h, U8 on);

#endif

