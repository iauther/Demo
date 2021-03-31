 #include <stddef.h>
 #include "at24cxx.h"
#include "drv/flash.h"
#include "cfg.h"
#include "data.h"
#include "upgrade.h"


typedef int (*upg_rw_t)(U8 rw, U32 addr, U8 *data, U32 len);

#ifdef UPGRADE_STORE_IN_FLASH
static int flash_rw(U8 rw, U32 addr, U8 *data, U32 len)
{
    if(rw==0) {
        return flash_read(addr, data, len);
    }
    else {
        return flash_write(addr, data, len);
    }
}
static upg_rw_t upg_rw=flash_rw;
#elif defined UPGRADE_STORE_IN_E2PROM
static int at24_rw(U8 rw, U32 addr, U8 *data, U32 len)
{
    if(rw==0) {
        return at24cxx_read(addr, data, len);
    }
    else {
        return at24cxx_write(addr, data, len);
    }
}
static upg_rw_t upg_rw=at24_rw;
#else
static upg_rw_t upg_rw=NULL;
#endif

int upgrade_get_fwmagic(U32 *fwmagic)
{
    int r=-1;
    U32 addr=offsetof(paras_t,fwInfo)+offsetof(fw_info_t,magic)+UPGRADE_STORE_ADDR;

    if(!upg_rw) {
        return -1;
    }
    
    r = upg_rw(0, addr, (U8*)fwmagic, sizeof(U32));
    
    return r;
}


int upgrade_set_fwmagic(U32 *fwmagic)
{
    int r=-1;
    U32 addr=offsetof(paras_t,fwInfo)+offsetof(fw_info_t,magic)+UPGRADE_STORE_ADDR;

    if(!upg_rw) {
        return -1;
    }
    
    r = upg_rw(1, addr, (U8*)fwmagic, sizeof(U32));
    
    return r;
}


int upgrade_get_fwinfo(fw_info_t *fwinfo)
{
    int r=-1;
    U32 addr=offsetof(paras_t,fwInfo)+UPGRADE_STORE_ADDR;
    
    if(!upg_rw) {
        return -1;
    }
    
    r = upg_rw(0, addr, (U8*)fwinfo, sizeof(fw_info_t));
    
    return r;
}


int upgrade_set_fwinfo(fw_info_t *fwinfo)
{
    int r=-1;
    U32 addr=offsetof(paras_t,fwInfo)+UPGRADE_STORE_ADDR;

    if(!upg_rw) {
        return -1;
    }
    
    r = upg_rw(1, addr, (U8*)fwinfo, sizeof(fw_info_t));
    
    return r;
}

static U32 upgrade_addr=0;
int upgrade_prep(U8 obj, U32 fwlen)
{
    int r;
    U32 from=(obj==OBJ_BOOT)?BOOT_OFFSET:APP_OFFSET;
    U32 to=from+fwlen;
    
    r = flash_erase(from, to);
    if(r==0) {
        upgrade_addr = from;
    }
    
    return r;
}

extern paras_t DEFAULT_PARAS;
int upgrade_check(void)
{
    fw_info_t fwInfo;
    upgrade_get_fwinfo(&fwInfo);
    if(fwInfo.magic!=DEFAULT_PARAS.fwInfo.magic) {
        upgrade_set_fwinfo(&DEFAULT_PARAS.fwInfo);
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


