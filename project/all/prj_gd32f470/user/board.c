#include "incs.h"
#include "log.h"
#include "paras.h"



static int hw_init(void)
{
    int r=0;

    sdram_init();
    log_init();
    //r = paras_load();
    
    return r;
}


int board_init(void)
{
    int r;
    
#ifdef BOOTLOADER
    //SCB->VTOR = FLASH_BASE | 0x20000; 
    //__enable_irq();
#endif
    
    dal_init();
    hw_init();
    
#ifdef BOOTLOADER
    if(!upgrade_is_need()) {
        board_deinit();
        jump_to_app();
    }
#endif

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
   
    return r;
}


