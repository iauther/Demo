#include "data.h"
#include "cfg.h"


const setts_t DEFAULT_SETTS={
    
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
};

const fw_info_t FW_INFO={
    .magic=FW_MAGIC,
    .version=VERSION,
    .bldtime=__DATE__,
    .totalLen=0,
};


setts_t curSetts;


