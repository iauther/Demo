#include "bio/ad8233.h"
#include "bio/ads8866.h"
#include "tca6424a.h"
#include "drv/gpio.h"
#include "drv/timer.h"
#include "log.h"
#include "drv/delay.h"
#include "bio/bio.h"
#include "cfg.h"


//#define USE_TIMER



#define LOFF_INT_PIN        HAL_GPIO_24

bio_stat_t bio_stat={
    LOFF_CURRENT_DC,
    LEAD_MAX
};

static int ad8233_read_ecg(ecg_data_t *ecg);


static bio_setting_t ad8233_sett={
    AC,
    
    {//wct_set_t
        {1, 1, 1, 0, 0, 0, 0, 0},       //
    },
    
    {//rld_set_t
        {1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       //in
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       //out
    },
};

static U8 ad8233_lead_index[LEAD_MAX]={
    7,        //LEAD_I
    6,        //LEAD_II
    5,        //LEAD_III
    4,        //LEAD_V1
    3,        //LEAD_V2
    2,        //LEAD_V3
    1,        //LEAD_V4
              
    0,        //LEAD_RLD
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
        adc = SWAP16(data[ad8233_lead_index[i]]);
        ecg->adc[i] = adc;
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
    //LOGD("_____ loff callback\r\n");
    gpio_int_en(LOFF_INT_PIN, 0);
    if(ad8233_callback.loffCallback) {
        ad8233_callback.loffCallback(user_data);
    }
    gpio_int_en(LOFF_INT_PIN, 1);
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
    
    ads8866_read((U8*)ad8233_buffer, sizeof(ad8233_buffer));
    if(ecg) {
        ecg->bits = 16;
        data_convert(ad8233_buffer, ecg);
    }
    
    return 0;
}



static void loff_lead_to_etrode(U8 *lead, loff_data_t *loff)
{
    if(lead[LEAD_I] && lead[LEAD_II]) {
        loff->etrode[ET_RA] = 1;
        if(lead[LEAD_III]) {
            loff->etrode[ET_LA] = 1;
            loff->etrode[ET_LL] = 1;
        }
    }
    else if(lead[LEAD_I] && lead[LEAD_III]) {
        loff->etrode[ET_LA] = 1;
        if(lead[LEAD_II]) {
            loff->etrode[ET_RA] = 1;
            loff->etrode[ET_LL] = 1;
        }
    }
    else if(lead[LEAD_II] && lead[LEAD_III]) {
        loff->etrode[ET_LL] = 1;
        if(lead[LEAD_I]) {
            loff->etrode[ET_LA] = 1;
            loff->etrode[ET_RA] = 1;
        }
    }
    
    loff->etrode[ET_RLD] = lead[LEAD_RLD];
    loff->etrode[ET_V1]  = lead[LEAD_V1];
    loff->etrode[ET_V2]  = lead[LEAD_V2];
    loff->etrode[ET_V3]  = lead[LEAD_V3];
    loff->etrode[ET_V4]  = lead[LEAD_V4];
}

static int ad8233_read_loff(loff_data_t *loff)
{
    int r=-1;
    F64 vol;
    U16 dat;
    U8  lead[LEAD_MAX];
    U8 i,tmp,rld_off=0;
    
    if(!loff) {
        return -1;
    }
    
    memset(loff, 0, sizeof(loff_data_t));
    if(bio_stat.current==DC) {
        int r1 = ads8866_read((U8*)&dat, 2);
        if(r1==0) {
            vol = SWAP16(dat)*LSB;
            //LOGD("______adc: 0x%04x vol: %0.2f V\r\n", SWAP16(dat), vol);
            if(vol<0.1) {
                rld_off = 1;
            }
        }
        
        r = tca6424a_read(TCA6424A_ADDR_L, TCA6424A_INPUT_REG0+2, &tmp, 1);
        if(r==0) {
            for(i=0; i<LEAD_MAX; i++) {
                lead[i] = (tmp&(1<<i))?1:0;
                 if(rld_off) {
                    lead[i] = 1;
                }
            }
            
            loff_lead_to_etrode(lead, loff);
            //LOGD("__vol: %0.2f__ lead  : %d, %d, %d, %d, %d, %d, %d, %d\r\n", vol, lead[0], lead[1], lead[2], lead[3], lead[4], lead[5], lead[6], lead[7]);
            //LOGD("__vol: %0.2f__ etrode: %d, %d, %d, %d, %d, %d, %d, %d\r\n", vol, loff->etrode[0], loff->etrode[1], loff->etrode[2], loff->etrode[3], loff->etrode[4], loff->etrode[5], loff->etrode[6], loff->etrode[7]);
        }
        
        //tca6424a_print_reg("", TCA6424A_ADDR_L);
    }
    else if(bio_stat.current==AC && bio_stat.ac_lead<LEAD_MAX) {
        r = tca6424a_read(TCA6424A_ADDR_L, TCA6424A_INPUT_REG0+2, &tmp, 1);
        if(r==0) {
            lead[bio_stat.ac_lead] = (tmp&(1<<bio_stat.ac_lead))?1:0;
        }
    }
    else {
        return -1;
    }

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
    timer_set_t set;
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
    
    timer_set(ad8233_tmr_handle, &set);
#endif
    
    return 0;
}

static int ad8233_get_stat(bio_stat_t *stat)
{
    int r;
    U16 vol;
    
    if(!stat) {
        return -1;
    }
    
    stat->current = bio_stat.current;
    stat->ac_lead = bio_stat.ac_lead;
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

typedef enum {
    DIY_FB_I=0,
    DIY_FB_II,
    DIY_FB_III,
    DIY_WCT,
}DIY;
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


typedef enum {
    LA,
    RA,
    LL,
}WCT;
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

static void select_messure_lead(U8 fun)
{

    switch(fun) {
        case MESSURE_LEAD_NORMAL:
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_LOFPU_SEL, 1);
        break;
        
        case MESSURE_LEAD_RLD:
        messure_rld(1);
        break;
        
        case MESSURE_LEAD_NONE:
        messure_rld(0);
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_LOFPU_SEL, 0);
        break;
    }
}


static void set_tmux(U8 fun)
{
    if(fun==LOFF_CURRENT_AC) {
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_LEAD_DC_SEL,  0);              //TMUX1136, 高直流，低交流
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_LEAD_AC_SEL,  0);
    }
    else {
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_LEAD_DC_SEL,  1);              //TMUX1136, 高直流，低交流
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_LEAD_AC_SEL,  1);
    }
}
static void set_loff_current(U8 fun)
{
    U8 hl=0;
    
    gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_ACDC_SEL_CH1, hl);             //AD8233,   高交流，低直流
    gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_ACDC_SEL_CH2, hl);             //AD8233,   高交流，低直流
    gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_ACDC_SEL_CH3, hl);
    gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_ACDC_SEL_CH4, hl);
    gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_ACDC_SEL_CH5, hl);
    gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_ACDC_SEL_CH6, hl);
    gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_ACDC_SEL_CH7, hl);
    
    set_tmux(fun);
    if(fun==LOFF_CURRENT_AC) {
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_LEAD_DC_SEL,  0);              //TMUX1136, 高直流，低交流
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_LEAD_AC_SEL,  0);
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_LOFPU_SEL, 0);
    }
    else {
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_LEAD_DC_SEL,  1);              //TMUX1136, 高直流，低交流
        gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_LEAD_AC_SEL,  1);
        
        if(fun==LOFF_CURRENT_DC_PULLUP) {
            gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_LOFPU_SEL, 1);             //直流检测脱落时才开启上拉
            //printf("____ FUN_LOFF_DC_PULLUP\r\n");
        }
        else {
            gpio_ext_set_hl(AD8233_GPIO_EXT_PIN_LOFPU_SEL, 0);
            //printf("____ FUN_LOFF_DC\r\n");
        }
    }
}


static U8 pin_map[LEAD_MAX]={AD8233_GPIO_EXT_PIN_ACDC_SEL_CH1, AD8233_GPIO_EXT_PIN_ACDC_SEL_CH2, AD8233_GPIO_EXT_PIN_ACDC_SEL_CH3, AD8233_GPIO_EXT_PIN_ACDC_SEL_CH4,
                             AD8233_GPIO_EXT_PIN_ACDC_SEL_CH5, AD8233_GPIO_EXT_PIN_ACDC_SEL_CH6, AD8233_GPIO_EXT_PIN_ACDC_SEL_CH7, AD8233_GPIO_EXT_PIN_MAX};
static void select_ac_loff_lead(U8 lead)
{
    U8 hl=0;
    U8 p1=pin_map[bio_stat.ac_lead];
    U8 p2=pin_map[lead];
    
    if(bio_stat.ac_lead>=LEAD_MAX) {
        return;
    }
    
    if(p1>=AD8233_GPIO_EXT_PIN_ACDC_SEL_CH1 && p1<=AD8233_GPIO_EXT_PIN_ACDC_SEL_CH7 ) {
        gpio_ext_set_hl(p1, 0);         //AD8233,   高交流，低直流
    }
    
    if(p2>=AD8233_GPIO_EXT_PIN_ACDC_SEL_CH1 && p2<=AD8233_GPIO_EXT_PIN_ACDC_SEL_CH7 ) {
        gpio_ext_set_hl(p2, 1);         //AD8233,   高交流，低直流
    }
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
        bio_stat.current = cmd->para;
        set_loff_current(cmd->para);
        break;
        
        case CMD_SEL_LOFF_AC_LEAD:
        bio_stat.ac_lead = cmd->para;
        select_ac_loff_lead(cmd->para);
        break;
        
        case CMD_MESA_LEAD:
        select_messure_lead(cmd->para);
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




