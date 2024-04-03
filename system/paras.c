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
static inline int isVaildChar(char c){         
    if(c >= 'A' && c <='F')      
        return 0;
    if(c >= 'a' && c <='f')
        return 0;
    if(c >= '0' && c <='9')
        return 0;
    return -1;           
}
 
static int is_ipv4(char *ip)
{
    char *tmpIp = ip;
    int sectorNum = 0;  //用于存储每一段字符的十进制值
    int sector = 0;     //用于存储输入的IP有多少段
    do{                 //do while才能判断到结束符
        if(*tmpIp == '.' || *tmpIp == '\0'){                    //完成一小段的字符获取后，进入判断，是否到末尾以及该段字符是否合法。
            if(sectorNum > 255) return -1;                      //如果该段字符十进制大于255，非法
            sectorNum = 0;                                      //该段字符合法，  清空sectorNum，为下段字符做准备
            sector++;                                           //IP段+1
            if(*tmpIp == '\0') break;                           //如果已经到达末尾，则跳出循环。
            tmpIp++;                                            //跳过.
        }
        if(*tmpIp >= '0' && *tmpIp <= '9'){                     //判断字符是否为数字
            if(sectorNum == 0 && *tmpIp == '0' && *(tmpIp+1) != '.')
                return -1;                                      //ipv4中每一段开头不能为0,否则非法
            sectorNum *= 10;                                    //正常字符以十进制存放在sectorNum
            sectorNum += (*tmpIp - '0');
        }else{                                                  //异常字符则退出函数
            return -1;
        }
    }while(*tmpIp++ != '\0');
 
    if(sector != 4)                                             //判断是否有4段，多或少都为非法
        return -1;
    return 0;
}
 
static int is_ipv6(char *ip)
{
    char *tmpIp = ip;           
    int sectorCount = 0;    //小段的字符数量，注意这里和v4不一样
    int sector = 0;         //用于存储输入的IP有多少段
 
    do{
        if(*tmpIp == ':' || *tmpIp == '\0'){
            if(sectorCount == 0 || sectorCount > 4) return -1;  //小段的字符数量为0或者大于4都是非法
            sectorCount = 0;                //清空小段字符数量，为下一次做准备
            sector++;                       //段+1
            if(*tmpIp == '\0') break;       //如果已经到达末尾，则跳出循环
            tmpIp++;                        //跳过:
        }
        if(isVaildChar(*tmpIp) == 0){       //判断字符是否为合法:0-9 a-f A-F
            sectorCount++;
        }else{                              //异常字符则退出函数
            return -1;
        }
    }while(*tmpIp++ != '\0');
 
    if(sector != 8)     //判断是否有8段，多或少都为非法
        return -1;
    return 0;
}
static int ip_is_valid(char* ip)
{
    int r=-1,flag=0;
    char *tmpIp = ip;                                
    
    while(*tmpIp != '\0'){                          //根据. :来区分是使用v4 v6的协议
        if(*tmpIp == '.'){
            flag = 4;
            break;
        }
        if(*tmpIp == ':'){
            flag = 6;
            break;
        }
        tmpIp++;
    }
 
    if(flag == 4){              //Ipv4解析
        r = is_ipv4(ip);
    }else if(flag == 6){        //Ipv6解析
        r = is_ipv6(ip);
    }
    
    return r;
}



static int usr_check(usr_para_t *usr)
{
    int i,j;
    
    net_para_t *net=&usr->net;
    if(net->mode>1) {
        LOGE("___ wrong net.mode, %d\n", net->mode);
        return -1;
    }
    
    if(net->mode==0) {
        if(net->para.tcp.port) {
            
        }
        
        if(strlen(net->para.tcp.ip)) {
            if(!ip_is_valid(net->para.tcp.ip)) {
                LOGE("___ wrong net.para.tcp.ip, %s\n", net->para.tcp.ip);
                return -1;
            }
        }
    }
    else {
        if(strlen(net->para.plat.host)==0 ||
           strlen(net->para.plat.prdKey)==0 ||
           strlen(net->para.plat.prdSecret)==0 ||
           strlen(net->para.plat.devKey)==0 ||
           strlen(net->para.plat.devSecret)==0) {
            LOGE("___ wrong net.plat string\n");
            return -1;
        }
           
        if(strcmp(usr->net.para.plat.devKey, allPara.usr.net.para.plat.devKey)) {
            LOGE("___ dev name not match\n");
            return -1;
        }
    }
    
    //////////////////////////////
    dac_para_t *dac=&usr->dac;
    if(dac->enable>1) {
        LOGE("___ wrong dac.enable: %d\n", dac->enable);
        return -1;
    }
    
    if(dac->fdiv==0) {
        LOGE("___ wrong dac.fdiv: %d\n", dac->fdiv);
        return -1;
    }
    
    /////////////////////////////
    dbg_para_t *dbg=&usr->dbg;
    if(dbg->enable>1) {
        LOGE("___ wrong dbg.enable: %d\n", dbg->enable);
        return -1;
    }
    
    if(dbg->level>=LV_MAX) {
        LOGE("___ wrong dbg.level: %d\n", dbg->level);
        return -1;
    }
    
    //mbus para check
    mbus_para_t *mbus=&usr->mbus;
    
    
    /////////////////////////////
    card_para_t *card=&usr->card;
    if(card->auth>2) {
        LOGE("___ wrong card.auth: %d\n", card->auth);
        return -1;
    }
   
    /////////////////////////////
    smp_para_t *smp=&usr->smp;
    if(smp->mode>=MODE_MAX) {
        LOGE("___ wrong smp.mode, %d\n", smp->mode);
        return -1;
    }
    
    if(smp->port) {
        
    }
    
    if(smp->pwrmode>PWR_PERIOD_PWRDN) {
        LOGE("___ wrong smp.pwrmode, %d\n", smp->pwrmode);
        return -1;
    }
    
    for(i=0; i<CH_MAX; i++) {
        ch_para_t *ch=&smp->ch[i];
        if(ch->ch!=i) {
            LOGE("___ wrong ch[%d].ch, %d\n", i, ch->ch);
            return -1;
        }
        
        if(ch->enable>1) {
            LOGE("___ wrong ch[%d].enable, %d\n", i, ch->enable);
            return -1;
        }
        
        if(ch->smpFreq>2500*KHZ || ch->smpFreq%1000) {
            LOGE("___ wrong ch[%d].smpFreq, %d\n", i, ch->smpFreq);
            return -1;
        }
        
        if(ch->smpMode>SMP_MODE_CONT) {
            LOGE("___ wrong ch[%d].enable, %d\n", i, ch->smpMode);
            return -1;
        }
        
        if(ch->smpPoints<1000 || ch->smpPoints>10000) {
            LOGE("___ wrong ch[%d].enable, %d\n", i, ch->smpPoints);
            return -1;
        }
        
        if(ch->smpTimes==0 || ch->smpTimes>100) {
            LOGE("___ wrong ch[%d].smpTimes, %d\n", i, ch->smpTimes);
            return -1;
        }
        
        if(ch->smpInterval>10000000) {
            LOGE("___ wrong ch[%d].smpInterval, %d\n", i, ch->enable);
            return -1;
        }
        
        if(ch->trigEvType!=EV_AMP) {
            LOGE("___ wrong ch[%d].trigEvType, %d\n", i, ch->trigEvType);
            return -1;
        }
        
        if(ch->trigThreshold>111) {
            LOGE("___ wrong ch[%d].trigThreshold, %d\n", i, ch->trigThreshold);
            return -1;
        }
        
        //trigTime check
        if(ch->trigTime.preTime>111) {
            
        }
        if(ch->trigTime.postTime>111) {
            
        }
        if(ch->trigTime.PDT>111) {
            
        }
        if(ch->trigTime.HDT>111) {
            
        }
        if(ch->trigTime.HLT>111) {
            
        }
        if(ch->trigTime.MDT>111) {
            
        }
        
        //
        if(ch->n_ev>EV_NUM) {
            LOGE("___ wrong ch[%d].n_ev, %d\n", i, ch->n_ev);
            return -1;
        }
        for(j=0; j<ch->n_ev; j++) {
            if(ch->ev[j]>=EV_NUM) {
                LOGE("___ wrong ch[%d].ev[%d]: %d\n", i, j, ch->ev[j]);
                return -1;
            }
        }
        
        if(ch->evCalcPoints>10000 || ch->evCalcPoints<1000) {
            LOGE("___ wrong ch[%d].evCalcPoints, %d\n", i, ch->evCalcPoints);
            return -1;
        }
        
        if(ch->coef.a) {
            
        }
    }
    
    return 0;
}



int paras_load(void)
{
    int i,r=-1;
    int flen,rlen;
    handle_t h=NULL;
    fw_info_t *fw=&allPara.sys.fwInfo;
    
    allPara = DFLT_PARA;
    LOGD("___fw version: %s\n", fw->version);
    
#ifndef DEV_MODE_DEBUG
    flen = fs_lengthx(FILE_PARA);
    if(flen==sizeof(allPara)) {
        rlen = fs_readx(FILE_PARA, &allPara, flen);
        if(rlen==flen) {
            r = usr_check(&allPara.usr);
            if(r) {
                allPara.usr = DFLT_PARA.usr;
            }
        }
    }
    
    if(r) {
        paras_save();
    }
#endif
    strcpy((char*)allPara.sys.fwInfo.version, FW_VERSION);
    
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
    ch_para_t *para=&allPara.usr.smp.ch[ch];
    if(!para->enable ||  (para->enable && (allPara.var.state.finished[ch] || para->smpMode!=SMP_MODE_PERIOD))) {
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
    
    allPara.usr.smp.mode = MODE_CALI;
    
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


int paras_is_period(void)
{
    U8 ch;
    int period=1;
    
    for(ch=0; ch<CH_MAX; ch++) {
        ch_para_t* para = paras_get_ch_para(ch);
        if(para->enable && (para->smpMode!=SMP_MODE_PERIOD)) {
            period = 0;
            break;
        }
    }
    
    return period;
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


cfg_file_t *paras_get_cfile(void)
{
    return &allPara.sys.cfile;
}

void paras_set_rtmin(U32 rtmin)
{
    allPara.var.wtime.rtmin = rtmin;
}

worktime_t paras_get_worktime(void)
{
    return allPara.var.wtime;
}

void paras_set_worktime(worktime_t wtime)
{    
    allPara.var.wtime = wtime;
}


char *paras_get_devname(void)
{
    if(allPara.usr.net.mode==0) {
        return NULL;
    }
    return allPara.usr.net.para.plat.devKey;
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


int paras_save_json(void *json, int len)
{
    int r;
    U8 takeff=0;
    usr_para_t *usr=NULL;
    all_para_t *p=NULL;
    
    if(len<=0) {
        return -1;
    }
    
    p = malloc(sizeof(all_para_t));
    if(p==NULL) {        
        return -1;
    }
    
    usr = malloc(sizeof(usr_para_t));
    if(usr==NULL) {
        free(p);
        return -1;
    }
    
    memcpy(p, &allPara, sizeof(all_para_t));
    memcpy(usr, &p->usr, sizeof(usr_para_t));
    
    r = json_to((char*)json, usr);
    if(r==0) {
        takeff = usr->takeff;
        usr->takeff = 0;
        
        r = usr_check(usr);
        if(r) {
            r = -1;
            goto quit;
        }
        
        if(usr->net.mode) {
            if(memcmp(usr, &p->usr, sizeof(usr_para_t))==0) {
                LOGW("___ config paras is same to the storage!\n");
                r = -1;
            }
            else {
                memcpy(&p->usr, usr, sizeof(usr_para_t));
                r = fs_savex(FILE_PARA, p, sizeof(all_para_t));
            }
        }
        else {
            memcpy(&p->usr, usr, sizeof(usr_para_t));
            r = fs_savex(FILE_PARA, p, sizeof(all_para_t));
        }
    }
    
quit:
    free(usr); 
    free(p);
    
    if(r==0 && takeff>0) {
        r = 1;
    }
    
    return r;
}



int paras_factory(void)
{
    allPara = DFLT_PARA;
    
    return paras_save();
}


int paras_check(all_para_t *all)
{
    int r;
    
    if(!all) {
        return -1;
    }
    
    r = usr_check(&all->usr);
     
    return r;
}



