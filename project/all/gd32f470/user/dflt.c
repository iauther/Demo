#include "protocol.h"
#include "net.h"
#include "cfg.h"

#define STR1(x)             #x
#define STR(x)              STR1(x)


#define HOST                "iot-06z00cq4vq4hkzx.mqtt.iothub.aliyuncs.com"
#define PORT                1883

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
        .fwInfo={
            .magic=FW_MAGIC,
            .version=VERSION,
            .bldtime=__DATE__,
        },
        
    },
    
    .usr = {
        
        .net = {
            .mode = 1,                  //默认连接平台
            .para = {
                .plat={
                    .host      = HOST,
                    .port      = PORT,
                    .prdKey    = PRD_KEY,
                    .prdSecret = PRD_SECRET,
                    .devKey    = DEV_KEY,
                    .devSecret = DEV_SECRET,
                }
            }
        },
        
        .mbus = {
            .addr = 0x02,
        },
        
        .card = {
            .type = 0,
            .auth = 1,
            .apn  = "CMNET",
        },
        
        .dbg = {
            .to     = 0,
            .level  = 1,
            .enable = 1,
        },
        
        .smp = {
            //.mode       = MODE_CALI,
            .mode       = MODE_TEST,
            .port       = PORT_NET,
            .dato       = DATO_ALI,
            .pwrmode    = PWR_NO_PWRDN,
            .worktime   = 60*10,
            
            .ch = {
                {   //CH_0
                    .ch         = 0,
                    .enable     = 1,
                
                    {
                        {   //normal
                            .smpMode      = SMP_PERIOD_MODE,
                            .smpFreq      = 1000000,
                            .smpPoints    = 10000,
                            .smpInterval  = 0,
                            .smpTimes     = 1,
                            .ampThreshold = 2.2f,
                            .messDuration = 5000,
                            .trigDelay    = 30000,
                            .ev           = {0,1,2,3},
                            .n_ev         = 4,
                            .evCalcCnt    = 10000,
                            .upway        = 0,
                            .upwav        = 0,
                            .savwav       = 1,
                        },
                        
                        {   //cali
                            .smpMode      = SMP_PERIOD_MODE,
                            .smpFreq      = 1000000,
                            .smpPoints    = 10000,
                        },                
                                          
                        {   //test        
                            .smpMode      = SMP_PERIOD_MODE,
                            .smpFreq      = 100000,
                            .smpPoints    = 10000,
                            .smpInterval  = 0,
                            .smpTimes     = 1,
                            .ampThreshold = 2.2f,
                            .messDuration = 5000,
                            .trigDelay    = 30000,
                            .ev           = {0,1,2,3},
                            .n_ev         = 4,
                            .evCalcCnt    = 10000,
                            .upway        = 0,
                            .upwav        = 0,
                            .savwav       = 1,
                        },
                    },
                    
                    .coef={
#ifdef USE_LAB_1
                        .a = 187.914246f,
                        .b = -18.265991f,
#elif defined USE_LAB_2
                        .a  = 190.326859f,
                        .b  = -11.934570f,
#else                        
                        .a  = 1.0f,
                        .b  = 0.0f,
#endif
                    }
                    
                },
                
                {      //CH_1
                    .ch         = 1,
                    .enable     = 0,
                    
                    {
                        {   //normal
                            .smpMode      = SMP_PERIOD_MODE,
                            .smpFreq      = 100000,
                            .smpPoints    = 1000,
                            .smpInterval  = 0,
                            .smpTimes     = 1,
                            .ampThreshold = 2.2f,
                            .messDuration = 5000,
                            .trigDelay    = 30000,
                            .ev           = {0,1,2,3},
                            .n_ev         = 4,
                            .evCalcCnt    = 1000,
                            .upway        = 0,
                            .upwav        = 0,
                            .savwav       = 1,
                        },
                    
                        {   //cali
                            .smpMode      = SMP_PERIOD_MODE,
                            .smpFreq      = 100000,
                            .smpPoints    = 1000,
                        },
                        
                        {   //test
                            .smpMode      = SMP_PERIOD_MODE,
                            .smpFreq      = 100000,
                            .smpPoints    = 1000,
                            .smpInterval  = 0,
                            .smpTimes     = 1,
                            .ampThreshold = 2.2f,
                            .messDuration = 5000,
                            .trigDelay    = 30000,
                            .ev           = {0,1,2,3},
                            .n_ev         = 4,
                            .evCalcCnt    = 1000,
                            .upway        = 0,
                            .upwav        = 0,
                            .savwav       = 1,
                        },
                    },
                    
                    .coef={
                        .a  = 1.0f,
                        .b  = 0.0f,
                    }
                }
            },
        },
        
        .dac = {
            .enable = 0,
            .fdiv   = 1,
        }
    },
    
    .var = {
        .cali={
            {.cnt=0},
            {.cnt=0},
        },
            
        .state={
            .stat={STAT_STOP, STAT_STOP},
            .finished={0, 0},
        }
    }
};


const date_time_t DFLT_TIME={
    .date = {
        .year  = 2000,
        .mon   = 1,
        .day   = 1,
        .week  = 6,
    },
    
    .time = {
        .hour = 0,
        .min  = 0,
        .sec  = 0,
    }
};

#if 0
const char *all_para_json="{\
    \"netPara\":\
        {\
             \"mode\":           1,                     \
             \"plat\":                                  \
             {\
                \"host\":           "STR(HOST)",        \
                \"port\":           "STR(PORT)",        \
                \"productKey\":     "STR(PRD_KEY)",     \
                \"productSecret\":  "STR(PRD_SECRET)",  \
                \"deviceKey\":      "STR(DEV_KEY)",     \
                \"deviceSecret\":   "STR(DEV_SECRET)"   \
             }\
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
             \"pwr_mode\":         0,          \
             \"pwr_period\":       \"3h\"      \
         },\
    \"chPara\":[\
         {\
            \"ch\":             0,             \
            \"smpMode\":        0,             \
            \"smpFreq\":        1000,          \
            \"smpTime\":        5,             \
            \"smpInterval\":    0,             \
            \"smpTimes\":       1,             \
            \"ampThreshold\":   2.2,           \
            \"messDuration\":   5000,          \
            \"trigDelay\":      300,           \
            \"ev\":             [0,1,2,3],     \
            \"evCalcCnt\":      10000,         \
            \"upway\":          0,             \
            \"upwav\":          0              \
        },\
        {\
            \"ch\":             1,             \
            \"smpMode\":        0,             \
            \"smpFreq\":        100,           \
            \"smpTime\":        5,             \
            \"smpInterval\":    100,           \
            \"smpTimes\":       1,             \
            \"ampThreshold\":   2.2,           \
            \"messDuration\":   5000,          \
            \"trigDelay\":      300,           \
            \"ev\":             [1],           \
            \"evCalcCnt\":      1000,          \
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
#endif


const char *filesPath[FILE_MAX]={
    SFLASH_MNT_PT"/cfg/para.bin",
    SDMMC_MNT_PT"/xxxx",
    
    SFLASH_MNT_PT"/upg/app.upg",
    SFLASH_MNT_PT"/log/run.log",
};



