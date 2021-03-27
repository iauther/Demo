#ifndef __ADS1298X_H__
#define __ADS1298X_H__

#include "types.h"

#define CHID_ADS1298   0x92
#define CHID_ADS1298R  0xD2

enum spi_command {
    // system commands
    CMD_WAKEUP = 0x02,
    CMD_STANDBY = 0x04,
    CMD_RESET = 0x06,
    CMD_START = 0x08,
    CMD_STOP = 0x0a,

    // read commands
    CMD_RDATAC = 0x10,
    CMD_SDATAC = 0x11,
    CMD_RDATA = 0x12,

    // register commands
    CMD_RREG = 0x20,
    CMD_WREG = 0x40
};

enum REG {
    // device settings
    REG_ID = 0x00,

    // global settings
    REG_CONFIG1 = 0x01,
    REG_CONFIG2 = 0x02,
    REG_CONFIG3 = 0x03,
    REG_LOFF = 0x04,

    // channel specific settings
    REG_CHnSET = 0x04,
    REG_CH1SET = REG_CHnSET + 1,
    REG_CH2SET = REG_CHnSET + 2,
    REG_CH3SET = REG_CHnSET + 3,
    REG_CH4SET = REG_CHnSET + 4,
    REG_CH5SET = REG_CHnSET + 5,
    REG_CH6SET = REG_CHnSET + 6,
    REG_CH7SET = REG_CHnSET + 7,
    REG_CH8SET = REG_CHnSET + 8,
    REG_RLD_SENSP = 0x0d,
    REG_RLD_SENSN = 0x0e,
    REG_LOFF_SENSP = 0x0f,
    REG_LOFF_SENSN = 0x10,
    REG_LOFF_FLIP = 0x11,

    // lead off status
    REG_LOFF_STATP = 0x12,
    REG_LOFF_STATN = 0x13,

    // other
    REG_GPIO = 0x14,
    REG_PACE = 0x15,
    REG_RESP = 0x16,
    REG_CONFIG4 = 0x17,
    REG_WCT1 = 0x18,
    REG_WCT2 = 0x19
};

#pragma pack (1)   
typedef struct {
    U8 cid                  :3;
    U8                      :2;
    U8 did                  :3;
}id_t;

typedef struct {
    U8 dr                   :3;     //data out rate
    U8                      :2;
    U8 clk_en               :1;
    U8 daisy_en             :1;     //0: daisy,      1: multi readback
    U8 hr                   :1;     //0: low power,  1: high resolution
}config1_t;

typedef struct {
    U8 test_freq            :2;     //00: fCLK/2^21   01: fCLK/2^20   10: no use   11: dc
    U8 test_amp             :1;     
    U8 reserved1            :1;
    U8 int_test             :1;     //0: internal test,         1: external test
    U8 chop                 :1;     //0: chop freq is vaiable,  1: chop freq is fixed: fMODE/16
    U8 reserved2            :2;
}config2_t;

typedef struct {
    U8 rld_stat             :1;
    U8 rld_loff_sens        :1;     //0: disable rld sens         1: enable rld sens
    U8 pd_rld               :1;
    U8 rldref_int           :1;     //0: 
    U8 rld_meas             :1;     //0: open,           1: rld_in
    U8 vref_4v              :1;     //0: VREFP is 2.4V,  1: VREFP is 4V
    U8 reserved             :1;     //1 only
    U8 pd_refbuf            :1;     //0: disable internal REFBUF, 1: enable internal REFBUF
}config3_t;

typedef struct {
    U8 flead_off            :2;     //0:
    U8 ilead_off            :2;     //0: 
    U8 vlead_off_en         :1;     //0: current mode,  1: pull up/down mode
    U8 comp_th              :3;     //0: 
}loff_t;

typedef struct {
    U8 mux                  :3;     //000: normal, 001: input short, 002: 
    U8 reserved             :1;
    U8 gain                 :3;     //0:1,2,3,4,6,8,12dB
    U8 pd                   :1;     //0: run,  1: power down
}channel_t;

typedef struct {
    U8 in1_p                :1;     //0: disable,  1: enable
    U8 in2_p                :1;     //
    U8 in3_p                :1;     //
    U8 in4_p                :1;     //
    U8 in5_p                :1;     //
    U8 in6_p                :1;     //
    U8 in7_p                :1;     //
    U8 in8_p                :1;     //
}rld_sens_p_t;

typedef struct {
    U8 in1_n                :1;     //0: disable,  1: enable
    U8 in2_n                :1;     //
    U8 in3_n                :1;     //
    U8 in4_n                :1;     //
    U8 in5_n                :1;     //
    U8 in6_n                :1;     //
    U8 in7_n                :1;     //
    U8 in8_n                :1;     //
}rld_sens_n_t;

typedef struct {
    U8 in1_p                :1;     //0: disable,  1: enable
    U8 in2_p                :1;     //
    U8 in3_p                :1;     //
    U8 in4_p                :1;     //
    U8 in5_p                :1;     //
    U8 in6_p                :1;     //
    U8 in7_p                :1;     //
    U8 in8_p                :1;     //
}loff_sens_p_t;

typedef struct {
    U8 in1_n                :1;     //0: disable,  1: enable
    U8 in2_n                :1;     //
    U8 in3_n                :1;     //
    U8 in4_n                :1;     //
    U8 in5_n                :1;     //
    U8 in6_n                :1;     //
    U8 in7_n                :1;     //
    U8 in8_n                :1;     //
}loff_sens_n_t;

typedef struct {
    U8 flip1                :1;     //0: disable,  1: enable
    U8 flip2                :1;     //
    U8 flip3                :1;     //
    U8 flip4                :1;     //
    U8 flip5                :1;     //
    U8 flip6                :1;     //
    U8 flip7                :1;     //
    U8 flip8                :1;     //
}loff_flip_t;

typedef struct {
    U8 in1_p                :1;     //
    U8 in2_p                :1;     //
    U8 in3_p                :1;     //
    U8 in4_p                :1;     //
    U8 in5_p                :1;     //
    U8 in6_p                :1;     //
    U8 in7_p                :1;     //
    U8 in8_p                :1;     //
}loff_stat_p_t;

typedef struct {
    U8 in1_n                :1;     //
    U8 in2_n                :1;     //
    U8 in3_n                :1;     //
    U8 in4_n                :1;     //
    U8 in5_n                :1;     //
    U8 in6_n                :1;     //
    U8 in7_n                :1;     //
    U8 in8_n                :1;     //
}loff_stat_n_t;

typedef struct {
    U8 gpioc                :4;     //0: output,  1: input
    U8 gpiod                :4;     //
}gpio_t;

typedef struct {
    U8 en                   :1;     //0: disable,  1: enable
    U8 paceo                :2;     //
    U8 pacee                :2;     //
    U8                      :3;     //
}pace_t;

typedef struct {
    U8 ctl                  :2;
    U8 pha                  :3;     //0: disable,  1: enable
    U8 reserved             :1;     //
    U8 mode                 :1;     //
    U8 demode               :1;     //
}resp_t;

typedef struct {
    U8 reserved1            :1;     //
    U8 loff_comp_en         :1;     //0: disable,  1: enable
    U8 wct2rld              :1;     //0: disable,  1: enable
    U8 single_shot          :1;     //
    U8 reserved2            :1;
    U8 resp_freq            :3;     //
}config4_t;

typedef struct {
    U8 wcta                 :3;     //
    U8 wcta_en              :1;     //0: disable,  1: enable
    U8 avr_ch4              :1;     //
    U8 avr_ch7              :1;     //
    U8 avl_ch5              :1;     //
    U8 avf_ch6              :1;     //
}wct1_t;

typedef struct {
    U8 wctc                 :3;     //
    U8 wctb                 :3;     //
    U8 wctb_en              :1;     //0: disable,  1: enable
    U8 wctc_en              :1;     //0: disable,  1: enable
}wct2_t;

typedef struct {
    id_t                    id;
    config1_t               config1;
    config2_t               config2;
    config3_t               config3;
    loff_t                  loff;
    channel_t               channel[8];
    rld_sens_p_t            rld_sens_p;
    rld_sens_n_t            rld_sens_n;
    loff_sens_p_t           loff_sens_p;
    loff_sens_n_t           loff_sens_n;
    loff_flip_t             loff_flip;
    loff_stat_p_t           loff_stat_p;
    loff_stat_n_t           loff_stat_n;
    gpio_t                  gpio;
    pace_t                  pace;
    resp_t                  resp;
    config4_t               config4;
    wct1_t                  wct1;
    wct2_t                  wct2;
}ads129x_regs_t;


typedef struct {
    U8                      d[2]; 
}adc16_t;
typedef struct {
    U8                      d[3]; 
}adc24_t;
typedef struct {
    adc16_t                 adc[8];
}data_16bits_t;             //16 bytes

typedef struct {
    adc24_t                 adc[8];
}data_24bits_t;             //24 bytes

typedef struct {
    U8                      h[3];
    union {
        data_16bits_t       d16;
        data_24bits_t       d24;
    }payload;
}adc_data_t;

#pragma pack ()



//#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
int ads129x_probe(void);

#endif
