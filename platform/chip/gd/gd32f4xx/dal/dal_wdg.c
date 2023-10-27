#include "gd32f4xx_fwdgt.h"
#include "dal_wdg.h"
#include "log.h"


int dal_wdg_init(U16 ms)
{
/*
    FWDGT在IRC32K独立时钟下工作
    
    Count =T * (32 * pre) 
    T：      为看门狗超时时间，单位：ms
    Count：  为看门狗的计数值
    pre：    为看门狗的预分频系数, FWDGT_PSC_DIV32则表示1/32
*/
    if(ms>4096) {
        LOGE("___ dal_wdg_init, wrong para, range: 1~4096\n");
        return -1;
    }
    
    fwdgt_config(ms, FWDGT_PSC_DIV32);
    fwdgt_enable();
    
    return 0;
}


int dal_wdg_feed(void)
{
    fwdgt_counter_reload();
    
    LOGD("___ dal_wdg_feed\n");
    return 0;
}