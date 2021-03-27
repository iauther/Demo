#ifndef __GPIO_Hx__
#define __GPIO_Hx__

#include "types.h"
#include "tca6424a.h"
#include "cfg.h"

typedef void (*gpio_callback)(void *user_data);
typedef struct {
    hal_eint_trigger_mode_t     mode;
    gpio_callback               callback;
    void                        *user_data;
}gpio_callback_t;



typedef struct {
    U8                  pin;
    tca6424a_set_t      tca;
    U8                  dir;            //0: output   1: input
    U8                  enable;         //0: low      1: high
    U8                  disable;        //0: low      1: high
    U8                  dflt;           //0: low      1: high
}gpio_ext_cfg_t;

enum {
#ifdef BOARD_V00_01
    //TCA6424A_ADDR_H, P0
    AD8233_GPIO_EXT_PIN_RLD_SDN_CH3=0,         //8233右腿驱动使能
    AD8233_GPIO_EXT_PIN_RLD_SDN_CH2,
    AD8233_GPIO_EXT_PIN_RLD_SDN_CH1,
    AD8233_GPIO_EXT_PIN_NSDN_CH8,              //8233芯片使能
    AD8233_GPIO_EXT_PIN_NSDN_CH7,
    AD8233_GPIO_EXT_PIN_NSDN_CH6,
    AD8233_GPIO_EXT_PIN_NSDN_CH5,
    AD8233_GPIO_EXT_PIN_NSDN_CH4,
    
    //TCA6424A_ADDR_H, P1
    AD8233_GPIO_EXT_PIN_NSDN_CH3,
    AD8233_GPIO_EXT_PIN_NSDN_CH2,
    AD8233_GPIO_EXT_PIN_NSDN_CH1,
    AD8233_GPIO_EXT_PIN_ACDC_SEL_CH1,          //8233导联脱落检测交流直流功能选择
    AD8233_GPIO_EXT_PIN_ACDC_SEL_CH2,
    AD8233_GPIO_EXT_PIN_ACDC_SEL_CH3,
    AD8233_GPIO_EXT_PIN_ACDC_SEL_CH4,
    AD8233_GPIO_EXT_PIN_ACDC_SEL_CH5,
    
    //TCA6424A_ADDR_H, P2
    AD8233_GPIO_EXT_PIN_ACDC_SEL_CH6,
    AD8233_GPIO_EXT_PIN_ACDC_SEL_CH7,
    AD8233_GPIO_EXT_PIN_ACDC_SEL_CH8,
    
    AD8233_GPIO_EXT_PIN_LEAD_DC_SEL,               //导联直流或交流通道选择
    AD8233_GPIO_EXT_PIN_LEAD_AC_SEL,               //导联直流或交流通道选择
    AD8233_GPIO_EXT_PIN_FR_CH_SEL,                 //8233复位
    AD8233_GPIO_EXT_PIN_LOFPU_SEL,                 //导联直流脱落检测时上拉电阻使能, 暂未使用
    AD8233_GPIO_EXT_PIN_P27,     
#else
    AD8233_GPIO_EXT_PIN_RLD_SDN_CH3, 
    AD8233_GPIO_EXT_PIN_RLD_SDN_CH2, 
    AD8233_GPIO_EXT_PIN_RLD_SDN_CH1, 
    AD8233_GPIO_EXT_PIN_NSDN_CH7,    
    AD8233_GPIO_EXT_PIN_NSDN_CH6,    
    AD8233_GPIO_EXT_PIN_NSDN_CH5,    
    AD8233_GPIO_EXT_PIN_NSDN_CH4,    
    AD8233_GPIO_EXT_PIN_NSDN_CH3,    

    AD8233_GPIO_EXT_PIN_NSDN_CH2,    
    AD8233_GPIO_EXT_PIN_NSDN_CH1,    
    AD8233_GPIO_EXT_PIN_ACDC_SEL_CH1,
    AD8233_GPIO_EXT_PIN_ACDC_SEL_CH2,
    AD8233_GPIO_EXT_PIN_ACDC_SEL_CH3,
    AD8233_GPIO_EXT_PIN_ACDC_SEL_CH4,
    AD8233_GPIO_EXT_PIN_ACDC_SEL_CH5,
    AD8233_GPIO_EXT_PIN_ACDC_SEL_CH6,

    AD8233_GPIO_EXT_PIN_ACDC_SEL_CH7,
    AD8233_GPIO_EXT_PIN_LEAD_DC_SEL, 
    AD8233_GPIO_EXT_PIN_LEAD_AC_SEL, 
    AD8233_GPIO_EXT_PIN_FR_CH_SEL,   
    AD8233_GPIO_EXT_PIN_LOFPU_SEL,   
    AD8233_GPIO_EXT_PIN_LED_R,
    AD8233_GPIO_EXT_PIN_LED_G,
    AD8233_GPIO_EXT_PIN_LED_B,

#endif

    //TCA6424A_ADDR_L, P0
    AD8233_GPIO_EXT_PIN_RA_OUTPUT_SEL,             //WCT中心电位合成信号源选择
    AD8233_GPIO_EXT_PIN_LA_OUTPUT_SEL,
    AD8233_GPIO_EXT_PIN_LL_OUTPUT_SEL,
    AD8233_GPIO_EXT_PIN_RLD_FB_SEL_CH1,            //右腿驱动信号来源选择
    AD8233_GPIO_EXT_PIN_RLD_FB_SEL_CH2,
    AD8233_GPIO_EXT_PIN_RLD_FB_SEL_CH3,
    AD8233_GPIO_EXT_PIN_RLD_WCT_SEL_CH4,
    AD8233_GPIO_EXT_PIN_RLD_SCAN_RES_UP_SEL,       //右腿电极自检上拉电阻选择

    //TCA6424A_ADDR_L, P1
    AD8233_GPIO_EXT_PIN_RLD_SCAN_RES_DOWN_SEL,     //右腿电极自检下拉电阻选择
    AD8233_GPIO_EXT_PIN_RLD_SELF_DISCON_SEL,       //右腿电极断开右腿驱动信号源，自检准备阶段
    AD8233_GPIO_EXT_PIN_RLD_TERMINAL_CON_SEL,      //右腿电极及自检电极连接一起，正常工作时
    AD8233_GPIO_EXT_PIN_RLD_CH_SEL_A0,             //右腿驱动信号来源选择
    AD8233_GPIO_EXT_PIN_RLD_CH_SEL_A1,             //右腿驱动信号来源选择
    AD8233_GPIO_EXT_PIN_RLD_AC_SEL,                //右腿电极自检AD检测是否跳过BUFFER
    AD8233_GPIO_EXT_PIN_WCT_OUT_SEL,               //胸导WCT信号来源选择，故障时可以使用右腿自检电极

    //TCA6424A_ADDR_L, P2
    AD8233_GPIO_EXT_PIN_LOD_CH1,
    AD8233_GPIO_EXT_PIN_LOD_CH2,
    AD8233_GPIO_EXT_PIN_LOD_CH3,
    AD8233_GPIO_EXT_PIN_LOD_CH4,
    AD8233_GPIO_EXT_PIN_LOD_CH5,
    AD8233_GPIO_EXT_PIN_LOD_CH6,
    AD8233_GPIO_EXT_PIN_LOD_CH7,
    AD8233_GPIO_EXT_PIN_LOD_CH8,
    
    AD8233_GPIO_EXT_PIN_MAX
};

enum {
    //TCA6424A_ADDR_L, P0
    ADS129X_GPIO_EXT_PIN_LOFPU_SEL,                         //右腿电极自检下拉电阻选择
    ADS129X_GPIO_EXT_PIN_LEAD_AC_SEL,                       //导联直流或交流通道选择
    ADS129X_GPIO_EXT_PIN_LEAD_DC_SEL,                       //导联直流或交流通道选择
    ADS129X_GPIO_EXT_PIN_P03,                               //not use
    ADS129X_GPIO_EXT_PIN_P04,                               //not use
    ADS129X_GPIO_EXT_PIN_P05,                               //not use
    ADS129X_GPIO_EXT_PIN_P06,                               //not use
    ADS129X_GPIO_EXT_PIN_RLD_SCAN_RES_UP_SEL,               //右腿电极自检上拉电阻选择

    //TCA6424A_ADDR_L, P1
    ADS129X_GPIO_EXT_PIN_RLD_SCAN_RES_DOWN_SEL,             //右腿电极自检下拉电阻选择
    ADS129X_GPIO_EXT_PIN_RLD_SELF_DISCON_SEL,               //右腿电极断开右腿驱动信号源，自检准备阶段
    ADS129X_GPIO_EXT_PIN_RLD_TERMINAL_CON_SEL,              //右腿电极及自检电极连接一起，正常工作时
    ADS129X_GPIO_EXT_PIN_RLD_OUT1_SEL1,                     //???
    ADS129X_GPIO_EXT_PIN_RLD_OUT2_SEL,                      //???
    ADS129X_GPIO_EXT_PIN_RLD_OUT1_SEL2,                     //???
    ADS129X_GPIO_EXT_PIN_RLD_INV_SEL,                       //???
    ADS129X_GPIO_EXT_PIN_P17,
    
    //TCA6424A_ADDR_L, P2
    ADS129X_GPIO_EXT_PIN_LED_R,
    ADS129X_GPIO_EXT_PIN_LED_G,
    ADS129X_GPIO_EXT_PIN_LED_B,
    ADS129X_GPIO_EXT_PIN_P23,
    ADS129X_GPIO_EXT_PIN_P24,
    ADS129X_GPIO_EXT_PIN_P25,
    ADS129X_GPIO_EXT_PIN_P26,
    ADS129X_GPIO_EXT_PIN_P27,

    ADS129X_GPIO_EXT_PIN_MAX
};

#define AD8233_GPIO_EXT_MAX     48
#define ADS129X_GPIO_EXT_MAX    24


int gpio_ext_init(void);
int gpio_ext_get_hl(U8 pin, U8 *hl);
int gpio_ext_set_hl(U8 pin, U8 hl);
int gpio_ext_int_init(U8 pin, gpio_callback_t *cb);
int gpio_ext_test(void);

int gpio_init(hal_gpio_pin_t pin, hal_gpio_direction_t dir, hal_gpio_data_t hl);
int gpio_deinit(hal_gpio_pin_t pin);
int gpio_pull_up(hal_gpio_pin_t pin);
int gpio_pull_down(hal_gpio_pin_t pin);
int gpio_get_hl(hal_gpio_pin_t pin);
int gpio_set_dir(hal_gpio_pin_t pin, hal_gpio_direction_t dir);
int gpio_set_hl(hal_gpio_pin_t pin,  hal_gpio_data_t hl);
int gpio_int_init(hal_gpio_pin_t pin, gpio_callback_t *cb);
int gpio_int_en(hal_gpio_pin_t pin, U8 on);

#endif
