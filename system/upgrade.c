 #include <stddef.h>
 #include "at24cxx.h"
#ifndef _WIN32
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
    int r=0;

#ifndef _WIN32
    r = flash_write(upgrade_addr, data, len);
#endif
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


int upgrade_is_need(void)
{
    U32 fwMagic=FW_MAGIC;
    
    paras_get_fwmagic(&fwMagic);
    if(fwMagic!=FW_MAGIC) {
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
    static U16 upg_pid=0;
    U8  *ptr=(U8*)&upgHeader;
    upgrade_pkt_t *upg=(upgrade_pkt_t*)data;
    
    if(upg->pid==0) {
        upg_pid = 0;
        upgRecvedLen = 0;
        upgrade_init(upgHeader.upgCtl.obj);
    }
    
 /*   
    if(upgRecvedLen<sizeof(upgrade_hdr_t)) {
        if(upgRecvedLen+upg->dataLen<sizeof(upgrade_hdr_t)) {
            memcpy(ptr+upgRecvedLen, upg->data, upg->dataLen);
            upgDataLength += upg->dataLen;
        }
        else {
            memcpy(ptr+upgRecvedLen, upg->data, sizeof(upgrade_hdr_t)-upgRecvedLen);
            upgDataLength = sizeof(upgrade_hdr_t);
            
            upgrade_write_fwinfo(upgHeader.fwInfo);
        }
    }
    else {
        memcpy(ptr, upg->data, sizeof(upgrade_hdr_t)-upgRecvedLen);
        upgDataLength = sizeof(upgrade_hdr_t);
    
        upgrade_write_fwinfo(upgHeader.fwInfo);
    
        
    
    }
*/        
            
    if(upg->pid!=upg_pid) {
        return ERROR_FW_PKT_ID;
    }
    
    if(upg->pid==upg->pkts-1) {
        //upgrade finished
    }
    else {
        upg_pid++;
    }
    
    return r;
}


