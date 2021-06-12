#include "paras.h"
#include "date.h"
#include "nvm.h"
#include "cfg.h"

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
        {0, 5},     //TYPE_STAT
        {1, 5},     //TYPE_ACK
        {1, 5},     //TYPE_SETT
        {0, 5},     //TYPE_PARAS
        {1, 5},     //TYPE_ERROR
        {1, 5},     //TYPE_UPGRADE
        {0, 5},     //TYPE_LEAP
    }
};

#ifdef OS_KERNEL
U8 curState=STAT_AUTO;
#else
U8 curState=STAT_UPGRADE;
#endif
paras_t curParas;


////////////////////////////////////////////////////////////
int paras_load(void)
{
    int r;
    
    r = paras_read(0, (U8*)&curParas, sizeof(curParas));
    if(r==0) {
        if(memcmp(&curParas, &DEFAULT_PARAS, sizeof(curParas))) {
            curParas = DEFAULT_PARAS;
            paras_write(0, &curParas, sizeof(curParas));
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
    fw_info_t fwInfo={0};

    return paras_write(0, &fwInfo, sizeof(fwInfo));
}


int paras_read(U32 addr, void *data, U32 len)
{
    return 0;// nvm_read(addr, data, len);
}

int paras_write(U32 addr, void *data, U32 len)
{
    return 0;// nvm_write(addr, data, len);
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




