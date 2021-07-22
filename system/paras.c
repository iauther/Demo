#include "paras.h"
#include "date.h"
#include "nvm.h"
#include "myCfg.h"

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
        
        .setts={
            .mode=MODE_CONTINUS,
            {
                {
                    .mode=MODE_CONTINUS,
                    .time={0,0,0},
                    .pres=90.0F,
                    .maxVol=100.0F
                 },
                
                 {
                    .mode=MODE_INTERVAL,
                    .time={0,0,0},
                    .pres=90.0F,
                    .maxVol=100.0F
                 },
                 
                 {
                    .mode=MODE_FIXED_TIME,
                    .time={0,0,0},
                    .pres=90.0F,
                    .maxVol=100.0F
                 },
                 
                 {
                    .mode=MODE_FIXED_VOLUME,
                    .time={0,0,0},
                    .pres=90.0F,
                    .maxVol=100.0F
                 },
            },
        }
    }
};


ack_timeout_t ackTimeout={
    {
        //enable      resendIvl         retryTimes
         {1,         ACK_POLL_MS*5,        2},       //TYPE_CMD
         {1,         ACK_POLL_MS*5,        2},       //TYPE_STAT
         {1,         ACK_POLL_MS*5,        2},       //TYPE_ACK
         {1,         ACK_POLL_MS*5,        2},       //TYPE_SETT
         {1,         ACK_POLL_MS*5,        -1},      //TYPE_PARAS
         {1,         ACK_POLL_MS*5,        -1},      //TYPE_ERROR
         {1,         ACK_POLL_MS*5,        2},       //TYPE_UPGRADE
         {1,         ACK_POLL_MS*10,       2},       //TYPE_LEAP
     }
};

notice_t allNotice[LEV_MAX]={
    {//warn
        //blink_color   stop_color      light_time      dark_time       total_time
        {BLUE,          BLANK,          100,            100,            5000},
        
        //ring_time     quiet_time      total_time
        {200,           200,            5000},
    },
    
    {//error
        //blink_color   stop_color      light_time      dark_time       total_time
        {BLUE,          BLANK,          100,            100,            5000},
        
        //ring_time     quiet_time      total_time
        {100,           100,            5000},
    },
    
    {//upgrade
        //blink_color   stop_color      light_time      dark_time       total_time
        {BLUE,          BLANK,          100,            100,            -1},
        
        //ring_time     quiet_time      total_time
        {30,            30,             -1},
    },
};



#ifdef OS_KERNEL
U8 adjMode=ADJ_AUTO;
U8 sysState=STAT_STOP;
#else
U8 adjMode=ADJ_MANUAL;
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



