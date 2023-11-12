#include "dal_gpio.h"
#include "gd32f4xx_gpio.h"

#if defined(__CC_ARM)
#pragma diag_suppress 1296
#endif


typedef struct {
    dal_gpio_cfg_t  cfg;
}dal_gpio_handle_t;



static void set_clk(dal_gpio_handle_t *h, U8 on)
{
    switch((U32)h->cfg.gpio.grp) {
        case GPIOA:
        {
            if(on)   rcu_periph_clock_enable(RCU_GPIOA);
            else     rcu_periph_clock_disable(RCU_GPIOA);
        }
        break;
        
        case GPIOB:
        {
            if(on)   rcu_periph_clock_enable(RCU_GPIOB);
            else     rcu_periph_clock_disable(RCU_GPIOB);
        }
        break;
        
        case GPIOC:
        {
            if(on)   rcu_periph_clock_enable(RCU_GPIOC);
            else     rcu_periph_clock_disable(RCU_GPIOC);
        }
        break;
        
        case GPIOD:
        {
            if(on)   rcu_periph_clock_enable(RCU_GPIOD);
            else     rcu_periph_clock_disable(RCU_GPIOD);
        }
        break;
        
        case GPIOE:
        {
            if(on)   rcu_periph_clock_enable(RCU_GPIOE);
            else     rcu_periph_clock_disable(RCU_GPIOE);
        }
        break;
        
        case GPIOF:
        {
            if(on)   rcu_periph_clock_enable(RCU_GPIOF);
            else     rcu_periph_clock_disable(RCU_GPIOF);
        }
        break;
        
        case GPIOG:
        {
            if(on)   rcu_periph_clock_enable(RCU_GPIOG);
            else     rcu_periph_clock_disable(RCU_GPIOG);
        }
        break;
        
        case GPIOH:
        {
            if(on)   rcu_periph_clock_enable(RCU_GPIOH);
            else     rcu_periph_clock_disable(RCU_GPIOH);
        }
        break;
        
        case GPIOI:
        {
            if(on)   rcu_periph_clock_enable(RCU_GPIOI);
            else     rcu_periph_clock_disable(RCU_GPIOI);
        }
        break;
    }
}


handle_t dal_gpio_init(dal_gpio_cfg_t *cfg)
{
    int r;
    bit_status hl;
    dal_gpio_handle_t *h=NULL;
    U32 pull[3]={GPIO_PUPD_NONE,GPIO_PUPD_PULLUP,GPIO_PUPD_PULLDOWN};
    
    if(!cfg) {
        return NULL;
    }
    
    h = (dal_gpio_handle_t*)malloc(sizeof(dal_gpio_handle_t));
    if(!h) {
        return NULL;
    }
    
    h->cfg = *cfg;
    
    set_clk(h, 1);
    switch(h->cfg.mode) {
        case MODE_IN:
        gpio_mode_set(h->cfg.gpio.grp, GPIO_MODE_INPUT, pull[h->cfg.pull], h->cfg.gpio.pin);
        break;
        
        case MODE_OUT_PP:
        hl = cfg->hl?SET:RESET;
        gpio_bit_write(h->cfg.gpio.grp, h->cfg.gpio.pin, hl);
        gpio_output_options_set(h->cfg.gpio.grp, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, h->cfg.gpio.pin);
        gpio_mode_set(h->cfg.gpio.grp, GPIO_MODE_OUTPUT, pull[h->cfg.pull], h->cfg.gpio.pin);
        break;
        
        case MODE_OUT_OD:
        hl = cfg->hl?SET:RESET;
        gpio_bit_write(h->cfg.gpio.grp, h->cfg.gpio.pin, hl);
        gpio_output_options_set(h->cfg.gpio.grp, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, h->cfg.gpio.pin);
        gpio_mode_set(h->cfg.gpio.grp, GPIO_MODE_OUTPUT, pull[h->cfg.pull], h->cfg.gpio.pin);
        break;
    }
    
    return h;
}


int dal_gpio_deinit(handle_t h)
{
    dal_gpio_handle_t *dh=(dal_gpio_handle_t*)h;
    
    if(!dh) {
        return -1;
    }
    
    //gpio_deinit(dh->cfg.gpio.grp);
    free(dh);
    
    return 0;
}


int dal_gpio_set_dir(handle_t h, U8 dir)
{
    dal_gpio_handle_t *dh=(dal_gpio_handle_t*)h;
    
    if(!dh) {
        return -1;
    }
    
    if(dir==OUTPUT) gpio_mode_set(dh->cfg.gpio.grp, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, dh->cfg.gpio.pin);
    else            gpio_mode_set(dh->cfg.gpio.grp, GPIO_MODE_INPUT,  GPIO_PUPD_PULLUP, dh->cfg.gpio.pin);
    
    return 0;
}


int dal_gpio_get_dir(handle_t h, U8 *dir)
{
    dal_gpio_handle_t *dh=(dal_gpio_handle_t*)h;
    
    if(!dh || !dir) {
        return -1;
    }
    
    //*dir = (DATA_OF(dh->p.pin)->init.Mode==GPIO_MODE_INPUT)?INPUT:OUTPUT;
    
    return 0;
}


int dal_gpio_set_hl(handle_t h, U8 hl)
{
    dal_gpio_handle_t *dh=(dal_gpio_handle_t*)h;
    
    if(!dh) {
        return -1;
    }
    
    if(hl) {
        gpio_bit_set(dh->cfg.gpio.grp, dh->cfg.gpio.pin);
    }
    else {
        gpio_bit_reset(dh->cfg.gpio.grp, dh->cfg.gpio.pin);
    }
    
    return 0;
}


int dal_gpio_get_hl(handle_t h)
{
    FlagStatus st;
    dal_gpio_handle_t *dh=(dal_gpio_handle_t*)h;
    
    if(!dh) {
        return -1;
    }
    
    st = gpio_input_bit_get(dh->cfg.gpio.grp, dh->cfg.gpio.pin);
    return (st==SET)?1:0;
}


int dal_gpio_pull(handle_t h, U8 type)
{
    dal_gpio_handle_t *dh=(dal_gpio_handle_t*)h;
    
    if(!dh) {
        return -1;
    }
    
    
    
    return 0;
}



int dal_gpio_set_callback(handle_t h, dal_gpio_callback cb)
{
    dal_gpio_handle_t *dh=(dal_gpio_handle_t*)h;
    
    if(!dh) {
        return -1;
    }
    
    
    return 0;
}


int dal_gpio_irq_en(handle_t h, U8 on)
{
    dal_gpio_handle_t *dh=(dal_gpio_handle_t*)h;
    
    if(!dh) {
        return -1;
    }
    
    
    
    return 0;
}

int dal_gpio_clk_en(handle_t h, U8 on)
{
    dal_gpio_handle_t *dh=(dal_gpio_handle_t*)h;
    
    if(!dh) {
        return -1;
    }
    
    set_clk(dh, on);
    
    return 0;
}

