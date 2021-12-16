#include "drv/gpio.h"
#include "log.h"
#include "drv/delay.h"
#include "bio/bio.h"
#include "myCfg.h"

typedef struct {
    hal_eint_number_t   num;
    U8                  mode;
}gpio_eint_t;

#ifdef USE_TCA6424A
volatile int bioDevice=0;
static gpio_eint_t gpio_eint[HAL_GPIO_MAX] = {
    {HAL_EINT_NUMBER_0 ,  HAL_GPIO_0_EINT0  },                   //HAL_GPIO_0,   
    {HAL_EINT_NUMBER_1 ,  HAL_GPIO_1_EINT1  },                   //HAL_GPIO_1,   
    {HAL_EINT_NUMBER_2 ,  HAL_GPIO_2_EINT2  },                   //HAL_GPIO_2,   
    {HAL_EINT_NUMBER_14,  HAL_GPIO_3_EINT14 },                   //HAL_GPIO_3,   
    {HAL_EINT_NUMBER_3 ,  HAL_GPIO_4_EINT3  },                   //HAL_GPIO_4,   
    {HAL_EINT_NUMBER_4 ,  HAL_GPIO_5_EINT4  },                   //HAL_GPIO_5,   
    {HAL_EINT_NUMBER_5 ,  HAL_GPIO_6_EINT5  },                   //HAL_GPIO_6,   
    {HAL_EINT_NUMBER_6 ,  HAL_GPIO_7_EINT6  },                   //HAL_GPIO_7,   
    {HAL_EINT_NUMBER_7 ,  HAL_GPIO_8_EINT7  },                   //HAL_GPIO_8,   
    {HAL_EINT_NUMBER_8 ,  HAL_GPIO_9_EINT8  },                   //HAL_GPIO_9,  
    {HAL_EINT_NUMBER_15,  HAL_GPIO_10_EINT15},                   //HAL_GPIO_10,  
    {HAL_EINT_NUMBER_9 ,  HAL_GPIO_11_EINT9 },                   //HAL_GPIO_11,  
    {HAL_EINT_NUMBER_10,  HAL_GPIO_12_EINT10},                   //HAL_GPIO_12,  
    {HAL_EINT_NUMBER_11,  HAL_GPIO_13_EINT11},                   //HAL_GPIO_13,  
    {HAL_EINT_NUMBER_12,  HAL_GPIO_14_EINT12},                   //HAL_GPIO_14,   
    {HAL_EINT_NUMBER_13,  HAL_GPIO_15_EINT13},                   //HAL_GPIO_15,  
    {HAL_EINT_NUMBER_16,  HAL_GPIO_16_EINT16},                   //HAL_GPIO_16,  
    {HAL_EINT_NUMBER_17,  HAL_GPIO_17_EINT17},                   //HAL_GPIO_17,  
                                                                 
    {HAL_EINT_NUMBER_MAX, 0},                                    //HAL_GPIO_18,  
                                                                 
    {HAL_EINT_NUMBER_18,  HAL_GPIO_19_EINT18},                   //HAL_GPIO_19,  
    {HAL_EINT_NUMBER_MAX, 0},                                    //HAL_GPIO_20,  
    {HAL_EINT_NUMBER_MAX, 0},                                    //HAL_GPIO_21,  
    {HAL_EINT_NUMBER_MAX, 0},                                    //HAL_GPIO_22,  
                                                                 
    {HAL_EINT_NUMBER_19,  HAL_GPIO_23_EINT19},                   //HAL_GPIO_23,  
    {HAL_EINT_NUMBER_9 ,  HAL_GPIO_24_EINT9 },                   //HAL_GPIO_24,  
    {HAL_EINT_NUMBER_MAX, 0},                                    //HAL_GPIO_25,  
    {HAL_EINT_NUMBER_MAX, 0},                                    //HAL_GPIO_26,  
    {HAL_EINT_NUMBER_MAX, 0},                                    //HAL_GPIO_27,  
    {HAL_EINT_NUMBER_MAX, 0},                                    //HAL_GPIO_28,  
                                                                 
    {HAL_EINT_NUMBER_10,  HAL_GPIO_29_EINT10},                   //HAL_GPIO_29,  
    {HAL_EINT_NUMBER_11,  HAL_GPIO_30_EINT11},                   //HAL_GPIO_30,  
    {HAL_EINT_NUMBER_12,  HAL_GPIO_31_EINT12},                   //HAL_GPIO_31,  
    {HAL_EINT_NUMBER_13,  HAL_GPIO_32_EINT13},                   //HAL_GPIO_32,  
    {HAL_EINT_NUMBER_14,  HAL_GPIO_33_EINT14},                   //HAL_GPIO_33,  
    {HAL_EINT_NUMBER_15,  HAL_GPIO_34_EINT15},                   //HAL_GPIO_34,  
    {HAL_EINT_NUMBER_3 ,  HAL_GPIO_35_EINT3 },                   //HAL_GPIO_35,  
    {HAL_EINT_NUMBER_4 ,  HAL_GPIO_39_EINT4 },                   //HAL_GPIO_39,  
    {HAL_EINT_NUMBER_MAX, 0},                                    //HAL_GPIO_40,  
                                                                 
    {HAL_EINT_NUMBER_5 ,  HAL_GPIO_41_EINT5 },                   //HAL_GPIO_41,  
    {HAL_EINT_NUMBER_MAX, 0},                                    //HAL_GPIO_42,  
                                                                 
    {HAL_EINT_NUMBER_6 ,  HAL_GPIO_43_EINT6 },                   //HAL_GPIO_43,  
    {HAL_EINT_NUMBER_MAX, 0},                                    //HAL_GPIO_44,  
    {HAL_EINT_NUMBER_MAX, 0},                                    //HAL_GPIO_45,  
    {HAL_EINT_NUMBER_MAX, 0},                                    //HAL_GPIO_46,  
    {HAL_EINT_NUMBER_MAX, 0},                                    //HAL_GPIO_47,  
    {HAL_EINT_NUMBER_MAX, 0},                                    //HAL_GPIO_48,  
                                                                 
    {HAL_EINT_NUMBER_MAX, 0}                                     //HAL_GPIO_MAX, 
};


//TMUX1104, EN总开关(高开低关), A1A0(00(S1开), 01(S2开), 02(S3开), 03(S4开))
//TMUX1112, 高开低关
//TMUX1136, 高SxA通(DC), 低SxB通(AC)
gpio_ext_cfg_t ad8233_gpio_cfg[AD8233_GPIO_EXT_MAX] = {
#ifdef BOARD_V00_01
    //  port                                     tca                             dir        enable      disable      dflt
    //TCA6424A_ADDR_H, P0 --------------start--------------
    {AD8233_GPIO_EXT_PIN_RLD_SDN_CH3,           {TCA6424A_ADDR_H,  0,  0},        0,          1,          0,          1},  //AD8233右腿驱动使能, 1: ENABLE   0:DISABLE
    {AD8233_GPIO_EXT_PIN_RLD_SDN_CH2,           {TCA6424A_ADDR_H,  0,  1},        0,          1,          0,          1},  //AD8233右腿驱动使能, 1: ENABLE   0:DISABLE
    {AD8233_GPIO_EXT_PIN_RLD_SDN_CH1,           {TCA6424A_ADDR_H,  0,  2},        0,          1,          0,          1},  //AD8233右腿驱动使能, 1: ENABLE   0:DISABLE
    {AD8233_GPIO_EXT_PIN_NSDN_CH8,              {TCA6424A_ADDR_H,  0,  3},        0,          1,          0,          1},  //AD8233芯片使能,     1: ENABLE   0:DISABLE
    {AD8233_GPIO_EXT_PIN_NSDN_CH7,              {TCA6424A_ADDR_H,  0,  4},        0,          1,          0,          1},  //AD8233芯片使能,     1: ENABLE   0:DISABLE
    {AD8233_GPIO_EXT_PIN_NSDN_CH6,              {TCA6424A_ADDR_H,  0,  5},        0,          1,          0,          1},  //AD8233芯片使能,     1: ENABLE   0:DISABLE
    {AD8233_GPIO_EXT_PIN_NSDN_CH5,              {TCA6424A_ADDR_H,  0,  6},        0,          1,          0,          1},  //AD8233芯片使能,     1: ENABLE   0:DISABLE
    {AD8233_GPIO_EXT_PIN_NSDN_CH4,              {TCA6424A_ADDR_H,  0,  7},        0,          1,          0,          1},  //AD8233芯片使能,     1: ENABLE   0:DISABLE
    //TCA6424A_ADDR_H, P0 --------------end----------------                                  
                                                            
    //TCA6424A_ADDR_H, P1 --------------start--------------                                 
    {AD8233_GPIO_EXT_PIN_NSDN_CH3,              {TCA6424A_ADDR_H,  1,  0},        0,          1,          0,          1},  //AD8233芯片使能,     1: ENABLE   0:DISABLE
    {AD8233_GPIO_EXT_PIN_NSDN_CH2,              {TCA6424A_ADDR_H,  1,  1},        0,          1,          0,          1},  //AD8233芯片使能,     1: ENABLE   0:DISABLE
    {AD8233_GPIO_EXT_PIN_NSDN_CH1,              {TCA6424A_ADDR_H,  1,  2},        0,          1,          0,          1},  //AD8233芯片使能,     1: ENABLE   0:DISABLE
    {AD8233_GPIO_EXT_PIN_ACDC_SEL_CH1,          {TCA6424A_ADDR_H,  1,  3},        0,          1,          0,          0},  //AD8233导联脱落检测交流直流功能选择, 1: AC  0: DC
    {AD8233_GPIO_EXT_PIN_ACDC_SEL_CH2,          {TCA6424A_ADDR_H,  1,  4},        0,          1,          0,          0},  //AD8233导联脱落检测交流直流功能选择, 1: AC  0: DC
    {AD8233_GPIO_EXT_PIN_ACDC_SEL_CH3,          {TCA6424A_ADDR_H,  1,  5},        0,          1,          0,          0},  //AD8233导联脱落检测交流直流功能选择, 1: AC  0: DC
    {AD8233_GPIO_EXT_PIN_ACDC_SEL_CH4,          {TCA6424A_ADDR_H,  1,  6},        0,          1,          0,          0},  //AD8233导联脱落检测交流直流功能选择, 1: AC  0: DC
    {AD8233_GPIO_EXT_PIN_ACDC_SEL_CH5,          {TCA6424A_ADDR_H,  1,  7},        0,          1,          0,          0},  //AD8233导联脱落检测交流直流功能选择, 1: AC  0: DC
    //TCA6424A_ADDR_H, P1 --------------end----------------              
                                                                
    //TCA6424A_ADDR_H, P2 --------------start--------------               
    {AD8233_GPIO_EXT_PIN_ACDC_SEL_CH6,          {TCA6424A_ADDR_H,  2,  0},        0,          1,          0,          0},  //AD8233导联脱落检测交流直流功能选择, 1: AC  0: DC
    {AD8233_GPIO_EXT_PIN_ACDC_SEL_CH7,          {TCA6424A_ADDR_H,  2,  1},        0,          1,          0,          0},  //AD8233导联脱落检测交流直流功能选择, 1: AC  0: DC
    {AD8233_GPIO_EXT_PIN_ACDC_SEL_CH8,          {TCA6424A_ADDR_H,  2,  2},        0,          1,          0,          0},  //AD8233导联脱落检测交流直流功能选择, 1: AC  0: DC                
    {AD8233_GPIO_EXT_PIN_LEAD_DC_SEL,           {TCA6424A_ADDR_H,  2,  3},        0,          1,          0,          1},  //导联直流或交流通道选择              0: AC  1: DC    TMUX1136, 高SxA通(DC), 低SxB通(AC)
    {AD8233_GPIO_EXT_PIN_LEAD_AC_SEL,           {TCA6424A_ADDR_H,  2,  4},        0,          1,          0,          1},  //导联直流或交流通道选择              0: AC  1: DC    TMUX1136, 高SxA通(DC), 低SxB通(AC)
    {AD8233_GPIO_EXT_PIN_FR_CH_SEL,             {TCA6424A_ADDR_H,  2,  5},        0,          1,          0,          1},  //AD8233快速恢复   1：恢复      0：正常
    {AD8233_GPIO_EXT_PIN_LOFPU_SEL,             {TCA6424A_ADDR_H,  2,  6},        0,          1,          0,          1},  //导联直流脱落检测时上拉电阻使能 1:使能   0：不使能   TMUX1112(高开低关)
    {AD8233_GPIO_EXT_PIN_P27,                   {TCA6424A_ADDR_H,  2,  7},        0,          1,          0,          0},  //保留未用
    //TCA6424A_ADDR_H, P2 --------------end----------------               
#else
    //  port                                     tca                              dir       enable      disable      dflt
    //TCA6424A_ADDR_H, P0 --------------start--------------
    {AD8233_GPIO_EXT_PIN_RLD_SDN_CH3,           {TCA6424A_ADDR_H,  0,  0},        0,          1,          0,          1},  //AD8233右腿驱动使能, 1: ENABLE   0:DISABLE
    {AD8233_GPIO_EXT_PIN_RLD_SDN_CH2,           {TCA6424A_ADDR_H,  0,  1},        0,          1,          0,          1},  //AD8233右腿驱动使能, 1: ENABLE   0:DISABLE
    {AD8233_GPIO_EXT_PIN_RLD_SDN_CH1,           {TCA6424A_ADDR_H,  0,  2},        0,          1,          0,          1},  //AD8233右腿驱动使能, 1: ENABLE   0:DISABLE
    {AD8233_GPIO_EXT_PIN_NSDN_CH7,              {TCA6424A_ADDR_H,  0,  3},        0,          1,          0,          1},  //AD8233芯片使能,     1: ENABLE   0:DISABLE
    {AD8233_GPIO_EXT_PIN_NSDN_CH6,              {TCA6424A_ADDR_H,  0,  4},        0,          1,          0,          1},  //AD8233芯片使能,     1: ENABLE   0:DISABLE
    {AD8233_GPIO_EXT_PIN_NSDN_CH5,              {TCA6424A_ADDR_H,  0,  5},        0,          1,          0,          1},  //AD8233芯片使能,     1: ENABLE   0:DISABLE
    {AD8233_GPIO_EXT_PIN_NSDN_CH4,              {TCA6424A_ADDR_H,  0,  6},        0,          1,          0,          1},  //AD8233芯片使能,     1: ENABLE   0:DISABLE
    {AD8233_GPIO_EXT_PIN_NSDN_CH3,              {TCA6424A_ADDR_H,  0,  7},        0,          1,          0,          1},  //AD8233芯片使能,     1: ENABLE   0:DISABLE
    //TCA6424A_ADDR_H, P0 --------------end----------------                                  
                                                            
    //TCA6424A_ADDR_H, P1 --------------start--------------                                 
    {AD8233_GPIO_EXT_PIN_NSDN_CH2,              {TCA6424A_ADDR_H,  1,  0},        0,          1,          0,          1},  //AD8233芯片使能,     1: ENABLE   0:DISABLE
    {AD8233_GPIO_EXT_PIN_NSDN_CH1,              {TCA6424A_ADDR_H,  1,  1},        0,          1,          0,          1},  //AD8233芯片使能,     1: ENABLE   0:DISABLE
    {AD8233_GPIO_EXT_PIN_ACDC_SEL_CH1,          {TCA6424A_ADDR_H,  1,  2},        0,          1,          0,          0},  //AD8233导联脱落检测交流直流功能选择, 1: AC  0: DC
    {AD8233_GPIO_EXT_PIN_ACDC_SEL_CH2,          {TCA6424A_ADDR_H,  1,  3},        0,          1,          0,          0},  //AD8233导联脱落检测交流直流功能选择, 1: AC  0: DC
    {AD8233_GPIO_EXT_PIN_ACDC_SEL_CH3,          {TCA6424A_ADDR_H,  1,  4},        0,          1,          0,          0},  //AD8233导联脱落检测交流直流功能选择, 1: AC  0: DC
    {AD8233_GPIO_EXT_PIN_ACDC_SEL_CH4,          {TCA6424A_ADDR_H,  1,  5},        0,          1,          0,          0},  //AD8233导联脱落检测交流直流功能选择, 1: AC  0: DC
    {AD8233_GPIO_EXT_PIN_ACDC_SEL_CH5,          {TCA6424A_ADDR_H,  1,  6},        0,          1,          0,          0},  //AD8233导联脱落检测交流直流功能选择, 1: AC  0: DC
    {AD8233_GPIO_EXT_PIN_ACDC_SEL_CH6,          {TCA6424A_ADDR_H,  1,  7},        0,          1,          0,          0},  //AD8233导联脱落检测交流直流功能选择, 1: AC  0: DC
    //TCA6424A_ADDR_H, P1 --------------end----------------              
                                                                
    //TCA6424A_ADDR_H, P2 --------------start--------------               
    {AD8233_GPIO_EXT_PIN_ACDC_SEL_CH7,          {TCA6424A_ADDR_H,  2,  0},        0,          1,          0,          0},  //AD8233导联脱落检测交流直流功能选择, 1: AC  0: DC               
    {AD8233_GPIO_EXT_PIN_LEAD_DC_SEL,           {TCA6424A_ADDR_H,  2,  1},        0,          1,          0,          1},  //导联直流或交流通道选择              0: AC  1: DC    TMUX1136, 高SxA通(DC), 低SxB通(AC)
    {AD8233_GPIO_EXT_PIN_LEAD_AC_SEL,           {TCA6424A_ADDR_H,  2,  2},        0,          1,          0,          1},  //导联直流或交流通道选择              0: AC  1: DC    TMUX1136, 高SxA通(DC), 低SxB通(AC)
    {AD8233_GPIO_EXT_PIN_FR_CH_SEL,             {TCA6424A_ADDR_H,  2,  3},        0,          1,          0,          1},  //AD8233快速恢复   1：恢复      0：正常
    {AD8233_GPIO_EXT_PIN_LOFPU_SEL,             {TCA6424A_ADDR_H,  2,  4},        0,          1,          0,          1},  //导联直流脱落检测时上拉电阻使能 1:使能   0：不使能   TMUX1112(高开低关)
    {AD8233_GPIO_EXT_PIN_LED_R,                 {TCA6424A_ADDR_H,  2,  5},        0,          1,          0,          0},  //RED   LED    1: ON    0: OFF
    {AD8233_GPIO_EXT_PIN_LED_G,                 {TCA6424A_ADDR_H,  2,  6},        0,          1,          0,          0},  //GREEN LED    1: ON    0: OFF
    {AD8233_GPIO_EXT_PIN_LED_B,                 {TCA6424A_ADDR_H,  2,  7},        0,          1,          0,          0},  //BLUE  LED    1: ON    0: OFF
    //TCA6424A_ADDR_H, P2 --------------end----------------                                 
#endif
    
    
// !!!!!!!! THE FOLLOWING IS TCA6424A_ADDR_L !!!!!!!!!!
//****************************************************************************************************************************************************************************************************
    //TCA6424A_ADDR_L, P0 --------------start--------------                                   
    {AD8233_GPIO_EXT_PIN_RA_OUTPUT_SEL,         {TCA6424A_ADDR_L,  0,  0},        0,          1,          0,          1},  //使能RA参与WCT            TMUX1112(高开低关)
    {AD8233_GPIO_EXT_PIN_LA_OUTPUT_SEL,         {TCA6424A_ADDR_L,  0,  1},        0,          1,          0,          1},  //使能LA参与WCT            TMUX1112(高开低关)
    {AD8233_GPIO_EXT_PIN_LL_OUTPUT_SEL,         {TCA6424A_ADDR_L,  0,  2},        0,          1,          0,          1},  //使能LL参与WCT            TMUX1112(高开低关)
    {AD8233_GPIO_EXT_PIN_RLD_FB_SEL_CH1,        {TCA6424A_ADDR_L,  0,  3},        0,          1,          0,          0},  //右腿驱动信号来源选择     TMUX1112(高开低关)
    {AD8233_GPIO_EXT_PIN_RLD_FB_SEL_CH2,        {TCA6424A_ADDR_L,  0,  4},        0,          1,          0,          0},  //右腿驱动信号来源选择     TMUX1112(高开低关)
    {AD8233_GPIO_EXT_PIN_RLD_FB_SEL_CH3,        {TCA6424A_ADDR_L,  0,  5},        0,          1,          0,          0},  //右腿驱动信号来源选择     TMUX1112(高开低关)
    {AD8233_GPIO_EXT_PIN_RLD_WCT_SEL_CH4,       {TCA6424A_ADDR_L,  0,  6},        0,          1,          0,          1},  //右腿驱动信号来源选择     TMUX1112(高开低关)
    {AD8233_GPIO_EXT_PIN_RLD_SCAN_RES_UP_SEL,   {TCA6424A_ADDR_L,  0,  7},        0,          1,          0,          0},  //右腿电极自检上拉电阻选择 TMUX1112(高开低关)
    //TCA6424A_ADDR_L, P0                                                 
                                                            
    //TCA6424A_ADDR_L, P1 --------------start--------------                                 
    {AD8233_GPIO_EXT_PIN_RLD_SCAN_RES_DOWN_SEL, {TCA6424A_ADDR_L,  1,  0},        0,          1,          0,          0},  //右腿电极自检下拉电阻选择 TMUX1112(高开低关)
    {AD8233_GPIO_EXT_PIN_RLD_SELF_DISCON_SEL,   {TCA6424A_ADDR_L,  1,  1},        0,          1,          0,          1},  //右腿电极断开右腿驱动信号源，自检准备阶段   TMUX1112(高开低关)
    {AD8233_GPIO_EXT_PIN_RLD_TERMINAL_CON_SEL,  {TCA6424A_ADDR_L,  1,  2},        0,          1,          0,          1},  //右腿电极及自检电极连接一起，正常工作时     TMUX1112(高开低关)
    {AD8233_GPIO_EXT_PIN_RLD_CH_SEL_A0,         {TCA6424A_ADDR_L,  1,  3},        0,          1,          0,          0},  //右腿驱动信号来源选择     TMUX1104, EN总开关(高开低关), A1A0(00(S1开), 01(S2开), 02(S3开), 03(S4开))
    {AD8233_GPIO_EXT_PIN_RLD_CH_SEL_A1,         {TCA6424A_ADDR_L,  1,  4},        0,          1,          0,          0},  //右腿驱动信号来源选择     TMUX1104, EN总开关(高开低关), A1A0(00(S1开), 01(S2开), 02(S3开), 03(S4开))
    {AD8233_GPIO_EXT_PIN_RLD_AC_SEL,            {TCA6424A_ADDR_L,  1,  5},        0,          1,          0,          0},  //右腿电极自检AD检测是否跳过BUFFER                   未使用？？
    {AD8233_GPIO_EXT_PIN_WCT_OUT_SEL,           {TCA6424A_ADDR_L,  1,  6},        0,          1,          0,          0},  //胸导WCT信号来源选择，故障时可以使用右腿自检电极    未使用？？
    //TCA6424A_ADDR_L, P1 --------------end---------------- 
                                                           
    //TCA6424A_ADDR_L, P2 --------------start--------------             
    {AD8233_GPIO_EXT_PIN_LOD_CH1,               {TCA6424A_ADDR_L,  2,  0},        1,          1,          0,          0},  //电极脱落检测
    {AD8233_GPIO_EXT_PIN_LOD_CH2,               {TCA6424A_ADDR_L,  2,  1},        1,          1,          0,          0},  //
    {AD8233_GPIO_EXT_PIN_LOD_CH3,               {TCA6424A_ADDR_L,  2,  2},        1,          1,          0,          0},  //
    {AD8233_GPIO_EXT_PIN_LOD_CH4,               {TCA6424A_ADDR_L,  2,  3},        1,          1,          0,          0},  //
    {AD8233_GPIO_EXT_PIN_LOD_CH5,               {TCA6424A_ADDR_L,  2,  4},        1,          1,          0,          0},  //
    {AD8233_GPIO_EXT_PIN_LOD_CH6,               {TCA6424A_ADDR_L,  2,  5},        1,          1,          0,          0},  //
    {AD8233_GPIO_EXT_PIN_LOD_CH7,               {TCA6424A_ADDR_L,  2,  6},        1,          1,          0,          0},  //
    {AD8233_GPIO_EXT_PIN_LOD_CH8,               {TCA6424A_ADDR_L,  2,  7},        1,          1,          0,          0},  //not use
    //TCA6424A_ADDR_L, P2 --------------end---------------- 
};

gpio_ext_cfg_t ads129x_gpio_cfg[ADS129X_GPIO_EXT_MAX] = {
    //  port                                                 tca                     dir       enable      disable       dflt
    
    //TCA6424A_ADDR_l, P0 --------------start--------------
    {ADS129X_GPIO_EXT_PIN_LOFPU_SEL,                {TCA6424A_ADDR_L,  0,  0},        0,          1,          0,          0},           //右腿电极自检下拉电阻选择
    {ADS129X_GPIO_EXT_PIN_LEAD_AC_SEL,              {TCA6424A_ADDR_L,  0,  1},        0,          1,          0,          1},           //导联直流或交流通道选择     0: AC  1: DC   TMUX1136, 高SxA通(DC), 低SxB通(AC)
    {ADS129X_GPIO_EXT_PIN_LEAD_DC_SEL,              {TCA6424A_ADDR_L,  0,  2},        0,          1,          0,          1},           //导联直流或交流通道选择     0: AC  1: DC   TMUX1136, 高SxA通(DC), 低SxB通(AC)
    {ADS129X_GPIO_EXT_PIN_P03,                      {TCA6424A_ADDR_L,  0,  3},        1,          1,          0,          0},           //not use
    {ADS129X_GPIO_EXT_PIN_P04,                      {TCA6424A_ADDR_L,  0,  4},        1,          1,          0,          0},           //not use
    {ADS129X_GPIO_EXT_PIN_P05,                      {TCA6424A_ADDR_L,  0,  5},        1,          1,          0,          0},           //not use
    {ADS129X_GPIO_EXT_PIN_P06,                      {TCA6424A_ADDR_L,  0,  6},        1,          1,          0,          0},           //not use
    {ADS129X_GPIO_EXT_PIN_RLD_SCAN_RES_UP_SEL,      {TCA6424A_ADDR_L,  0,  7},        0,          1,          0,          0},           //右腿电极自检上拉电阻选择   TMUX1112(高开低关)
                                                                                                                          
    //TCA6424A_ADDR_L, P1                                                                                                 
    {ADS129X_GPIO_EXT_PIN_RLD_SCAN_RES_DOWN_SEL,    {TCA6424A_ADDR_L,  1,  0},        0,          1,          0,          0},           //右腿电极自检下拉电阻选择   TMUX1112(高开低关)
    {ADS129X_GPIO_EXT_PIN_RLD_SELF_DISCON_SEL,      {TCA6424A_ADDR_L,  1,  1},        0,          1,          0,          1},           //右腿电极断开右腿驱动信号源，自检准备阶段   TMUX1112(高开低关)
    {ADS129X_GPIO_EXT_PIN_RLD_TERMINAL_CON_SEL,     {TCA6424A_ADDR_L,  1,  2},        0,          1,          0,          1},           //右腿电极及自检电极连接一起，正常工作时     TMUX1112(高开低关)
    {ADS129X_GPIO_EXT_PIN_RLD_OUT1_SEL1,            {TCA6424A_ADDR_L,  1,  3},        0,          1,          0,          1},           //ADS129X RLDOUT直接输出
    {ADS129X_GPIO_EXT_PIN_RLD_OUT2_SEL,             {TCA6424A_ADDR_L,  1,  4},        0,          1,          0,          0},           //ADS129X RLDOUT经过运放后再输出 需与ADS129X_GPIO_EXT_PIN_RLD_OUT1_SEL2同时开启且
                                                                                                                                        //ADS129X_GPIO_EXT_PIN_RLD_OUT1_SEL1需关闭        TMUX1112(高开低关)
    {ADS129X_GPIO_EXT_PIN_RLD_OUT1_SEL2,            {TCA6424A_ADDR_L,  1,  5},        0,          1,          0,          0},           //ADS129X RLDOUT经过运放后再输出 需与ADS129X_GPIO_EXT_PIN_RLD_OUT1_SEL1同时开启且
                                                                                                                                        //ADS129X_GPIO_EXT_PIN_RLD_OUT1_SEL1需关闭        TMUX1112(高开低关)
    {ADS129X_GPIO_EXT_PIN_RLD_INV_SEL,              {TCA6424A_ADDR_L,  1,  6},        0,          1,          0,          0},           //ADS129X RLDOUT和RLDINV直连输出        TMUX1112(高开低关)
    {ADS129X_GPIO_EXT_PIN_P17,                      {TCA6424A_ADDR_L,  1,  7},        1,          1,          0,          0},           //not use
    
    //TCA6424A_ADDR_L, P2                                                                                                 
    {ADS129X_GPIO_EXT_PIN_LED_R,                    {TCA6424A_ADDR_L,  2,  0},        0,          1,          0,          0},           //not use
    {ADS129X_GPIO_EXT_PIN_LED_G,                    {TCA6424A_ADDR_L,  2,  1},        0,          1,          0,          0},           //not use
    {ADS129X_GPIO_EXT_PIN_LED_B,                    {TCA6424A_ADDR_L,  2,  2},        0,          1,          0,          0},           //not use
    {ADS129X_GPIO_EXT_PIN_P23,                      {TCA6424A_ADDR_L,  2,  3},        1,          1,          0,          0},           //not use
    {ADS129X_GPIO_EXT_PIN_P24,                      {TCA6424A_ADDR_L,  2,  4},        1,          1,          0,          0},           //not use
    {ADS129X_GPIO_EXT_PIN_P25,                      {TCA6424A_ADDR_L,  2,  5},        1,          1,          0,          0},           //not use
    {ADS129X_GPIO_EXT_PIN_P26,                      {TCA6424A_ADDR_L,  2,  6},        1,          1,          0,          0},           //not use
    {ADS129X_GPIO_EXT_PIN_P27,                      {TCA6424A_ADDR_L,  2,  7},        1,          1,          0,          0},           //not use  
};

static void set_bit(U8 *data, U8 bit, U8 v)
{
    if(v==0) {
        *data &= ~(1<<bit);
    }
    else {
        *data |= 1<<bit;
    }
}
static void ad8233_cfg_conv(tca6424a_regs_t *regs)
{
    int i,j;
    tca6424a_set_t *tca;
    gpio_ext_cfg_t *cfg;
    tca6424a_reg_t *reg;
    
    memset(regs, 0, sizeof(tca6424a_regs_t));
    for(i=0; i<AD8233_GPIO_EXT_MAX; i++) {
        cfg = &ad8233_gpio_cfg[i];
        reg = (cfg->tca.addr==TCA6424A_ADDR_L)?&regs->l:&regs->h;
        
        //set_bit(&reg->input[cfg->tca.index],    cfg->tca.bit, cfg->dir);
        set_bit(&reg->output[cfg->tca.index],   cfg->tca.bit, cfg->dflt);
        //set_bit(&reg->polarity[cfg->tca.index], cfg->tca.bit, 0);
        set_bit(&reg->config[cfg->tca.index],   cfg->tca.bit, cfg->dir);
    }
}
static void ads129x_cfg_conv(tca6424a_reg_t *reg)
{
    int i,j;
    tca6424a_set_t *tca;
    gpio_ext_cfg_t *cfg;
    
    memset(reg, 0, sizeof(tca6424a_reg_t));
    for(i=0; i<ADS129X_GPIO_EXT_MAX; i++) {
        cfg = &ads129x_gpio_cfg[i];
        
        //set_bit(&reg->input[cfg->tca.index],    cfg->tca.bit, cfg->dir);
        set_bit(&reg->output[cfg->tca.index],   cfg->tca.bit, cfg->dflt);
        //set_bit(&reg->polarity[cfg->tca.index], cfg->tca.bit, 0);
        set_bit(&reg->config[cfg->tca.index],   cfg->tca.bit, cfg->dir);
    }
}
static int load_setting(void)
{
    int r=0;
    if(bioDevice==AD8233) {
        tca6424a_regs_t regs;
        
        ad8233_cfg_conv(&regs);   
        r |= tca6424a_write_reg(TCA6424A_ADDR_H, &regs.h);
        r |= tca6424a_write_reg(TCA6424A_ADDR_L, &regs.l);
    }
    else if(bioDevice==ADS129X) {
        tca6424a_reg_t reg;

        ads129x_cfg_conv(&reg);   
        r |= tca6424a_write_reg(TCA6424A_ADDR_L, &reg);
    }
    
    return r;
}
#endif

////////////////////////////////////////////////////////////
int gpio_ext_init(void)
{
    int r;
    
#ifdef USE_TCA6424A
    r = tca6424a_init();
    r |= load_setting();
    if(r) {
        LOGE("___ load_setting error\r\n");
    }
#endif
    
    return 0;
}


int gpio_ext_get_hl(U8 pin, U8 *hl)
{
    return 0;
}


int gpio_ext_set_hl(U8 pin, U8 hl)
{
    int r;
    U8 reg;
    
#ifdef USE_TCA6424A
    tca6424a_set_t *tca;
    
    if(bioDevice==AD8233) {
        tca = &ad8233_gpio_cfg[pin].tca;
    }
    else if(bioDevice==ADS129X) {
        tca = &ads129x_gpio_cfg[pin].tca;
    }
    else {
        return -1;
    }
    
    r = tca6424a_read(tca->addr, TCA6424A_OUTPUT_REG0+tca->index, &reg, 1);
    if(hl) {
        reg |= 1<<tca->bit;
    }
    else {
        reg &= ~(1<<tca->bit);
    }
    r |= tca6424a_write(tca->addr, TCA6424A_OUTPUT_REG0+tca->index, &reg, 1);
    if(r) {
        LOGE("___ gpio_ext_set_hl error， pin: %d, hl: %d\r\n", pin, hl);
    }
#endif
    
    return r;
}


int gpio_ext_int_set(U8 pin, gpio_callback_t *cb)
{
    return 0;
}

int gpio_ext_test(void)
{
    U8 tmp;

    return 0;
}


int gpio_init(gpio_pin_t *pin, U8 dir)
{
    if(!pin) {
        return -1;
    }
    
    hal_gpio_init(pin->pin);
	hal_pinmux_set_function(pin->pin, 0);
    
	hal_gpio_set_direction(pin->pin, (hal_gpio_direction_t)dir);
    hal_gpio_set_output(pin->pin, (hal_gpio_data_t)0);
    
    return 0;
}


int gpio_deinit(gpio_pin_t *pin)
{
    if(!pin) {
        return -1;
    }
    
    return hal_gpio_deinit(pin->pin);
}


int gpio_pull_up(gpio_pin_t *pin)
{
    if(!pin) {
        return -1;
    }
    
    hal_gpio_pull_up(pin->pin);
    return 0;
}


int gpio_pull_down(gpio_pin_t *pin)
{
    if(!pin) {
        return -1;
    }
    
    hal_gpio_pull_down(pin->pin);
    return 0;
}


int gpio_get_hl(gpio_pin_t *pin)
{
    if(!pin) {
        return -1;
    }
    
    hal_gpio_data_t gData;
    hal_gpio_get_input(pin->pin, &gData);
    
    return (int)gData;
}

int gpio_set_dir(gpio_pin_t *pin, U8 dir)
{
    if(!pin) {
        return -1;
    }
    
    hal_gpio_set_direction(pin->pin, (hal_gpio_direction_t)dir);
    return 0;
}


int gpio_set_hl(gpio_pin_t *pin, U8 hl)
{
    if(!pin) {
        return -1;
    }
    
    hal_gpio_set_output(pin->pin, (hal_gpio_data_t)hl);
    return 0;
}


int gpio_int_init(gpio_pin_t *pin, gpio_callback_t *cb)
{
    hal_eint_config_t cfg;
    gpio_eint_t  *ge=NULL;
    
    if(!pin) {
        return -1;
    }
    
    ge = &gpio_eint[pin->pin];
    
    hal_gpio_init(pin->pin);
    hal_pinmux_set_function(pin->pin, ge->mode);
    hal_gpio_set_direction(pin->pin, HAL_GPIO_DIRECTION_INPUT);
    hal_gpio_disable_pull(pin->pin);
    
    cfg.debounce_time = 0;
    cfg.trigger_mode = cb->mode;
    
    //hal_eint_mask(ge->num);
    hal_eint_init(ge->num, &cfg);
    hal_eint_register_callback(ge->num, cb->callback, cb->user_data);
    //hal_eint_unmask(ge->num);
    
    return 0;
}


int gpio_int_en(gpio_pin_t *pin, U8 on)
{
    hal_eint_number_t num;
    
    if(!pin) {
        return -1;
    }
    
    num=gpio_eint[pin->pin].num;
    if(on) {
        hal_eint_unmask(num);
    }
    else {
        hal_eint_mask(num);
    }
    
    return 0;
}






