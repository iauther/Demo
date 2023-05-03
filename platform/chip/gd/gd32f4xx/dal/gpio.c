#include "dal/gpio.h"

#if defined(__CC_ARM)
#pragma diag_suppress 1296
#endif





int gpio_init(gpio_pin_t *pin, U8 mode)
{
    int r;
    
    if(!pin) {
        return -1;
    }
    
    
    return 0;
}


int gpio_deinit(gpio_pin_t *pin)
{
    if(!pin) {
        return -1;
    }
    
    
    return 0;
}


int gpio_set_dir(gpio_pin_t *pin, U8 dir)
{
    if(!pin) {
        return -1;
    }
    
    if(dir==MODE_OUTPUT) gpio_mode_set(pin->grp, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, pin->pin);
    else                 gpio_mode_set(pin->grp, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, pin->pin);
    
    return 0;
}


int gpio_get_dir(gpio_pin_t *pin, U8 *dir)
{
    if(!pin || !dir) {
        return -1;
    }
    
    //*dir = (DATA_OF(pin->pin)->init.Mode==GPIO_MODE_INPUT)?INPUT:OUTPUT;
    
    return 0;
}


int gpio_set_hl(gpio_pin_t *pin, U8 hl)
{
    if(!pin) {
        return -1;
    }
    
    gpio_output_options_set(pin->grp, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, pin->pin);
    gpio_bit_write(pin->grp, pin->pin, hl?SET:RESET);
    
    return 0;
}


U8 gpio_get_hl(gpio_pin_t *pin)
{
    lagStatus st = gpio_input_bit_get(pin->grp, pin->pin);
    return (st==SET)?1:0;
}


int gpio_pull(gpio_pin_t *pin, U8 type)
{
    if(!pin) {
        return -1;
    }
    
    
    
    return 0;
}



int gpio_set_callback(gpio_pin_t *pin, gpio_callback cb)
{
    int index;
    
    if(!pin) {
        return -1;
    }
    
    
    return 0;
}


int gpio_irq_en(gpio_pin_t *pin, U8 on)
{
    if(!pin) {
        return -1;
    }
    
    
    
    return 0;
}

int gpio_clk_en(gpio_pin_t *pin, U8 on)
{
    if(!pin) {
        return -1;
    }
    
    switch((U32)pin->grp) {
        case (U32)GPIOA:
        {
            if(on)   rcu_periph_clock_enable(RCU_GPIOA);
            else     rcu_periph_clock_disable(RCU_GPIOA);
        }
        break;
        
        case (U32)GPIOB:
        {
            if(on)   rcu_periph_clock_enable(RCU_GPIOB);
            else     rcu_periph_clock_disable(RCU_GPIOB);
        }
        break;
        
        case (U32)GPIOC:
        {
            if(on)   rcu_periph_clock_enable(RCU_GPIOC);
            else     rcu_periph_clock_disable(RCU_GPIOC);
        }
        break;
        
        case (U32)GPIOD:
        {
            if(on)   rcu_periph_clock_enable(RCU_GPIOD);
            else     rcu_periph_clock_disable(RCU_GPIOD);
        }
        break;
        
        case (U32)GPIOE:
        {
            if(on)   rcu_periph_clock_enable(RCU_GPIOE);
            else     rcu_periph_clock_disable(RCU_GPIOE);
        }
        break;
        
        case (U32)GPIOF:
        {
            if(on)   rcu_periph_clock_enable(RCU_GPIOF);
            else     rcu_periph_clock_disable(RCU_GPIOF);
        }
        break;
        
        case (U32)GPIOG:
        {
            if(on)   rcu_periph_clock_enable(RCU_GPIOG);
            else     rcu_periph_clock_disable(RCU_GPIOG);
        }
        break;
        
        
        
        
        default:
        return -1;
    }
    
    return 0;
}

