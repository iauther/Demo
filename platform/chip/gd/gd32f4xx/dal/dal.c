#include "gd32f4xx.h"
#include "upgrade.h"
#include "dal_sdram.h"
#include "dal.h"
#include "log.h"
#include "cfg.h"


#define STORAGE_INFO_REG       0x1FFF7A20
#define UNIQUE_ID_REG          0x1FFF7A10     //0x1FFF7A10~0x1FFF7A18


U32 sys_freq = 0;
////////////////////////////////////////

extern void systick_config(void);
static int dal_clk_init(void)
{
	nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);
    systick_config();
    
	return 0;
}


int dal_init(void)
{
    dal_clk_init();
    sys_freq = SystemCoreClock;

#ifndef BOOTLOADER
    dal_sdram_init();
#endif
    
    return 0;
}


int dal_set_vector(void)
{
    int r;
    upg_info_t info;
    U32 addr=APP_OFFSET+sizeof(upg_hdr_t);
    
    r = upgrade_get_upg_info(&info);
    if(r==0) {
        addr = info.runAddr;
    }
    addr |= FLASH_BASE;
    LOGD("___dal_set_vector, addr: 0x%08x\n", addr);
    
    nvic_vector_table_set(NVIC_VECTTAB_FLASH, addr);
    __enable_irq();
    
    return 0;
}


void dal_reboot(void)
{
    __disable_fiq();
    NVIC_SystemReset();
    //HAL_NVIC_SystemReset();
}


U32 dal_get_freq(void)
{
    return SystemCoreClock;
}


int dal_get_info(mcu_info_t *info)
{
    U8 i;
    U32 v;
    
    v = REG32(STORAGE_INFO_REG);
	info->flashSize  = (v>>16)*1024;
    info->flashStart = 0;
    info->flashEnd   = info->flashStart+info->flashSize;
    
    info->sramSize   = (v&0xffff)*1024;
    info->sramStart  = 0;
    info->sramEnd    = info->sramStart+info->sramSize;
	for(i=0; i<3; i++) {
		info->uniqueID[i] = REG32(UNIQUE_ID_REG+i);
	}
    
    return 0;
}


