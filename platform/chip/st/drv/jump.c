#include "jump.h"
#include "cfg.h"


#define APP_ADDR     (FLASH_BASE+APP_OFFSET)
#define BOOT_ADDR    (FLASH_BASE+BOOT_OFFSET)

static void reboot(void)
{
    __disable_fiq();
    //NVIC_SystemReset();
    //HAL_NVIC_SystemReset();
}


void jump_to_app(void)
{
    U32 init_addr = *(__IO U32*)(BOOT_ADDR);
    U32 jump_addr = *(__IO U32*)(BOOT_ADDR+4);
    jump_fn_t jump_fn  = (jump_fn_t)jump_addr;
    
    //RCC_DeInit();
    //NVIC_DeInit();
    __disable_irq();
    
    if((init_addr & 0x2FFE0000) == 0x20000000) {

        __set_MSP(init_addr);
        jump_fn();
    }
}


void jump_to_boot(void)
{
    U32 init_addr = *(__IO U32*)(APP_ADDR);
    U32 jump_addr = *(__IO U32*)(APP_ADDR+4);
    jump_fn_t jump_fn  = (jump_fn_t)jump_addr;
        
    //RCC_DeInit();
    //NVIC_DeInit();
    __disable_irq();
    
    __set_CONTROL(0);
    __set_MSP(init_addr);
    jump_fn();
}


