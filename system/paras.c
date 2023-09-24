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
static int paras_check()
{
    
    return 0;
}

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
return 0;
    
    jlen = fs_lengthx(FILE_PARA);
    if(jlen>0) {
        js = malloc(jlen);
        if(!js) return -1;
        fs_readx(FILE_PARA, &allPara, jlen);
        free(js);
    }
    else {
        paras_save();
    }
    
    get_date_string((char*)DFLT_PARA.sys.para.fwInfo.bldtime, allPara.sys.para.fwInfo.bldtime);
    
    return r;
}


int paras_set_usr(usr_para_t *usr)
{
    if(!usr) {
        return -1;
    }
    
    allPara.usr = *usr;
    
    return 0;
}


smp_para_t* paras_get_smp(void)
{
    return &allPara.usr.smp;
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



int paras_set_cali_sig(U8 ch, cali_sig_t *sig)
{
    cali_t *cali=NULL;
    
    if(ch>=CH_MAX || !sig) {
        return -1;
    }
    
    cali = &allPara.var.cali[ch];
    if(cali->cnt>1) {
        cali->cnt = 0;
    }
    
    cali->sig = *sig;
    cali->rms[cali->cnt].in = sig->rms;
    cali->cnt++;

    
    return 0;
}


cali_t* paras_get_cali(U8 ch)
{
    if(ch>=CH_MAX) {
        return NULL;
    }
    
    return &allPara.var.cali[ch];
}


int paras_set_coef(U8 ch, coef_t *coef)
{
    if(ch>=CH_MAX || !coef) {
        return -1;
    }
    
    allPara.usr.ch[ch].coef = *coef;
    return 0;
}


ch_para_t* paras_get_ch_para(U8 ch)
{
    if(ch>=CH_MAX) {
        return NULL;
    }
    return &allPara.usr.ch[ch];
}


int paras_get_datetime(date_time_t *dt)
{
    if(!dt) {
        return -1;
    }
    
    *dt = allPara.sys.stat.dt;
    
    return 0;
}

int paras_get_smp_cnt(U8 ch)
{
    ch_para_t *pch=NULL;
    
    if(ch>=CH_MAX) {
        return -1;
    }
    pch = &allPara.usr.ch[ch];
    
    return pch->smpFreq*pch->smpTime/1000;
}



int paras_save(void)
{
    int r;
    all_para_t *p=malloc(sizeof(all_para_t));
    
    if(!p) {
        return -1;
    }
    
    p->sys = allPara.sys;
    p->usr = allPara.usr;
    p->var = DFLT_PARA.var;
    r = fs_savex(FILE_PARA, p, sizeof(all_para_t));
    free(p);
    
    return r;
}


int paras_reset(void)
{
    allPara = DFLT_PARA;
    
    return paras_save();
}



