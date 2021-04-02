#include "data.h"
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


