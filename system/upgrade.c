 #include <stddef.h>
 #include "at24cxx.h"
#include "drv/flash.h"
#include "cfg.h"
#include "data.h"
#include "paras.h"
#include "upgrade.h"

static U32 upgrade_addr=0;
int upgrade_init(U8 obj)
{
    int r;
    U32 from=(obj==OBJ_BOOT)?BOOT_OFFSET:APP_OFFSET;
    
    upgrade_addr = from;
    r = flash_init();
    
    return r;
}

extern paras_t DEFAULT_PARAS;
int upgrade_check(void)
{
    fw_info_t fwInfo;
    
    paras_get_fwinfo(&fwInfo);
    if(fwInfo.magic!=DEFAULT_PARAS.fwInfo.magic) {
        paras_set_fwinfo(&DEFAULT_PARAS.fwInfo);
    }
    
    return 0;
}


int upgrade_write(U8 *data, U32 len)
{
    int r;
    
    r = flash_write(upgrade_addr, data, len);
    if(r==0) {
        upgrade_addr += len;
    }
    
    return 0;
}


int upgrade_refresh(void)
{
    int r;
    U32 fwmagic;
    
    paras_get_fwmagic(&fwmagic);
    if(fwmagic!=FW_MAGIC) {       
        paras_set_fwinfo(&DEFAULT_PARAS.fwInfo);
    }
    
    return 0;
}

