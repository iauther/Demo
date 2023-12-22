#include "protocol.h"
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
    //#define COEF_A          185.495178f  
    #define COEF_A          206.110092f
    #define COEF_B          0.0f
    
    #elif defined USE_LAB_2
    #define DEV_KEY         "lab_2"
    #define DEV_SECRET      "4506649b902d8f747f73d3e6872fbb23"
    #define COEF_A          187.564941
    #define COEF_B          0.0f
    
    #elif defined USE_LAB_3
    #define DEV_KEY         "lab_3"
    #define DEV_SECRET      "d48f983e2bf44e617b4e68d59d81ec8a"
    #define COEF_A          185.810608f
    #define COEF_B          0.0f
    #elif defined USE_LAB_4
    #define DEV_KEY         "lab_4"
    #define DEV_SECRET      "2b1bc1aa666fc0763d4eaac87cb9214b"
    #define COEF_A          1.0f
    #define COEF_B          0.0f
    #endif
#endif
      

const all_para_t DFLT_PARA={
    
    .sys = {
        .fwInfo={
            .magic=FW_MAGIC,
            .version=FW_VERSION,
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
            .port       = PORT_NET,
            .pwrmode    = PWR_NO_PWRDN,
            
            .mode       = MODE_NORM,
    #ifdef DEV_MODE_DEBUG
            .workInterval    = 60*10,
            
            .ch = {
                    {   //CH_0
                        .ch         = 0,
                        .enable     = 1,
                        
                        .smpMode      = SMP_PERIOD_MODE,
                        .smpFreq      = 100000,
                        .smpPoints    = 1000,
                        .smpInterval  = 1000000,
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
                        
                        .coef={
                            .a = COEF_A,
                            .b = COEF_B,
                        }
                    },
                    
                    {
                        .ch         = 1,
                        .enable     = 0,
                
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
                        
                        .coef={
                            .a = 1.0f,
                            .b = 0.0f,
                        }
                    }
            },
    #elif defined DEV_MODE_TEST
            .workInterval   = 60*10,
            
            .ch = {
                    {
                        .ch         = 0,
                        .enable     = 1,
                
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
                        
                        .coef={
                            .a = COEF_A,
                            .b = COEF_B,
                        }
                    },
                    
                    {
                        .ch         = 1,
                        .enable     = 0,
                
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
                        
                        .coef={
                            .a = 1.0f,
                            .b = 0.0f,
                        }
                    }
            },
    #else
            .workInterval   = 60*60*4,
            
            .ch = {
                    {   //CH_0
                        .ch         = 0,
                        .enable     = 1,
                        
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
                        .upwav        = 1,
                        .savwav       = 1,
                        
                        .coef={
                            .a = COEF_A,
                            .b = COEF_B,
                        }
                    },
                    
                    {   //CH_1
                        .ch         = 1,
                        .enable     = 0,

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

                        .coef={
                            .a = 1.0f,
                            .b = 0.0f,
                        }  
                    }
            },
            #endif
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


const datetime_t DFLT_TIME={
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



const char *filesPath[FILE_MAX]={
    SFLASH_MNT_PT"/cfg/para.bin",
    SDMMC_MNT_PT"/xxxx",
    
    SFLASH_MNT_PT"/upg/app.upg",
    SFLASH_MNT_PT"/log/run.log",
};



