#include "task.h"
#include "fs.h"
#include "dal.h"
#include "dal_sd.h"
#include "upgrade.h"


#define POLL_MS         1000

#define MOUNT_DIR       "/sd"
#define UPG_FILE        "/sd/app.upg"
#define TMP_LEN         (0x100*8)

#ifdef OS_KERNEL

static void upgrade_tmr_callback(void *arg)
{
    task_post(TASK_UPGRADE, NULL, EVT_TIMER, 0, NULL, 0);
}


static int upg_file_is_exist(void)
{
    int r,flen;
    handle_t fl,fs;
    fw_info_t info1,info2;
    int upg_file_exist=0;
    
    r = dal_sd_status();
    if(r==0) {  //sd exist
        
        fl = fs_open(fs, UPG_FILE, FS_MODE_RW);
        if(fl) {
            flen = fs_size(fl);
            if(flen>0) {
                
                fs_read(fl, &info1, sizeof(info1));
                upgrade_get_fw_info(&info2);
        
                if(memcmp(&info1, &info2, sizeof(fw_info_t))) {
                    upg_file_exist = 1;
                }   
            }
            
            fs_close(fl);
        }
    }
    
    return upg_file_exist;
}

static void upgrade_proc(U8 loop)
{
    int r,index=0;
    int is_exist=0;
    U8 *pbuf=NULL;
    handle_t fl,fs;
    static int upg_chk_flag=0;
    int rlen,flen,upg_ok_flag=0;
    
    if(!upg_chk_flag) {
        is_exist = upg_file_is_exist();
        if(!is_exist) {
            return;
        }
        
        if(!loop) {
            upg_chk_flag = 1;
        }
    }
        
    pbuf = (U8*)malloc(TMP_LEN);
    if(!pbuf) {
        LOGE("____ upgrade_proc malloc failed\n");
        return;
    }
        
    fl = fs_open(fs, UPG_FILE, FS_MODE_RW);
    if(!fl) {
        LOGE("____ %s open failed\n", UPG_FILE);
        goto fail;
    }
    
    flen = fs_size(fl);
    if(flen<=0) {
        LOGE("____ upgrade file len wrong\n");
        goto fail;
    }
        
    LOGD("____ upgrade file len: %d\n", flen);
    upgrade_erase(flen);
    
    while(1) {
        rlen = fs_read(fl, pbuf, TMP_LEN);
        //LOGD("_____%d, 0x%02x, 0x%02x, \n", rlen, tmp[0], tmp[1]);
        if(rlen>0) {
            if(rlen<TMP_LEN) {
                r = upgrade_write(pbuf, rlen, -1);
                if(r) {
                    LOGE("____ upgrade failed\n");
                }
                else {
                    LOGD("____ upgrade ok\n");
                    upg_ok_flag = 1;
                }
                break;
            }
            
            r = upgrade_write(pbuf, rlen, index++);
            if(r) {
                LOGE("____ upgrade write failed, index: %d\n", index);
                break;
            }
        }
        else {
            LOGE("____ fs_read error\n");
            break;
        }
    }
    fs_close(fl);
    
    if(upg_ok_flag) {
        LOGD("____ reboot now ...\n\n\n");
        dal_reboot();
    }
    
fail:
    free(pbuf);
    fs_close(fl);
}



void task_upgrade_fn(void *arg)
{
    int i,r;
    U8  err;
    evt_t e;
    osTimerId_t tmrId;
    task_handle_t *h=(task_handle_t*)arg;
    
    LOGD("_____ task_upgrade running\n");
    
    task_tmr_start(TASK_UPGRADE, upgrade_tmr_callback, NULL, POLL_MS, TIME_INFINITE);
    while(1) {
        r = task_recv(TASK_UPGRADE, &e, sizeof(e));
        if(r==0) {
            switch(e.evt) {
                
                case EVT_TIMER:
                {
                    upgrade_proc(0);
                }
                break;
                
                default:
                continue;
            }
        }
    }
}
#endif


