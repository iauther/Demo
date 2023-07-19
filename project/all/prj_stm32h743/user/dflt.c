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


char *chJson="{\
    \"chParams\":[\
         {\
            \"chID\":           0,      \
            \"caliParaA\":      4.7,    \
            \"caliParaB\":      0,      \
            \"coef\":           1.0,    \
            \"freq\":           8,      \
            \"sampleTime\":     1,      \
            \"samplePoints\":   1000    \
        },\
        {\
            \"chID\":           1,      \
            \"caliParaA\":      5.3,    \
            \"caliParaB\":      0,      \
            \"coef\":           1.0,    \
            \"freq\":           8,      \
            \"sampleTime\":     1,      \
            \"samplePoints\":   1000    \
        },\
        {\
            \"chID\":           8,      \
            \"caliParaA\":      2.9,    \
            \"caliParaB\":      0,      \
            \"coef\":           1.0,    \
            \"freq\":           8,      \
            \"sampleTime\":     1,      \
            \"samplePoints\":   1000    \
        },\
        {\
            \"chID\":           9,      \
            \"caliParaA\":      1.3,    \
            \"caliParaB\":      0,      \
            \"coef\":           1.0,    \
            \"freq\":           8,      \
            \"sampleTime\":     1,      \
            \"samplePoints\":   1000    \
        }]\
}";




