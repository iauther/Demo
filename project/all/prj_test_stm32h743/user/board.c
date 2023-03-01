#include "incs.h"

handle_t i2c0Handle=NULL;
handle_t i2c1Handle=NULL;
handle_t i2c2Handle=NULL;

////////////////////////////////////////////

static int bus_init(void)
{
    i2c_cfg_t  ic;
    i2c_pin_t  p2={I2C1_SCL_PIN,I2C1_SDA_PIN};
    
    ic.pin= p2;
    ic.freq = I2C1_FREQ;
    ic.useDMA = 0;;
    ic.callback = NULL;
    ic.finish_flag=0;
    //i2c2Handle = i2c_init(&ic);
    
    return ((i2c1Handle&&i2c2Handle)?-1:0);
}

static int bus_deinit(void)
{
    int r;
    
    //r = i2c_deinit(&i2c1Handle);
    //r |= i2c_deinit(&i2c2Handle);
    
    return r;
}



static int dev_init(void)
{
    int r=0;

    //r = temp_init(i2c2Handle);
    
    return r;
}
static int dev_deinit(void)
{
    int r=0;
    
    sdram_init();
    
    return r;
}


int board_init(void)
{
    int r;
    
#ifdef BOOTLOADER
    //SCB->VTOR = FLASH_BASE | 0x20000; 
    //__enable_irq();
#endif
    
    sys_init();
    
#ifdef BOOTLOADER
    if(!upgrade_is_need()) {
        board_deinit();
        jump_to_app();
    }
#endif
    
    r = dev_init();
    //r = paras_load();

#ifdef OS_KERNEL
    //led_set_color(GREEN);
    task_init();
#else
    test_main();
#endif
   
    return r;
}


int board_deinit(void)
{
    int r;
    
    r = HAL_DeInit();
    r = HAL_RCC_DeInit();
    
    r = dev_deinit();
    r = bus_deinit();
   
    return r;
}


