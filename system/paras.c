#include "paras.h"
#include "datetime.h"
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

int paras_load(void)
{
    int i,r=0;
    int flen;
    handle_t h=NULL;
    fw_info_t *fw=&allPara.sys.fwInfo;
    
    allPara = DFLT_PARA;
    LOGD("___fw version: %s\n", fw->version);
    
#ifndef DEV_MODE_DEBUG
    flen = fs_lengthx(FILE_PARA);
    if(flen==sizeof(allPara)) {
        r = fs_readx(FILE_PARA, &allPara, flen);
    }
    else {
        paras_save();
    }
#endif
    
    dal_rtc_get_time(&allPara.var.state.dt);
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
    
    if(((sig->max<1)||(sig->max>CALI_MAX)) || ((sig->seq<1)||(sig->seq>CALI_MAX)) || 
        (sig->lv>LEVEL_VPP) || (sig->ch>=CH_MAX) || (sig->seq>sig->max)) {
        LOGE("___ cali sig para wrong, quit calibration\n");
        return -1;
    }
    
    cali = &allPara.var.cali[ch];
    cali->cnt = 0;
    cali->sig = *sig;
    cali->rms[sig->seq-1].in = (sig->lv==LEVEL_RMS)?sig->volt:sig->volt/(2*sqrt(2));
    
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
    return &allPara.usr.smp.ch[ch];
}




int paras_get_smp_points(U8 ch)
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
    
    p->usr.smp.mode = MODE_NORM;        //其它模式不保存
    r = fs_savex(FILE_PARA, p, sizeof(all_para_t));
    free(p);
    
    return r;
}


int paras_save_json(void *data, int len)
{
    int r;
    usr_para_t *usr=NULL;
    all_para_t *p=malloc(sizeof(all_para_t));
    
    if(!p || len<=0) {
        return -1;
    }
    
    usr = malloc(sizeof(usr_para_t));
    if(!usr) {
        free(p);
        return -1;
    }
    
    memcpy(p, &DFLT_PARA, sizeof(all_para_t));
    r = json_to((char*)data, usr);
    if(r==0) {
        if(memcmp(usr, &p->usr, sizeof(usr_para_t))) {
            r = fs_savex(FILE_PARA, p, sizeof(all_para_t));
        }
        else {
            r = -1;
        }
    }
    free(usr); free(p);
    
    return r;
}



int paras_factory(void)
{
    allPara = DFLT_PARA;
    
    return paras_save();
}


int paras_check(all_para_t *all)
{
    int i;
    
    if(!all) {
        return -1;
    }
    
    if(all->usr.card.apn>=0) {
        
    }
    
    //if(all->usr.card.auth>=0)
   
    
    ch_para_t *ch=all->usr.smp.ch;
    
    for(i=0; i<CH_MAX; i++) {
        if(ch[i].ch!=i) {
            
        }
    }
     
    return 0;
}



