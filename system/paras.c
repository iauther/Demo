#include "paras.h"
#include "date.h"
#include "nvm.h"
#include "cfg.h"

#ifdef OS_KERNEL
U8 sysState=STAT_STOP;
#else
U8 sysState=STAT_UPGRADE;
#endif

state_t curStat;
para_t  curPara;


////////////////////////////////////////////////////////////
int paras_load(void)
{
    int r=0;
    fw_info_t fwinfo;
    
    if(sysState!=STAT_UPGRADE) {
        r = paras_get_fwinfo(&fwinfo);
        if(r==0) {
            if(fwinfo.magic!=FW_MAGIC) {
                curPara = DEFAULT_PARAS.para;
                r = paras_write(FILE_SETT, &DEFAULT_PARAS, sizeof(DEFAULT_PARAS));
            }
            else {
                
                if(memcmp(fwinfo.version, DEFAULT_PARAS.para.fwInfo.version, sizeof(fwinfo)-sizeof(fwinfo.magic))) {
                    paras_set_fwinfo(&DEFAULT_PARAS.para.fwInfo);
                }
                
                r = paras_read(offsetof(paras_t, para), &curPara, sizeof(curPara));
            }
        }
        else {
            curPara = DEFAULT_PARAS.para;
        }
        get_date_string((char*)DEFAULT_PARAS.para.fwInfo.bldtime, curPara.fwInfo.bldtime);
    }
    
    return r;
}


int paras_erase(void)
{
    paras_t paras={0};

    return paras_write(FILE_SETT, &paras, sizeof(paras));
}


int paras_reset(void)
{
    int r;
    U32 addr=offsetof(paras_t, para);
    
    curPara = DEFAULT_PARAS.para;
    r = paras_write(addr, &curPara, sizeof(curPara));
    if(r==0) {
        get_date_string((char*)DEFAULT_PARAS.para.fwInfo.bldtime, curPara.fwInfo.bldtime);
    }
    
    return r;
}



int paras_read(int id, void *data, int len)
{
    return nvm_read(id, data, len);
}


int paras_write(int id, void *data, int len)
{
    return nvm_write(id, data, len);
}



int paras_get_fwinfo(fw_info_t *fwinfo)
{
    int r=-1;
    para_t *p=malloc(sizeof(para_t));
    
    if (!fwinfo || !p) {
        return -1;
    }

    r = paras_read(FILE_SETT, p, sizeof(para_t));
    if(r==0) {
        *fwinfo = p->fwInfo;
    }
    free(p);
    
    return r;
}


int paras_set_fwinfo(fw_info_t *fwinfo)
{
    int r=-1;
    para_t *p=malloc(sizeof(para_t));

    if (!fwinfo || !p) {
        return -1;
    }
    
    r = paras_read(FILE_SETT, p, sizeof(para_t));
    if(r==0) {
        p->fwInfo = *fwinfo;
        r = paras_write(FILE_SETT, p, sizeof(para_t));
    }
    free(p);
    
    return r;
}


int paras_get_state(U8 *state)
{
    int r;
    para_t *p=malloc(sizeof(para_t));
    
    if(!state || !p) {
        return -1;
    }
    
    r = paras_read(FILE_SETT, p, sizeof(para_t));
    if(r==0) {
        *state = p->stat.sysState;
    }
    free(p);
    
    return r;
}


int paras_set_state(U8 state)
{
    int r;
    para_t *p=malloc(sizeof(para_t));
    
    if(!state || !p) {
        return -1;
    }
    
    r = paras_read(FILE_SETT, p, sizeof(para_t));
    if(r==0 && p->stat.sysState!=state) {
        r = paras_write(FILE_SETT, p, sizeof(para_t));
    }
    free(p);
    
    return r;
}



