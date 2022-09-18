#include "paras.h"
#include "date.h"
#include "nvm.h"
#include "cfg.h"

paras_t DEFAULT_PARAS={
    
    .flag={
        .magic=FW_MAGIC,
        .state=STAT_STOP,
    },
    
    .para={
        .fwInfo={
            .magic=FW_MAGIC,
            .version=VERSION,
            .bldtime=__DATE__,
        },
        
        .sett = {
            .mode = 0,
        }
    }
};


#ifdef OS_KERNEL
U8 sysState=STAT_STOP;
#else
U8 sysState=STAT_UPGRADE;
#endif

stat_t  curStat;
para_t  curPara;


////////////////////////////////////////////////////////////
int paras_load(void)
{
    int r=0;
    fw_info_t fwinfo;
    
    if(sysState!=STAT_UPGRADE) {
        r = paras_get_fwinfo(&fwinfo);
        if(r==0) {
            if(fwinfo.magic!=FW_MAGIC && fwinfo.magic!=UPG_MAGIC) {
                curPara = DEFAULT_PARAS.para;
                r = paras_write(0, &DEFAULT_PARAS, sizeof(DEFAULT_PARAS));
            }
            else {
                
                paras_clr_upg();
                if(memcmp(fwinfo.version, DEFAULT_PARAS.para.fwInfo.version, sizeof(fwinfo)-sizeof(fwinfo.magic))) {
                    paras_set_fwinfo(&DEFAULT_PARAS.para.fwInfo);
                }
                
                r = paras_read(offsetof(paras_t, para), &curPara, sizeof(curPara));
                paras_get_state(&sysState);
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

    return paras_write(0, &paras, sizeof(paras));
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



int paras_read(U32 addr, void *data, U32 len)
{
#ifdef _WIN32
    return 0;
#else
    return nvm_read(addr, data, len);
#endif
}

int paras_write(U32 addr, void *data, U32 len)
{
#ifdef _WIN32
    return 0;
#else
    return nvm_write(addr, data, len);
#endif
}


int paras_write_node(node_t *n)
{
    U32 addr=offsetof(paras_t, para)+UPGRADE_INFO_ADDR;
    
    if(!n) {
        return -1;
    }
    
    addr += ((U32)n->ptr-(U32)&curPara);
    return paras_write(addr, n->ptr, n->len);
}


int paras_get_magic(U32 *magic)
{
    int r=-1;
    U32 addr=offsetof(flag_t, magic)+UPGRADE_INFO_ADDR;
    
    if (!magic) {
        return -1;
    }

    r = paras_read(addr, magic, sizeof(U32));
    
    return r;
}


int paras_set_magic(U32 *magic)
{
    int r=-1;
    U32 addr=offsetof(flag_t, magic)+UPGRADE_INFO_ADDR;
    
    if (!magic) {
        return -1;
    }

    r = paras_write(addr, magic, sizeof(U32));
    
    return r;
}


int paras_get_fwinfo(fw_info_t *fwinfo)
{
    int r=-1;
    U32 addr=offsetof(paras_t, para)+offsetof(para_t,fwInfo)+UPGRADE_INFO_ADDR;
    
    if (!fwinfo) {
        return -1;
    }

    r = paras_read(addr, fwinfo, sizeof(fw_info_t));
    
    return r;
}


int paras_set_fwinfo(fw_info_t *fwinfo)
{
    int r=-1;
    U32 addr=offsetof(paras_t, para)+offsetof(para_t,fwInfo)+UPGRADE_INFO_ADDR;

    if (!fwinfo) {
        return -1;
    }

    r = paras_write(addr, fwinfo->version, sizeof(fw_info_t)-sizeof(fwinfo->magic));
    
    return r;
}



int paras_set_upg(void)
{
    int r;
    U32 magic=0;
    
    r = paras_get_magic(&magic);
    if(r==0 && magic!=UPG_MAGIC) {
        magic = UPG_MAGIC;
        r = paras_set_magic(&magic);
    }
    
    return r;
}


int paras_clr_upg(void)
{
    int r;
    U32 magic=0;
    
    r = paras_get_magic(&magic);
    if(r==0 && magic==UPG_MAGIC) {
        magic = FW_MAGIC;
        r = paras_set_magic(&magic);
    }
    
    return r;
}


int paras_get_state(U8 *state)
{
    int r;
    U32 addr=offsetof(flag_t, state)+UPGRADE_INFO_ADDR;
    
    if(!state) {
        return -1;
    }
    
    r = paras_read(addr, state, sizeof(*state));
    
    return r;
}


int paras_set_state(U8 state)
{
    int r;
    U8  st;
    U32 addr=offsetof(flag_t, state)+UPGRADE_INFO_ADDR;
    
    r = paras_read(addr, &st, sizeof(st));
    if(r==0 && st!=state) {
        r = paras_write(addr, &state, sizeof(state));
    }
    
    return r;
}



