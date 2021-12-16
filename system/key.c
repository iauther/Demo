#include "key.h"



static key_cfg_t keyCfg={0};

static void key_isr(void)
{
    
    
}

static int key_scan(void)
{
    
    return 0;
}

/////////////////////////////////////


int key_init(key_cfg_t *cfg)
{
    if(!cfg) {
        return -1;
    }
    
    //gpio_init();
    
    
    
    
    
    
    
    
    keyCfg = *cfg;
    
    return 0;
}



int key_set_callback(key_cb_fn cb)
{
    if(!cb) {
        return -1;
    }
    
    keyCfg.cb = cb;
    
    return 0;
}














