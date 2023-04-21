#include "devs.h"
#include "incs.h"


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
    r = dal_flash_init();
#endif

    return r;
}


static int upgrade_write(U8 *data, U32 len)
{
    int r=0;

#ifndef _WIN32
    r = dal_flash_write(upgrade_addr, data, len);
    if(r==0) {
        upgrade_addr += len;
    }
#endif
    
    return r;
}


int upgrade_is_need(void)
{
    int r;
    U32 magic;
    
    r = paras_get_magic(&magic);
    if(r==0 && magic==UPG_MAGIC) {
        return 1;
    }
    
    return 0;
}


static U8 upgFinished=0;
static U32 upgRecvedLen=0;
static U16 upgCurPktId=0;
static upg_hdr_t upgHeader;
U8 upgrade_proc(void *data)
{
    int r=0;
    U8  err=0;
    U8  *pHdr=(U8*)&upgHeader;
    file_pkt_t *fp=(file_pkt_t*)data;

    if(fp->dataLen==0) {
        return ERROR_FW_PKT_LENGTH;
    }
    
    if(fp->pkts==0 || (fp->pid>0 && fp->pkts>0 && fp->pid>=fp->pkts)) {
        return ERROR_FW_PKT_ID;
    }
    
    if(fp->pid==0) {
        upgCurPktId = 0;
        upgFinished = 0;
        upgRecvedLen = 0;
        
#ifndef _WIN32
        //notice_start(DEV_LED, LEV_UPGRADE);
#endif
    }
    
    if(fp->pid!=upgCurPktId) {
        return ERROR_FW_PKT_ID;
    }
    
    if(upgRecvedLen<sizeof(upg_hdr_t)) {
        if(upgRecvedLen+fp->dataLen < sizeof(upg_hdr_t)) {
            memcpy(pHdr+upgRecvedLen, fp->data, fp->dataLen);
            upgRecvedLen += fp->dataLen;
        }
        else {
            int len = sizeof(upg_hdr_t)-upgRecvedLen;
            memcpy(pHdr+upgRecvedLen, fp->data, len);
            upgRecvedLen = sizeof(upg_hdr_t);
            
            if(upgHeader.upgCtl.erase>0) {
                paras_erase();
            }
            upgrade_start(upgHeader.upgCtl.goal);
            upgrade_write(fp->data+len, fp->dataLen-len);
        }
    }
    else {
        upgrade_write(fp->data, fp->dataLen);
    }
    upgCurPktId++;
    
    if(fp->pid==fp->pkts-1) {
        upgFinished = 1;
    }
    
    return r;
}



U8 upgrade_is_finished(void)
{
    return upgFinished;
}


U8 upgrade_is_app(void)
{
    return upgHeader.upgCtl.goal;
}



