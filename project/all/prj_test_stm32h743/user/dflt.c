#include "protocol.h"
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











