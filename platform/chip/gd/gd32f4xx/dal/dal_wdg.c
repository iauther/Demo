#include "gd32f4xx_fwdgt.h"
#include "dal_wdg.h"
#include "log.h"


int dal_wdg_init(U16 ms)
{
/*
    FWDGT��IRC32K����ʱ���¹���
    
    Count =T * (32 * pre) 
    T��      Ϊ���Ź���ʱʱ�䣬��λ��ms
    Count��  Ϊ���Ź��ļ���ֵ
    pre��    Ϊ���Ź���Ԥ��Ƶϵ��, FWDGT_PSC_DIV32���ʾ1/32
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