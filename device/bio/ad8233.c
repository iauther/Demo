#include "bio/ad8233.h"
#include "bio/ads8866.h"
#include "tca6424a.h"
#include "drv/gpio.h"
#include "drv/htimer.h"
#include "log.h"
#include "drv/delay.h"
#include "bio/bio.h"
#include "myCfg.h"


//#define USE_TIMER

#define LOFF_INT_PIN        HAL_GPIO_24


typedef enum {
    DIY_FB_I=0,
    DIY_FB_II,
    DIY_FB_III,
    DIY_WCT,
}DIY;
typedef enum {
    LA,
    RA,
    LL,
}WCT;

static int ad8233_read_ecg(ecg_data_t *ecg);
static void set_lead_current(U8 lead, U8 current);
static void set_pullup(U8 on);
static void set_ad8244_bypass(U8 on);
static void set_wct(WCT wct, U8 on);
static void set_diy(DIY diy, U8 on);
static void select_rld(U8 fun);

static bio_setting_t ad8233_sett={
#ifdef DC_LOFF_DETECT
    DC,
#else
    AC,
#endif
    
    {//wct_set_t
        {1, 1, 1, 0, 0, 0, 0, 0},       //
    },
    
    {//rld_set_t
        {1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       //in
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       //out
    },
};


static U8 ad8233_lead_index[LEAD_MAX]={
#ifdef BOARD_V01_02
    6,        //LEAD_I
    5,        //LEAD_II
    4,        //LEAD_III
    3,        //LEAD_V1
    2,        //LEAD_V2
    1,        //LEAD_V3
    0,        //LEAD_V4
    
    LEAD_MAX
#else
    7,        //LEAD_I
    6,        //LEAD_II
    5,        //LEAD_III
    4,        //LEAD_V1
    3,        //LEAD_V2
    2,        //LEAD_V3
    1,        //LEAD_V4
              
    0,        //LEAD_RLD
#endif
    
};


static handle_t ad8233_tmr_handle;
static U16 ad8233_buffer[LEAD_MAX];
static bio_callback_t ad8233_callback={0};
#define GAIN            (300.0)
#define VREF            (3.0)
#define ADCMAX          (0xffff)
#define LSB             (VREF/ADCMAX)
#define ADC2VOL(adc)    ((adc*LSB)/GAIN)
#define MVALUE          ((U32)(GAIN/(LSB*1000)))
#define SWAP16(x)       (((x&0x00ff)<<8)|((x&0xff00)>>8))
static void data_convert(U16 *data, ecg_data_t *ecg)
{
    int i;
    U16 adc;
    for(i=0; i<LEAD_MAX; i++) {
        if(ad8233_lead_index[i]<LEAD_MAX) {
            adc = SWAP16(data[ad8233_lead_index[i]]);
            ecg->adc[i] = adc;
        }
    }
}
static void ad8233_timer_callback(void *user_data)
{
    if(ad8233_callback.ecgCallback) {
        ad8233_callback.ecgCallback(ad8233_callback.user_data);
    }
}


static void set_setting(bio_setting_t *sett)
{
    //U8 i;     
    
    
}



static void ad8233_loff_callback(void *user_data)
{
    gpio_pin_t p={LOFF_INT_PIN};
    //LOGD("_____ loff callback\r\n");
    gpio_int_en(&p, 0);
    if(ad8233_callback.loffCallback) {
        ad8233_callback.loffCallback(user_data);
    }
    gpio_int_en(&p, 1);
}
//////////////////////////////////////////////////

int ad8233_probe(void)
{
    U8 tmp;
    return tca6424a_read(TCA6424A_ADDR_H, TCA6424A_OUTPUT_REG0, &tmp, 1); 
}



static int ad8233_init(void)
{
    int r;
    
    r = ads8866_init();
    if(r==0) {
        set_setting(&ad8233_sett);
    }
    
#ifdef USE_TIMER
    ad8233_tmr_handle = timer_new();
#endif
    
    return r;
}


static int ad8233_reset(void)
{
    
    return 0;
}


static int ad8233_get_info(bio_info_t *info)
{
    if(!info) {
        return -1;
    }
    info->dev = AD8233;
    info->mvValue = MVALUE;
    
    return 0;
}


static int ad8233_get_setting(bio_setting_t *sett)
{
    if(!sett) {
        return -1;
    }
    *sett = ad8233_sett;
    
    return 0;
}


static int ad8233_set_current(U8 current)
{
    if(current!=DC || current!=AC) {
        return -1;
    }
    ad8233_sett.current = current;
    
    return 0;
}


static int ad8233_read_ecg(ecg_data_t *ecg)
{
    if(!ecg) {
        return -1;
    }

#ifdef BOARD_V01_02
    ads8866_read((U8*)ad8233_buffer, sizeof(U16)*(LEAD_MAX-1));
#else
    ads8866_read((U8*)ad8233_buffer, sizeof(U16)*LEAD_MAX);
#endif
    if(ecg) {
        ecg->bits = 16;
        data_convert(ad8233_buffer, ecg);
    }
    
    return 0;
}


#define SET_BIT(d,b,v)  \
        if(v) d |= 1<<b;\
        else  d &= ~(1<<b)
static U8 loff_lead_to_etrode(U8 *lead)
{
    U8 loff = 0;
    if(lead[LEAD_I] && lead[LEAD_II]) {
        SET_BIT(loff, ET_RA, 1);
        if(lead[LEAD_III]) {
            SET_BIT(loff, ET_LA, 1);
            SET_BIT(loff, ET_LL, 1);
        }
    }
    else if(lead[LEAD_I] && lead[LEAD_III]) {
        SET_BIT(loff, ET_LA, 1);
        if(lead[LEAD_II]) {
            SET_BIT(loff, ET_RA, 1);
            SET_BIT(loff, ET_LL, 1);
        }
    }
    else if(lead[LEAD_II] && lead[LEAD_III]) {
        SET_BIT(loff, ET_LL, 1);
        if(lead[LEAD_I]) {
            SET_BIT(loff, ET_LA, 1);
            SET_BIT(loff, ET_RA, 1);
        }
    }

#ifndef BOARD_V01_02
    SET_BIT(loff, ET_RLD, lead[LEAD_RLD]);
#endif
    
    SET_BIT(loff, ET_V1,  lead[LEAD_V1]);
    SET_BIT(loff, ET_V2,  lead[LEAD_V2]);
    SET_BIT(loff, ET_V3,  lead[LEAD_V3]);
    SET_BIT(loff, ET_V4,  lead[LEAD_V4]);
    
    return loff;
}

static U8 rld_is_off(U8 v)
{
    int r;
    F32 vol;
    U16 dat;

    r = ads8866_read((U8*)&dat, 2);
    if(r==0) {
        vol = SWAP16(dat)*LSB;
        //LOGD("______vol: %0.2f V\r\n", vol);
        if(vol<0.1F) {
            return 1;
        }
    }
    
    return 0;
}

static void set_all_wct(U8 on)
{
    set_wct(LA, on);
    set_wct(RA, on);
    set_wct(LL, on);
}
static U8 get_lead_loff(U8 lead)
{
    U8  x,tmp=0;
    int dly=5;
    
    switch(lead) {
        case LEAD_I:
        set_lead_current(LEAD_I, AC);set_lead_current(LEAD_II, DC);set_lead_current(LEAD_III, DC);
        select_rld(RLD_SRC_I);
        break;
        
        case LEAD_II:
        set_lead_current(LEAD_I, DC);set_lead_current(LEAD_II, AC);set_lead_current(LEAD_III, DC);
        select_rld(RLD_SRC_II);
        break;
        
        case LEAD_III:
        set_lead_current(LEAD_I, DC);set_lead_current(LEAD_II, DC);set_lead_current(LEAD_III, AC);
        select_rld(RLD_SRC_III);
        break;
        
        default:
        return 0;
    }
    
    delay_ms(dly);
    tca6424a_read(TCA6424A_ADDR_L, TCA6424A_INPUT_REG0+2, &tmp, 1);
    x = (tmp&(1<<lead))?1:0;
    
    set_lead_current(LEAD_I, AC);set_lead_current(LEAD_II, AC);set_lead_current(LEAD_III, AC);
    select_rld(RLD_SRC_DIY);
    
    return x;
}
static void ac_get_loff(U8 *lead)
{
    U8 i,tmp;
    int dly=20;
    
    set_ad8244_bypass(1);
    
    tca6424a_read(TCA6424A_ADDR_L, TCA6424A_INPUT_REG0+2, &tmp, 1);
    for(i=LEAD_V1; i<=LEAD_V4; i++) {
        lead[i] = (tmp&(1<<i))?1:0;
    }
    
    lead[LEAD_I] = get_lead_loff(LEAD_I);
    lead[LEAD_II] = get_lead_loff(LEAD_II);
    lead[LEAD_III] = get_lead_loff(LEAD_III);
    
#ifndef BOARD_V01_02 
    lead[LEAD_RLD] = 0;
#endif

    set_ad8244_bypass(0);
    
}


static void dc_get_loff(U8 *lead)
{
    U8 i,tmp,dly=5;
    
    //set_ad8244_bypass(1);
    set_pullup(1);
    delay_ms(5);
    
    tca6424a_read(TCA6424A_ADDR_L, TCA6424A_INPUT_REG0+2, &tmp, 1);
    for(i=LEAD_I; i<=LEAD_V4; i++) {  
        lead[i] = (tmp&(1<<i))?1:0;
    }
    
#ifndef BOARD_V01_02 
    lead[LEAD_RLD] = 0;
#endif
    
    set_pullup(0);
    //set_ad8244_bypass(0);
}



static int ad8233_read_loff(loff_data_t *loff)
{
    int r=-1;
    U8  lead[LEAD_MAX]={0};
    U8 i,t,tmp,rld_off=0;
    
    if(!loff) {
        return -1;
    }
    memset(loff, 0, sizeof(loff_data_t));
    
    return 0;           //因为脱落检测对信号采集有影响，暂不做检测，直接返回 @2021.02.27
    
    rld_off = rld_is_off(tmp);
    if(rld_off) {
        loff->loff = 0xff;
    }
    else {
        
        if(ad8233_sett.current==AC) {
            ac_get_loff(lead);
        }
        else {
            dc_get_loff(lead);
        }
        
        loff->loff = loff_lead_to_etrode(lead);
    }
    //LOGD("lead: %d, %d, %d, %d, %d, %d, %d, %d, loff: 0x%02x\r\n", lead[0], lead[1], lead[2], lead[3], lead[4], lead[5], lead[6], lead[7], loff->loff);

    return r;
}


static int ad8233_get_rld(rld_set_t *rld)
{
    if(!rld) {
        return -1;
    }
    *rld = ad8233_sett.rld;
    
    return 0;
}


static int ad8233_set_rld(rld_set_t *rld)
{
    if(!rld) {
        return -1;
    }
    ad8233_sett.rld = *rld;
    
    return 0;
}


static int ad8233_get_wct(wct_set_t *wct)
{
    if(!wct) {
        return -1;
    }
    *wct = ad8233_sett.wct;
    
    return 0;
}


static int ad8233_set_wct(wct_set_t *wct)
{
    if(!wct) {
        return -1;
    }
    ad8233_sett.wct = *wct;
    
    return 0;
}    



static int ad8233_start(void)
{
    int r=0;
    
#ifdef USE_TIMER
    r = timer_start(ad8233_tmr_handle);
#endif
    
    return r;
}


static int ad8233_stop(void)
{
    int r=0;
    
#ifdef USE_TIMER
    r = timer_stop(ad8233_tmr_handle);
#endif
    
    return r;
}


static int ad8233_set_callback(bio_callback_t *cb)
{
    htimer_set_t set;
    gpio_callback_t gc;
    
    if(!cb) {
        return -1;
    }
    
    ad8233_callback = *cb;
    
    gc.mode = HAL_EINT_EDGE_FALLING_AND_RISING;
    gc.callback = ad8233_loff_callback;
    gc.user_data = cb->user_data;
    //gpio_int_init(LOFF_INT_PIN, &gc);
    //gpio_int_en(LOFF_INT_PIN, 1);

#ifdef USE_TIMER    
    set.ms = 0;
    set.freq = cb->freq;
    set.repeat = 1;
    set.callback = ad8233_timer_callback;
    
    htimer_set(ad8233_tmr_handle, &set);
#endif
    
    return 0;
}

static int ad8233_get_stat(bio_stat_t *stat)
{
    int r;
    U16 vol;
    static int stat_cnt=0;
    
    if(!stat) {
        return -1;
    }
    
    stat->current = ad8233_sett.current;
    r = ad8233_read_loff(&stat->loff);
    
    return r;
}



/////////////electrode switch functions start//////////////
static void select_rld(U8 fun)
{
    switch(fun) {
        case RLD_SRC_I:
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_RLD_CH_SEL_A0, 0);
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_RLD_CH_SEL_A1, 0);
        break;
        
        case RLD_SRC_II:
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_RLD_CH_SEL_A0, 1);
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_RLD_CH_SEL_A1, 0);
        break;
        
        case RLD_SRC_III:
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_RLD_CH_SEL_A0, 0);
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_RLD_CH_SEL_A1, 1);
        break;
        
        case RLD_SRC_DIY:
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_RLD_CH_SEL_A0, 1);
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_RLD_CH_SEL_A1, 1);
        break;
    }
}


static void set_diy(DIY diy, U8 on)
{
    U8 hl=on?1:0;
    
    switch(diy) {
        case DIY_FB_I:
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_RLD_FB_SEL_CH1, hl);
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_RLD_SDN_CH1, 0);
        break;
        
        case DIY_FB_II:
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_RLD_FB_SEL_CH2, !hl);
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_RLD_SDN_CH2, 0);
        break;
        
        case DIY_FB_III:
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_RLD_FB_SEL_CH3, hl);
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_RLD_SDN_CH3, 0);
        break;
        
        case DIY_WCT:
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_RLD_WCT_SEL_CH4, hl);
        break;
    }
}
static void select_rld_diy(U8 fun)
{
    if(fun&RLD_DIY_SRC_FB_I)    set_diy(DIY_FB_I, 1);
    else                        set_diy(DIY_FB_I, 0);
    
    if(fun&RLD_DIY_SRC_FB_II)   set_diy(DIY_FB_II, 1);
    else                        set_diy(DIY_FB_II, 0);
    
    if(fun&RLD_DIY_SRC_FB_III)  set_diy(DIY_FB_III, 1);
    else                        set_diy(DIY_FB_III, 0);
    
    if(fun&RLD_DIY_SRC_WCT)     set_diy(DIY_WCT, 1);
    else                        set_diy(DIY_WCT, 0);
    
    select_rld(RLD_SRC_DIY);
}


static void set_wct(WCT wct, U8 on)
{
    U8 hl=on?1:0;
    
    switch(wct) {
        case LA:
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_LA_OUTPUT_SEL, hl);
        break;
        
        case RA:
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_RA_OUTPUT_SEL, hl);
        break;
        
        case LL:
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_LL_OUTPUT_SEL, hl);
        break;
    }
}
static void select_wct(U8 fun)
{
    if(fun&RLD_DIY_SRC_WCT_RA)  set_wct(RA, 1);
    else                        set_wct(RA, 0);
    
    if(fun&RLD_DIY_SRC_WCT_LA)  set_wct(LA, 1);
    else                        set_wct(LA, 0);
    
    if(fun&RLD_DIY_SRC_WCT_LL)  set_wct(LL, 1);
    else                        set_wct(LL, 0);
}


static void messure_rld(U8 on)
{
    if(on) {
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_RLD_SELF_DISCON_SEL, 0);               //断开右腿通路，切换到阻抗测试电路
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_RLD_TERMINAL_CON_SEL, 0);              //断开两瓣儿电极的连接，使两部分形成回路
        
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_RLD_SCAN_RES_UP_SEL, 1);
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_RLD_SCAN_RES_DOWN_SEL, 1);
        
    }
    else {
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_RLD_SELF_DISCON_SEL, 1);              //恢复右腿通路，进入正常状态
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_RLD_TERMINAL_CON_SEL, 1);              //连接两瓣儿电极，使两部分形成回路
        
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_RLD_SCAN_RES_UP_SEL, 0);
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_RLD_SCAN_RES_DOWN_SEL, 0);
    }
}

static void set_pullup(U8 on)
{
    U8 hl=on?1:0;
    gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_LOFPU_SEL, hl);
}

static void set_ad8244_bypass(U8 on)
{
    U8 hl=on?0:1;
    
    gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_LEAD_DC_SEL,  hl);
    gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_LEAD_AC_SEL,  hl);
}

static void set_loff_current(U8 current)
{
    U8 hl=(current==AC)?1:0;
    
    gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_ACDC_SEL_CH1, 0);             //AD8233,   高交流，低直流
    gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_ACDC_SEL_CH2, 0);             //AD8233,   高交流，低直流
    gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_ACDC_SEL_CH3, 0);
    if(current==AC) {
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_ACDC_SEL_CH4, 1);
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_ACDC_SEL_CH5, 1);
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_ACDC_SEL_CH6, 1);
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_ACDC_SEL_CH7, 1);
    }
    else {
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_ACDC_SEL_CH4, 0);
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_ACDC_SEL_CH5, 0);
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_ACDC_SEL_CH6, 0);
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_ACDC_SEL_CH7, 0);
    }
}


static U8 pin_map[LEAD_MAX]={AD8233_GPIO_EXT_PIN_ACDC_SEL_CH1, AD8233_GPIO_EXT_PIN_ACDC_SEL_CH2, AD8233_GPIO_EXT_PIN_ACDC_SEL_CH3, AD8233_GPIO_EXT_PIN_ACDC_SEL_CH4,
                             AD8233_GPIO_EXT_PIN_ACDC_SEL_CH5, AD8233_GPIO_EXT_PIN_ACDC_SEL_CH6, AD8233_GPIO_EXT_PIN_ACDC_SEL_CH7, AD8233_GPIO_EXT_PIN_MAX};
static void set_lead_current(U8 lead, U8 current)
{
    U8 hl=(current==AC)?1:0;
    U8 p=pin_map[lead];    
    
    if(p<AD8233_GPIO_EXT_PIN_ACDC_SEL_CH1 || p>AD8233_GPIO_EXT_PIN_ACDC_SEL_CH7) {
        return;
    }
    
    gpio_ext_set_hl(p, hl);         //AD8233,   高交流，低直流
    
    
}

static void set_fast_recover(U8 v)
{
    U8 hl=v?1:0;
    gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_FR_CH_SEL, hl);
}


static int ad8233_send_cmd(bio_cmd_t *cmd)
{
    if(!cmd) {
        return -1;
    }
    
    switch(cmd->id) {
        
        case CMD_SEL_RLD_SRC:
        select_rld(cmd->para);
        break;
        
        case CMD_SEL_DIY_SRC:
        select_rld_diy(cmd->para);
        break;
        
        case CMD_SEL_WCT_SRC:
        select_wct(cmd->para);
        break;
        
        case CMD_SET_LOFF_CURRENT:
        ad8233_sett.current = cmd->para;
        set_loff_current(cmd->para);
        break;
        
        case CMD_SET_FAST_RECOVER:
        set_fast_recover(cmd->para);
        break;
        
        case CMD_SET_LOFF_PULLUP:
        set_pullup(cmd->para);
        break;
        
        default:
        return -1;
    }
    
    return 0;
}
/////////////electrode switch functions end//////////////


bio_handle_t ad8233_handle={
    ad8233_init,
    ad8233_reset,
    ad8233_get_stat,
    ad8233_send_cmd,
    ad8233_start,
    ad8233_stop,
    ad8233_get_info,
    ad8233_set_current,
    ad8233_get_setting,
    ad8233_set_callback,
    
    ad8233_read_ecg,
    ad8233_read_loff,
    ad8233_get_rld,
    ad8233_set_rld,
    ad8233_get_wct,
    ad8233_set_wct,
};




