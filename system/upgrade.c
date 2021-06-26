#include <stddef.h>
#include "at24cxx.h"
#ifndef _WIN32
#include "drv/jump.h"
#include "drv/flash.h"
#endif
#include "myCfg.h"
#include "data.h"
#include "paras.h"
#include "upgrade.h"

static U32 upgrade_addr=0;
int upgrade_init(U8 obj)
{
    int r=0;
    U32 from=(obj==OBJ_BOOT)?BOOT_OFFSET:APP_OFFSET;
    
    upgrade_addr = from;
#ifndef _WIN32
    r = flash_init();
#endif

    return r;
}


int upgrade_write(U8 *data, U32 len)
{
    int r=0;

#ifndef _WIN32
    r = flash_write(upgrade_addr, data, len);
    if(r==0) {
        upgrade_addr += len;
    }
#endif
    
    return r;
}


int upgrade_is_need(void)
{
    U32 fwMagic;
    
    paras_get_fwmagic(&fwMagic);
    if(fwMagic==UPG_MAGIC) {
        return 1;
    }
    
    return 0;
}



static U32 upgRecvedLen=0;
static upgrade_hdr_t upgHeader;
U8 upgrade_proc(void *data)
{
    int r=0;
    U8  err=0;
    static U16 upg_pkt_pid=0;
    U8  *pHdr=(U8*)&upgHeader;
    upgrade_pkt_t *upg=(upgrade_pkt_t*)data;

#ifdef OS_KERNEL
    if(upg->dataLen==0) {

        paras_set_upg();
        reboot();               //jump_to_boot();
        return 0;
    }
#else
    
    if(upg->dataLen==0) {
        return 0;
    }
    
    if(upg->pid==0) {
        upg_pkt_pid = 0;
        upgRecvedLen = 0;
    }
    
    if(upgRecvedLen<sizeof(upgrade_hdr_t)) {
        if(upgRecvedLen+upg->dataLen < sizeof(upgrade_hdr_t)) {
            memcpy(pHdr+upgRecvedLen, upg->data, upg->dataLen);
            upgRecvedLen += upg->dataLen;
        }
        else {
            int len = sizeof(upgrade_hdr_t)-upgRecvedLen;
            memcpy(pHdr+upgRecvedLen, upg->data, len);
            upgRecvedLen = sizeof(upgrade_hdr_t);
            
            if(upgHeader.upgCtl.erase>0) {
                paras_erase();
            }
            upgrade_init(upgHeader.upgCtl.obj);
            upgrade_write(upg->data+len, upg->dataLen-len);
        }
    }
            
    if(upg->pid!=upg_pkt_pid) {
        return ERROR_FW_PKT_ID;
    }
    
    upgrade_write(upg->data, upg->dataLen);
    upg_pkt_pid++;
    
    if(upg->pid==upg->pkts-1) {

    #ifndef _WIN32
        jump_to_app();
    #endif
    }
    
#endif
    
    return r;
}


