#include "dal_jump.h"
#include "log.h"
#include "cfg.h"


typedef void (*jump_fn_t)(void);


void dal_jump_to_app(U32 addr)
{
    U32 init_addr = *((__IO U32*)(addr|FLASH_BASE));
    U32 jump_addr = *((__IO U32*)((addr+4)|FLASH_BASE));
    jump_fn_t jump_fn  = (jump_fn_t)jump_addr;
    
    LOGD("__dal_jump_to_app SP: 0x%08x\n", init_addr);
    LOGD("__dal_jump_to_app PC: 0x%08x\n", jump_addr);
    //if((init_addr & 0x2FFE0000) == 0x20000000) {
        __disable_irq();
        __set_MSP(init_addr);
        
        LOGD("__jump to 0x%08x\n", jump_addr);
        jump_fn();
    //}
}


void dal_jump_to_boot(U32 addr)
{
    U32 init_addr = *((__IO U32*)(addr|FLASH_BASE));
    U32 jump_addr = *((__IO U32*)((addr+4)|FLASH_BASE));
    jump_fn_t jump_fn  = (jump_fn_t)jump_addr;
        
    LOGD("__dal_jump_to_boot SP: 0x%08x\n", init_addr);
    LOGD("__dal_jump_to_boot PC: 0x%08x\n", jump_addr);
    //if((init_addr & 0x2FFE0000) == 0x20000000) {
        __disable_irq();
        __set_CONTROL(0);
        __set_MSP(init_addr);
        
        LOGD("__dal_jump_to_boot 0x%08x\n", jump_addr);
        jump_fn();
    //}
}


