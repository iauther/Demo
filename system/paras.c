#include "paras.h"
#include "date.h"
#include "nvm.h"
#include "cfg.h"
#include "fs.h"
#include "rtc.h"
#include "log.h"
#include "dal_rtc.h"
#include "json.h"

all_para_t allPara;
static int prev_state=-1;
////////////////////////////////////////////////////////////
int paras_load(void)
{
    int r=0;
    char *js;
    int jlen;
    fw_info_t fwinfo;
    handle_t h=NULL;
    int exist=0;
    
    allPara = DFLT_PARA;
    dal_rtc_get(&allPara.sys.stat.dt);
    
    fs_init();
    
    jlen = fs_lengthx(FILE_PARA);
    if(jlen>0) {
        js = malloc(jlen);
        if(!js) return -1;
        fs_readx(FILE_PARA, js, jlen);
        exist = 1;
    }
    else {
        js = (char*)all_para_json;
        jlen = strlen(js);
    }
    
    r = json_to_para(js, &allPara.usr);
    
    if(r==0) {
        if(exist)  free(js);
        else       fs_savex(FILE_PARA, js, jlen);
        
        get_date_string((char*)DFLT_PARA.sys.para.fwInfo.bldtime, allPara.sys.para.fwInfo.bldtime);
    }
    
    return r;
}


void paras_set_state(int state)
{
    prev_state = allPara.sys.stat.state;
    allPara.sys.stat.state = state;
}


int paras_get_state(void)
{
    return allPara.sys.stat.state;
}


int paras_state_restore(void)
{
    if(prev_state==-1) {
        return -1;
    }
    
    allPara.sys.stat.state = prev_state;
    
    return 0;
}


int paras_get_prev_state(void)
{
    return prev_state;
}



int paras_set_cali(U8 ch, cali_t *ca)
{
    if(ch>=CH_MAX || !ca) {
        return -1;
    }
    
    allPara.usr.ch[ch].cali = *ca;
    return 0;
}

int paras_set_coef(U8 ch, coef_t *cf)
{
    if(ch>=CH_MAX || !cf) {
        return -1;
    }
    
    allPara.usr.ch[ch].cali.coef = *cf;
    return 0;
}


adc_para_t* paras_get_adc_para(U8 ch)
{
    if(ch>=CH_MAX) {
        return NULL;
    }
    return &allPara.usr.ch[ch];
}


int paras_get_run_datetime(date_time_t *dt)
{
    if(!dt) {
        return -1;
    }
    
    *dt = allPara.sys.stat.dt;
    
    return 0;
}


int paras_reset(void)
{
    int r=0;
    //U32 addr=offsetof(paras_t, para);
    
    //curPara = DEFAULT_PARAS.para;
    //r = paras_write(addr, &curPara, sizeof(curPara));
    if(r==0) {
    //    get_date_string((char*)DEFAULT_PARAS.para.fwInfo.bldtime, curPara.fwInfo.bldtime);
    }
    
    return r;
}




