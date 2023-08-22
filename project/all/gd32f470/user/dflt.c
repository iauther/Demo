#include "protocol.h"
#include "cfg.h"

#define STR1(x)         #x
#define STR(x)          STR1(x)
    
#define HOST            "iot-06z00cq4vq4hkzx.mqtt.iothub.aliyuncs.com"
#define PORT            1883

#ifdef PROD_V2
    #define PRD_KEY         "izenXXeJiFN"
    #define PRD_SECRET      "lxHU6ztsJTaeFHT3"

    #ifdef USE_LAB_1
    #define DEV_KEY         "lab_1"
    #define DEV_SECRET      "6f313fec2995a652164da48ac4eb71cc"
    #else
    #define DEV_KEY         "lab_2"
    #define DEV_SECRET      "2ee6fb3372ae139c48da2fc569f50670"
    #endif
#elif defined PROD_V3
    #define PRD_KEY         "izenKGgdF0A"
    #define PRD_SECRET      "gnAqaOm9UE4JTpJc"

    #ifdef USE_LAB_1
    #define DEV_KEY         "lab_1"
    #define DEV_SECRET      "a53fff307f9da16b629868f5232adec7"
    #else
    #define DEV_KEY         "lab_2"
    #define DEV_SECRET      "4506649b902d8f747f73d3e6872fbb23"
    #endif
#endif
      

const all_para_t DFLT_PARA={
    
    .sys = {
        .stat={
            .mode=0,
            .state=STAT_STOP,
        },
        
        .para={
            .fwInfo={
                .magic=FW_MAGIC,
                .version=VERSION,
                .bldtime=__DATE__,
            },
            
            .sett = {
                //.mode = 0,
            }
        }
    },
    
    .usr = {
        
        .net = {
            .host      = HOST,
            .port      = PORT,
            .prdKey    = PRD_KEY,
            .prdSecret = PRD_SECRET,
            .devKey    = DEV_KEY,
            .devSecret = DEV_SECRET,
        },
        
        .mbus = {
            .addr = 0x02,
        },
        
        .card = {
            .type = 0,
            .auth = 1,
            .apn  = "CMNET",
        },
        
        .ch = {
            {
                .ch         = 0,
                .smpFreq    = 1000000,
                .smpTime    = 20,
                .ev         = {0,1,2,3},
                .n_ev       = 4,
                .upway      = 0,
                .upwav      = 0,
                .coef       = {
                    .a      = 1.0F,
                    .b      = 0.0F,
                }
            },
            
            {
                .ch         = 1,
                .smpFreq    = 10000,
                .smpTime    = 20,
                .ev         = {0,1,2,3},
                .n_ev       = 4,
                .upway      = 0,
                .upwav      = 0,
                .coef       = {
                    .a      = 100.0F,
                    .b      = 0.0F,
                }
            },
        },
        
        .dbg = {
            .to     = 0,
            .level  = 1,
            .enable = 1,
        },
        
        .smp = {
            .mode   = 0,
            .period = 3600*3,
        },
        
        .dac = {
            .enable = 0,
            .fdiv   = 1,
        }
    }
};




const char *all_para_json="{\
    \"netPara\":\
         {\
            \"host\":           "STR(HOST)",        \
            \"port\":           "STR(PORT)",        \
            \"productKey\":     "STR(PRD_KEY)",     \
            \"productSecret\":  "STR(PRD_SECRET)",  \
            \"deviceKey\":      "STR(DEV_KEY)",     \
            \"deviceSecret\":   "STR(DEV_SECRET)"   \
         },\
    \"mbPara\":\
         {\
            \"addr\":           20             \
         },\
    \"cardPara\":\
         {\
            \"type\":           0,             \
            \"auth\":           1,             \
            \"apn\":            \"CMNET\"      \
         },\
    \"smpPara\":\
         {\
             \"mode\":           0,            \
             \"period\":         \"10m\"       \
         },\
    \"chPara\":[\
         {\
            \"ch\":             0,             \
            \"smpFreq\":        1000000,       \
            \"smpTime\":        20,            \
            \"ev\":             [0,1,2,3],     \
            \"evCalcLen\":      1000,          \
            \"upway\":          0,             \
            \"upwav\":          0              \
        },\
        {\
            \"ch\":             1,             \
            \"smpFreq\":        1000000,       \
            \"smpTime\":        20,            \
            \"ev\":             [1],           \
            \"evCalcLen\":      1000,          \
            \"upway\":          0,             \
            \"upwav\":          0              \
        }],\
    \"dbgPara\":\
        {\
            \"to\":             0,             \
            \"level\":          8,             \
            \"enable\":         1              \
        },\
    \"dacPara\":\
        {\
            \"enable\":         0,             \
            \"fdiv\":           1              \
        }\
}";


const char *all_para_ini="\
\
\
";


const char *filesPath[FILE_MAX]={
    SFLASH_MNT_PT"/cfg/para.bin",
    SFLASH_MNT_PT"/cfg/cali.bin",
    SDMMC_MNT_PT"/xxxx",
    
    SFLASH_MNT_PT"/upg/app.upg",
    SFLASH_MNT_PT"/log/run.log",
};



