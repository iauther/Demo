#include "paras.h"
#include "cfg.h"
#include "date.h"
#include "drv/flash.h"
#include "at24cxx.h"


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
        {1, 5},     //TYPE_CMD
        {1, 5},     //TYPE_STAT
        {1, 5},     //TYPE_ACK
        {1, 5},     //TYPE_SETT
        {1, 5},     //TYPE_PARAS
        {1, 5},     //TYPE_ERROR
        {1, 5},     //TYPE_UPGRADE
        {1, 5},     //TYPE_LEAP
    }
};


U8 curState=STAT_AUTO;
paras_t curParas;


////////////////////////////////////////////////////////
typedef int (*upg_rw_t)(U8 rw, U32 addr, U8 *data, U32 len);
static int flash_rw(U8 rw, U32 addr, U8 *data, U32 len)
{
    if(rw==0) {
        return flash_read(addr, data, len);
    }
    else {
        return flash_write(addr, data, len);
    }
}
static int at24_rw(U8 rw, U32 addr, U8 *data, U32 len)
{
    if(rw==0) {
        return at24cxx_read(addr, data, len);
    }
    else {
        return at24cxx_write(addr, data, len);
    }
}

#ifdef USE_EEPROM
static upg_rw_t upg_rw=at24_rw;
#else
static upg_rw_t upg_rw=flash_rw;
#endif

////////////////////////////////////////////////////////////
int paras_load(void)
{
    int r;
    date_t date;
    
#ifdef USE_EEPROM
    at24cxx_init();
#else
    flash_init();
#endif
    
    r = paras_read(0, (U8*)&curParas, sizeof(curParas));
    if(r==0) {
        if(memcmp(&curParas, &DEFAULT_PARAS, sizeof(curParas))) {
            curParas = DEFAULT_PARAS;
            paras_write(0, &curParas, sizeof(curParas));
        }
    }
    
    return r;
}

int paras_read(U32 addr, void *data, U32 len)
{
    int r;
    
#ifdef USE_EEPROM
    r = at24cxx_read(addr, (U8*)&curParas, sizeof(curParas));
#else
    r = flash_read(addr, (U8*)&curParas, sizeof(curParas));
#endif
    
    return r;
}

int paras_write(U32 addr, void *data, U32 len)
{
    int r;
    
#ifdef USE_EEPROM
    r = at24cxx_write(addr, (U8*)&curParas, sizeof(curParas));
#else
    r = flash_write(addr, (U8*)&curParas, sizeof(curParas));
#endif
    
    return r;
}


int paras_write_node(node_t *n)
{
    U32 addr;
    
    if(!n) {
        return -1;
    }
    
    addr=((U8*)n->ptr-(U8*)&curParas);
    return paras_write(addr, n->ptr, n->len);
}


int paras_get_fwmagic(U32 *fwmagic)
{
    int r=-1;
    U32 addr=offsetof(paras_t,fwInfo)+offsetof(fw_info_t,magic)+UPGRADE_INFO_ADDR;

    if(!upg_rw) {
        return -1;
    }
    
    r = paras_read(addr, (U8*)fwmagic, sizeof(U32));
    
    return r;
}


int paras_set_fwmagic(U32 *fwmagic)
{
    int r=-1;
    U32 addr=offsetof(paras_t,fwInfo)+offsetof(fw_info_t,magic)+UPGRADE_INFO_ADDR;

    if(!upg_rw) {
        return -1;
    }
    
    r = paras_write(addr, (U8*)fwmagic, sizeof(U32));
    
    return r;
}


int paras_get_fwinfo(fw_info_t *fwinfo)
{
    int r=-1;
    U32 addr=offsetof(paras_t,fwInfo)+UPGRADE_INFO_ADDR;
    
    if(!upg_rw) {
        return -1;
    }
    
    r = paras_read(addr, (U8*)fwinfo, sizeof(fw_info_t));
    
    return r;
}


int paras_set_fwinfo(fw_info_t *fwinfo)
{
    int r=-1;
    U32 addr=offsetof(paras_t,fwInfo)+UPGRADE_INFO_ADDR;

    if(!upg_rw) {
        return -1;
    }
    
    r = paras_write(addr, (U8*)fwinfo, sizeof(fw_info_t));
    
    return r;
}




