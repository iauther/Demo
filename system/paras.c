#include "paras.h"
#include "date.h"
#include "nvm.h"
#include "cfg.h"
#include "fs.h"
#include "rtc.h"
#include "log.h"
#include "dal.h"
#include "dal_rtc.h"
#include "json.h"

all_para_t allPara;
////////////////////////////////////////////////////////////
static int paras_check()
{
    
    return 0;
}

int paras_load(void)
{
    int i,r=0;
    int flen;
    handle_t h=NULL;
    fw_info_t *fw=&allPara.sys.fwInfo;
    
    allPara = DFLT_PARA;
    
#if 0
    flen = fs_lengthx(FILE_PARA);
    if(flen==sizeof(allPara)) {
        r = fs_readx(FILE_PARA, &allPara, flen);
    }
    else {
        paras_save();
    }
#endif
    
    dal_rtc_get(&allPara.var.state.dt);
    allPara.sys.devInfo.devID = dal_get_chipid();
    get_datetime_str(fw->bldtime, sizeof(fw->bldtime));
    
    allPara.var.psrc = rtc2_get_psrc();
    for(i=0; i<CH_MAX; i++) {
        allPara.var.state.stat[i] = STAT_STOP;
        allPara.var.state.finished[i] = 0;
    }
    
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



int paras_set_state(U8 ch, U8 stat)
{
    if(stat == allPara.var.state.stat[ch]) {
        return -1;
    }
    
    allPara.var.state.stat[ch] = stat;
    return 0;
}


int paras_get_state(U8 ch)
{
    return allPara.var.state.stat[ch];
}


int paras_set_finished(U8 ch, U8 f)
{
    allPara.var.state.finished[ch] = f;
    return 0;
}


int paras_is_finished(U8 ch)
{
    if(!allPara.usr.smp.ch[ch].enable || (allPara.usr.smp.ch[ch].enable && allPara.var.state.finished[ch])) {
        return 1;
    }
    
    return 0;
}


int paras_set_mode(U8 mode)
{
    allPara.usr.smp.mode = mode;
    return 0;
}


U8 paras_get_mode(void)
{
    return allPara.usr.smp.mode;
}



int paras_set_cali_sig(U8 ch, cali_sig_t *sig)
{
    cali_t *cali=NULL;
    
    if(ch>=CH_MAX || !sig) {
        return -1;
    }
    
    cali = &allPara.var.cali[ch];
    cali->sig = *sig;
    if(sig->tms>1) {
        if(cali->cnt>1) {
            cali->cnt = 0;
        }
        
        cali->rms[cali->cnt].in = sig->rms;
        cali->cnt++;
    }
    
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
    
    allPara.usr.smp.ch[ch].coef = *coef;
    return 0;
}


ch_para_t* paras_get_ch_para(U8 ch)
{
    if(ch>=CH_MAX) {
        return NULL;
    }
    U8 mode=allPara.usr.smp.mode;
    ch_paras_t *p=&allPara.usr.smp.ch[ch];
    
    return &p->para[mode];
}


ch_paras_t* paras_get_ch_paras(U8 ch)
{
    if(ch>=CH_MAX) {
        return NULL;
    }
    return &allPara.usr.smp.ch[ch];
}

int paras_get_smp_cnt(U8 ch)
{
    ch_para_t *pch=paras_get_ch_para(ch);
    
    if(!pch) {
        return -1;
    }
    
    return pch->smpPoints;
}


U8 paras_get_port(void)
{
    return allPara.usr.smp.port;
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


int paras_factory(void)
{
    allPara = DFLT_PARA;
    
    return paras_save();
}


