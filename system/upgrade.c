#include <stddef.h>
#include "at24cxx.h"
#ifndef _WIN32
#include "board.h"
#include "drv/jump.h"
#include "drv/flash.h"
#include "notice.h"
#endif
#include "myCfg.h"
#include "data.h"
#include "paras.h"
#include "upgrade.h"

static U32 upgrade_addr=0;
static int upgrade_start(U8 goal)
{
    int r=0;
    
    if(goal==GOAL_BOOT) {
        upgrade_addr = BOOT_OFFSET;
    }
    else {
        upgrade_addr = APP_OFFSET;
    }
    
#ifndef _WIN32
    r = flash_init();
#endif

    return r;
}


static int upgrade_write(U8 *data, U32 len)
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
    int r;
    U32 fwMagic;
    
    r = paras_get_fwmagic(&fwMagic);
    if(r==0 && fwMagic==UPG_MAGIC) {
        return 1;
    }
    
    return 0;
}


static U8 upgFinished=0;
static U32 upgRecvedLen=0;
static U16 upgCurPktId=0;
static upgrade_hdr_t upgHeader;
U8 upgrade_proc(void *data)
{
    int r=0;
    U8  err=0;
    U8  *pHdr=(U8*)&upgHeader;
    upgrade_pkt_t *upg=(upgrade_pkt_t*)data;

    if(upg->dataLen==0) {
        return ERROR_PKT_LENGTH;
    }
    
    if(upg->pkts==0 || (upg->pid>0 && upg->pkts>0 && upg->pid>=upg->pkts)) {
        return ERROR_PKT_ID;
    }
    
    if(upg->pid==0) {
        upgCurPktId = 0;
        upgFinished = 0;
        upgRecvedLen = 0;
        
#ifndef _WIN32
        notice_start(DEV_LED, LEV_UPGRADE);
#endif
    }
    
    if(upg->pid!=upgCurPktId) {
        return ERROR_FW_PKT_ID;
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
            upgrade_start(upgHeader.upgCtl.goal);
            upgrade_write(upg->data+len, upg->dataLen-len);
        }
    }
    else {
        upgrade_write(upg->data, upg->dataLen);
    }
    upgCurPktId++;
    
    if(upg->pid==upg->pkts-1) {
        upgFinished = 1;
#ifndef _WIN32
        notice_stop(DEV_LED);
#endif
    }
    
    return r;
}



U8 upgrade_is_finished(void)
{
    return upgFinished;
}

