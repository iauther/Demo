#include "paras.h"
#include "date.h"
#include "nvm.h"
#include "myCfg.h"

paras_t DEFAULT_PARAS={
    
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
paras_t curParas;


////////////////////////////////////////////////////////////
int paras_load(void)
{
    int r;
    U32 fwmagic=0;
    
    r = paras_get_fwmagic(&fwmagic);
    if(r==0) {
        if(fwmagic!=FW_MAGIC && fwmagic!=UPG_MAGIC) {
            curParas = DEFAULT_PARAS;
            r = paras_write(0, &curParas, sizeof(curParas));
        }
        else {
            if(fwmagic==UPG_MAGIC){
                fwmagic = FW_MAGIC;
                paras_set_fwmagic(&fwmagic);
            }
            r = paras_read(0, &curParas, sizeof(curParas));
        }
    }
    else {
        curParas = DEFAULT_PARAS;
    }
    get_date_string((char*)DEFAULT_PARAS.fwInfo.bldtime, curParas.fwInfo.bldtime);
    
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
    
    curParas = DEFAULT_PARAS;
    r = paras_write(0, &curParas, sizeof(curParas));
    if(r==0) {
        get_date_string((char*)DEFAULT_PARAS.fwInfo.bldtime, curParas.fwInfo.bldtime);
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
    U32 addr;
    
    if(!n) {
        return -1;
    }
    
    addr=((U32)n->ptr-(U32)&curParas);
    return paras_write(addr, n->ptr, n->len);
}


int paras_get_fwmagic(U32 *fwmagic)
{
    int r=-1;
    U32 addr=offsetof(paras_t,fwInfo)+offsetof(fw_info_t,magic)+UPGRADE_INFO_ADDR;
    
    if (!fwmagic) {
        return -1;
    }

    r = paras_read(addr, (U8*)fwmagic, sizeof(U32));
    
    return r;
}


int paras_set_fwmagic(U32 *fwmagic)
{
    int r=-1;
    U32 addr=offsetof(paras_t,fwInfo)+offsetof(fw_info_t,magic)+UPGRADE_INFO_ADDR;
    
    if (!fwmagic) {
        return -1;
    }

    r = paras_write(addr, (U8*)fwmagic, sizeof(U32));
    
    return r;
}


int paras_get_fwinfo(fw_info_t *fwinfo)
{
    int r=-1;
    U32 addr=offsetof(paras_t,fwInfo)+UPGRADE_INFO_ADDR;
    
    if (!fwinfo) {
        return -1;
    }

    r = paras_read(addr, (U8*)fwinfo, sizeof(fw_info_t));
    
    return r;
}


int paras_set_fwinfo(fw_info_t *fwinfo)
{
    int r=-1;
    U32 addr=offsetof(paras_t,fwInfo)+UPGRADE_INFO_ADDR;

    if (!fwinfo) {
        return -1;
    }

    r = paras_write(addr, (U8*)fwinfo, sizeof(fw_info_t));
    
    return r;
}



int paras_set_upg(void)
{
    int r;
    U32 fwMagic=0;
    
    r = paras_get_fwmagic(&fwMagic);
    if(r==0 && fwMagic!=UPG_MAGIC) {
        fwMagic = UPG_MAGIC;
        r = paras_set_fwmagic(&fwMagic);
    }
    
    return r;
}


int paras_clr_upg(void)
{
    int r;
    U32 fwMagic=0;
    
    r = paras_get_fwmagic(&fwMagic);
    if(r==0 && fwMagic==UPG_MAGIC) {
        fwMagic = UPG_MAGIC;
        r = paras_set_fwmagic(&fwMagic);
    }
    
    return r;
}






