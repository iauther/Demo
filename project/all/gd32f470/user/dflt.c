#include "protocol.h"

#ifndef BOOTLOADER
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
    #define COEF_A          197.745422f
    //#define COEF_A          206.110092f
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

const char *filesPath[FILE_MAX]={
    SFLASH_MNT_PT"/cfg/para.bin",
    SDMMC_MNT_PT"/xxxx",
    
    SFLASH_MNT_PT"/upg/app.upg",
    SFLASH_MNT_PT"/log/run.log",
};

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
            .worktime    = 60*10,
            
            .ch = {
                    {
                        .ch         = CH_0,
                        .enable     = 1,
                        
                        //.smpMode      = SMP_MODE_PERIOD,
                        .smpMode    = SMP_MODE_TRIG,
                        .smpFreq    = 1000000,
                        
                        .smpPoints    = 10000,
                        .smpInterval  = 1000000,
                        .smpTimes     = 1,
                        
                        .trigEvType     = 1,
                        .trigThreshold  = 60.0f,
                        .trigTime={
                            .preTime    = 200,
                            .postTime   = 200,
                            .PDT        = 300,
                            .HDT        = 600,
                            .HLT        = 500000,
                            .MDT        = 4000,
                        },
                        
                        .n_ev         = 4,
                        .ev           = {0,1,2,3},
                        .evCalcPoints = 10000,
                        
                        .upway      = 0,
                        .upwav      = 1,
                        .savwav     = 1,
                        
                        .coef={
                            .a = COEF_A,
                            .b = COEF_B,
                        }
                    },
                    
                    {
                        .ch         = CH_1,
                        .enable     = 0,
                        
                        .smpMode      = SMP_MODE_PERIOD,
                        .smpFreq    = 1000000,
                        
                        .smpPoints    = 10000,
                        .smpInterval  = 1000000,
                        .smpTimes     = 1,
                        
                        .trigEvType = 1,
                        .trigThreshold  = 50.0f,
                        .trigTime={
                            .preTime    = 600,
                            .postTime   = 600,
                            .PDT        = 300,
                            .HDT        = 600,
                            .HLT        = 500000,
                            .MDT        = 10000,
                        },
                        
                        .n_ev         = 4,
                        .ev           = {0,1,2,3},
                        .evCalcPoints = 5000,
                        
                        .upway      = 0,
                        .upwav      = 0,
                        .savwav     = 1,
                        
                        .coef={
                            .a = COEF_A,
                            .b = COEF_B,
                        }
                    },
            },
    #elif defined DEV_MODE_TEST
            .worktime   = 60*10,
            
            .ch = {
                    {
                        .ch         = CH_0,
                        .enable     = 1,
                        
                        //.smpMode      = SMP_MODE_PERIOD,
                        .smpMode    = SMP_MODE_TRIG,
                        .smpFreq    = 1000000,
                        
                        .smpPoints    = 10000,
                        .smpInterval  = 1000000,
                        .smpTimes     = 1,
                        
                        .trigEvType = 1,
                        .trigThreshold  = 50.0f,
                        .trigTime={
                            .preTime    = 600,
                            .postTime   = 600,
                            .PDT        = 300,
                            .HDT        = 600,
                            .HLT        = 5000,
                            .MDT        = 10000,
                        },
                        
                        .n_ev         = 4,
                        .ev           = {0,1,2,3},
                        .evCalcPoints = 5000,
                        
                        .upway      = 0,
                        .upwav      = 0,
                        .savwav     = 1,
                        
                        .coef={
                            .a = COEF_A,
                            .b = COEF_B,
                        }
                    },
                    
                    {
                        .ch         = CH_1,
                        .enable     = 0,
                        
                        .smpMode      = SMP_MODE_PERIOD,
                        .smpFreq    = 1000000,
                        
                        .smpPoints    = 10000,
                        .smpInterval  = 1000000,
                        .smpTimes     = 1,
                        
                        .trigEvType = 1,
                        .trigThreshold  = 50.0f,
                        .trigTime={
                            .preTime    = 600,
                            .postTime   = 600,
                            .PDT        = 300,
                            .HDT        = 600,
                            .HLT        = 5000,
                            .MDT        = 10000,
                        },
                        
                        .n_ev         = 4,
                        .ev           = {0,1,2,3},
                        .evCalcPoints = 5000,
                        
                        .upway      = 0,
                        .upwav      = 0,
                        .savwav     = 1,
                        
                        .coef={
                            .a = COEF_A,
                            .b = COEF_B,
                        }
                    },
            },
    #else
            .worktime   = 60*60*4,
            
            .ch = {
                    {
                        .ch         = CH_0,
                        .enable     = 1,
                        
                        .smpMode      = SMP_MODE_PERIOD,
                        .smpFreq    = 1000000,
                        
                        .smpPoints    = 10000,
                        .smpInterval  = 1000000,
                        .smpTimes     = 1,
                        
                        .trigEvType = 1,
                        .trigThreshold  = 50.0f,
                        .trigTime={
                            .preTime    = 600,
                            .postTime   = 600,
                            .PDT        = 300,
                            .HDT        = 600,
                            .HLT        = 5000,
                            .MDT        = 10000,
                        },
                        
                        .n_ev         = 4,
                        .ev           = {0,1,2,3},
                        .evCalcPoints = 5000,
                        
                        .upway      = 0,
                        .upwav      = 0,
                        .savwav     = 1,
                        
                        .coef={
                            .a = COEF_A,
                            .b = COEF_B,
                        }
                    },
                    
                    {
                        .ch         = CH_1,
                        .enable     = 0,
                        
                        .smpMode      = SMP_MODE_PERIOD,
                        .smpFreq    = 1000000,
                        
                        .smpPoints    = 10000,
                        .smpInterval  = 1000000,
                        .smpTimes     = 1,
                        
                        .trigEvType = 1,
                        .trigThreshold  = 50.0f,
                        .trigTime={
                            .preTime    = 600,
                            .postTime   = 600,
                            .PDT        = 300,
                            .HDT        = 600,
                            .HLT        = 5000,
                            .MDT        = 10000,
                        },
                        
                        .n_ev         = 4,
                        .ev           = {0,1,2,3},
                        .evCalcPoints = 5000,
                        
                        .upway      = 0,
                        .upwav      = 0,
                        .savwav     = 1,
                        
                        .coef={
                            .a = COEF_A,
                            .b = COEF_B,
                        }
                    },
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
#endif


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







