#include "gd32f4xx_fwdgt.h"
#include "dal_wdg.h"
#include "log.h"
#include "cfg.h"


int dal_wdg_init(U16 ms)
{
/*
    FWDGT��IRC32K����ʱ���¹���
    
    Count = T * (32 * pre) 
    T��      Ϊ���Ź���ʱʱ�䣬��λ��ms
    Count��  Ϊ���Ź��ļ���ֵ
    pre��    Ϊ���Ź���Ԥ��Ƶϵ��, FWDGT_PSC_DIV32���ʾ1/32
*/
    if(ms>32760) {
        LOGE("___ dal_wdg_init, wrong para, range: 1~4096\n");
        return -1;
    }
    
    if(ms<=4095) {
        fwdgt_config(ms, FWDGT_PSC_DIV32);
    }
    else {
        U16 val=ms/(256/32);
        fwdgt_config(val, FWDGT_PSC_DIV256);
    }
    fwdgt_enable();
    
    return 0;
}


int dal_wdg_feed(void)
{
#ifdef USE_WDG
    fwdgt_counter_reload();
#endif
    
    return 0;
}



