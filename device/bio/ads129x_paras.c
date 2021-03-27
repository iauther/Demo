#include "bio/ads129x.h"
#include "cfg.h"


#ifdef ADS129X_ENABLE

/*
    0: normal input
    1: input  short
    2: rld meassure
    3: VDD messure
    4: temprature test
    5: test signal
    6: rld drive positive
    7: rld drive negative 
*/
#define MUX     0//5//0


/*
        0: fCLK/POW(2,21)
        1: fCLK/POW(2,20)
        2: not use
        3: dc current
*/
#define TEST_FREQ    0

ads129x_regs_t adsRegs={
    
    .id = {
        .cid                      = 0,
        .did                      = 0,
    },

    .config1 = {
        .dr                       = 6,   //250sps
        .clk_en                   = 0,
        .daisy_en                 = 1,      //0: daisy mode        1: multi readback mode
        .hr                       = 0,      //0: low power mode    1: high resolution mode
    },

    .config2 = {
        .test_freq                = TEST_FREQ,
        .test_amp                 = 0,
        .reserved1                = 0,
        .int_test                 = 0,
        .chop                     = 0,
        .reserved2                = 0,
    },

    .config3 = {
        .rld_stat                 = 0,
        .rld_loff_sens            = 0,      //0: disable rld sensing    1: enable rld sensing
        .pd_rld                   = 1,      //0: power down rld buffer  1: power on rld buffer
        .rldref_int               = 1,      //0: extern rld   1: internal rld
        .rld_meas                 = 0,      //???
        .vref_4v                  = 0,
        .reserved                 = 1,
        .pd_refbuf                = 1,
    },

    .loff = {
        .flead_off                = 3,//1: ac lead off detection 2: not use, 3: dc lead off detection
        .ilead_off                = 0,
        .vlead_off_en             = 0,
        .comp_th                  = 0,
    },

    .channel = {
        {.mux=2,      .reserved=0,    .gain=0,    .pd=0},
        {.mux=MUX,    .reserved=0,    .gain=0,    .pd=0},
        {.mux=MUX,    .reserved=0,    .gain=0,    .pd=0},
        {.mux=MUX,    .reserved=0,    .gain=0,    .pd=0},
        {.mux=MUX,    .reserved=0,    .gain=0,    .pd=0},
        {.mux=MUX,    .reserved=0,    .gain=0,    .pd=0},
        {.mux=MUX,    .reserved=0,    .gain=0,    .pd=0},
        {.mux=MUX,    .reserved=0,    .gain=0,    .pd=0},
    },

    .rld_sens_p = {
        .in1_p                    = 0,
        .in2_p                    = 1,
        .in3_p                    = 1,
        .in4_p                    = 0,
        .in5_p                    = 0,
        .in6_p                    = 0,
        .in7_p                    = 0,
        .in8_p                    = 0,
    },

    .rld_sens_n = {
        .in1_n                    = 0,
        .in2_n                    = 1,
        .in3_n                    = 0,
        .in4_n                    = 0,
        .in5_n                    = 0,
        .in6_n                    = 0,
        .in7_n                    = 0,
        .in8_n                    = 0,
    },

    .loff_sens_p = {
        .in1_p                    = 0,
        .in2_p                    = 0,
        .in3_p                    = 0,
        .in4_p                    = 0,
        .in5_p                    = 0,
        .in6_p                    = 0,
        .in7_p                    = 0,
        .in8_p                    = 0,
    },

    .loff_sens_n = {
        .in1_n                    = 0,
        .in2_n                    = 0,
        .in3_n                    = 0,
        .in4_n                    = 0,
        .in5_n                    = 0,
        .in6_n                    = 0,
        .in7_n                    = 0,
        .in8_n                    = 0,
    },

    .loff_flip = {
        .flip1                    = 0,
        .flip2                    = 0,
        .flip3                    = 0,
        .flip4                    = 0,
        .flip5                    = 0,
        .flip6                    = 0,
        .flip7                    = 0,
        .flip8                    = 0,
    },

    .loff_stat_p = {
        .in1_p                    = 0,
        .in2_p                    = 0,
        .in3_p                    = 0,
        .in4_p                    = 0,
        .in5_p                    = 0,
        .in6_p                    = 0,
        .in7_p                    = 0,
        .in8_p                    = 0,
    },

    .loff_stat_n = {
        .in1_n                    = 0,
        .in2_n                    = 0,
        .in3_n                    = 0,
        .in4_n                    = 0,
        .in5_n                    = 0,
        .in6_n                    = 0,
        .in7_n                    = 0,
        .in8_n                    = 0,
    },

    .gpio = {
        .gpioc                    = 0x00,
        .gpiod                    = 0x00,
    },

    .pace = {
        .en                       = 0,
        .paceo                    = 0,
        .pacee                    = 0,
    },

    .resp = {
        .ctl                      = 0,
        .pha                      = 0,
        .reserved                 = 1,
        .mode                     = 0,
        .demode                   = 0,
    },

    .config4 = {
        .reserved1                = 0,
        .loff_comp_en             = 1,      //need enable
        .wct2rld                  = 0,
        .single_shot              = 0,
        .reserved2                = 0,
        .resp_freq                = 0,
    },

    .wct1 = {
        .wcta                     = 3,
        .wcta_en                  = 1,
        .avr_ch4                  = 0,
        .avr_ch7                  = 0,
        .avl_ch5                  = 0,
        .avf_ch6                  = 0,
    },

    .wct2 = {
        .wctc                     = 4,
        .wctb                     = 2,
        .wctb_en                  = 1,
        .wctc_en                  = 1,
    },
};

#endif


