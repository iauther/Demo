#include <math.h>
#include "log.h"
#include "drv/pwm.h"

#if defined(__CC_ARM)
#pragma diag_suppress 1296
#endif


extern U32 sys_freq;

enum {
    TIMER_1=0,
    TIMER_2,
    TIMER_3,
    TIMER_4,

    TIMER_MAX
};

typedef struct {
    U8                  idx;
    gpio_pin_t          pin;
    U8                  chn;
    
    TIM_TypeDef         *TIM;
    IRQn_Type           IRQn;
    U8                  Alternate;
}pwm_info_t;
typedef struct {
    
}pwm_time_t;

typedef struct {
    TIM_HandleTypeDef   htim;
    U32                 period;
    pwm_info_t          *info;
    pwm_cfg_t           cfg;
    
    U64                 cnt;
    U32                 rise;
    U32                 fail;
}pwm_handle_t;

pwm_handle_t *pwm_handle[TIMER_MAX]={NULL};


pwm_info_t PWM_INFO[PWM_PIN_MAX]={
    {TIMER_1, {GPIOA, GPIO_PIN_0 }, TIM_CHANNEL_1, TIM2, TIM2_IRQn, GPIO_AF1_TIM2},
    {TIMER_1, {GPIOA, GPIO_PIN_1 }, TIM_CHANNEL_2, TIM2, TIM2_IRQn, GPIO_AF1_TIM2},
    {TIMER_1, {GPIOA, GPIO_PIN_2 }, TIM_CHANNEL_3, TIM2, TIM2_IRQn, GPIO_AF1_TIM2},
    {TIMER_1, {GPIOA, GPIO_PIN_3 }, TIM_CHANNEL_4, TIM2, TIM2_IRQn, GPIO_AF1_TIM2},
                                                                                 
    {TIMER_2, {GPIOA, GPIO_PIN_6 }, TIM_CHANNEL_1, TIM3, TIM3_IRQn, GPIO_AF2_TIM3},
    {TIMER_2, {GPIOA, GPIO_PIN_7 }, TIM_CHANNEL_2, TIM3, TIM3_IRQn, GPIO_AF2_TIM3},
    {TIMER_2, {GPIOB, GPIO_PIN_0 }, TIM_CHANNEL_3, TIM3, TIM3_IRQn, GPIO_AF2_TIM3},
    {TIMER_2, {GPIOB, GPIO_PIN_1 }, TIM_CHANNEL_4, TIM3, TIM3_IRQn, GPIO_AF2_TIM3},
                                                                                 
    {TIMER_3, {GPIOD, GPIO_PIN_12}, TIM_CHANNEL_1, TIM4, TIM4_IRQn, GPIO_AF2_TIM4},
    {TIMER_3, {GPIOD, GPIO_PIN_13}, TIM_CHANNEL_2, TIM4, TIM4_IRQn, GPIO_AF2_TIM4},
    {TIMER_3, {GPIOD, GPIO_PIN_14}, TIM_CHANNEL_3, TIM4, TIM4_IRQn, GPIO_AF2_TIM4},
    {TIMER_3, {GPIOD, GPIO_PIN_15}, TIM_CHANNEL_4, TIM4, TIM4_IRQn, GPIO_AF2_TIM4},
                                                                                 
    {TIMER_4, {GPIOF, GPIO_PIN_3 }, TIM_CHANNEL_1, TIM5, TIM5_IRQn, GPIO_AF2_TIM5},
    {TIMER_4, {GPIOF, GPIO_PIN_4 }, TIM_CHANNEL_2, TIM5, TIM5_IRQn, GPIO_AF2_TIM5},
    {TIMER_4, {GPIOF, GPIO_PIN_5 }, TIM_CHANNEL_3, TIM5, TIM5_IRQn, GPIO_AF2_TIM5},
    {TIMER_4, {GPIOF, GPIO_PIN_10}, TIM_CHANNEL_4, TIM5, TIM5_IRQn, GPIO_AF2_TIM5},
                                                                        
};



//////////////////////////////以下为pwm捕获接口/////////////////////////////
static int get_index(TIM_TypeDef *tim)
{
    switch((U32)tim) {
        case (U32)TIM2:
        return TIMER_1;
        
        case (U32)TIM3:
        return TIMER_2;
        
        case (U32)TIM4:
        return TIMER_3;
        
        case (U32)TIM5:
        return TIMER_4;
    }
    
    return -1;
}
static U8 get_pwm_type(pwm_handle_t *h, U8 pwm_pin)
{
    U8 i;
    
    for(i=0; i<PCH_MAX; i++) {
        if(h->cfg.para.pch[i].pwm_pin==pwm_pin) {
            return h->cfg.para.pch[i].type;
        }
    }
    
    return TYPE_NO;
}

static pwm_info_t *get_info(pwm_ch_t *pch)
{
    for(int i=0; i<PCH_MAX; i++) {
        if(pch[i].type!=TYPE_NO) {
            return &PWM_INFO[pch[i].pwm_pin];
        }
    }

    return NULL;
}

static pwm_handle_t *get_handle(TIM_HandleTypeDef *htim)
{
    int idx=get_index(htim->Instance);
    
    if(idx==-1) {
        return NULL;
    }
    
    return pwm_handle[idx];
}

static int irq_clk_set(pwm_handle_t *h, U8 on)
{   
    switch((U32)h->info->TIM) {
        case (U32)TIM2:
        if(on) {
            __HAL_RCC_TIM2_CLK_ENABLE();
        }
        else {
            __HAL_RCC_TIM2_CLK_DISABLE();
        }
        break;
        
        case (U32)TIM3:
        if(on) {
            __HAL_RCC_TIM3_CLK_ENABLE();
        }
        else {
            __HAL_RCC_TIM3_CLK_DISABLE();
        }
        break;
        
        case (U32)TIM4:
        if(on) {
            __HAL_RCC_TIM4_CLK_ENABLE();
        }
        else {
            __HAL_RCC_TIM4_CLK_DISABLE();
        }
        break;
        
        case (U32)TIM5:
        if(on) {
            __HAL_RCC_TIM5_CLK_ENABLE();
        }
        else {
            __HAL_RCC_TIM5_CLK_DISABLE();
        }
        break;
        
        default:
        return -1;
    }
    
    if(on) {
        HAL_NVIC_SetPriority(h->info->IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(h->info->IRQn);
    }
    else {
        HAL_NVIC_DisableIRQ(h->info->IRQn);
    }
    
    return 0;
}
static U8 get_pwm_pin(TIM_TypeDef *tim, U8 ch)
{
    
    for(U8 i=0; i<PWM_PIN_MAX; i++) {
        if(PWM_INFO[i].chn==ch && PWM_INFO[i].TIM==tim) {
            return i;
        }
    }
    
    return 0;
}
static int get_tim_para(TIM_HandleTypeDef *htim, U8 *chn, U8 *pwm_pin)
{
    U32 ch=0;
    
    switch((U32)htim->Channel) {
        case HAL_TIM_ACTIVE_CHANNEL_1:
        ch = TIM_CHANNEL_1;
        break;
        
        case HAL_TIM_ACTIVE_CHANNEL_2:
        ch = TIM_CHANNEL_2;
        break;
        
        case HAL_TIM_ACTIVE_CHANNEL_3:
        ch = TIM_CHANNEL_3;
        break;
        
        case HAL_TIM_ACTIVE_CHANNEL_4:
        ch = TIM_CHANNEL_4;
        break;
        
        default:
        return -1;
    }
    
    *chn = ch;
    *pwm_pin = get_pwm_pin(htim->Instance, ch);
    
    return 0;
}

////////////////////////////////////////////////////////////
static void pin_init(pwm_ch_t *pch, U8 on)
{
    pwm_info_t *info;
    GPIO_InitTypeDef init;

    info = &PWM_INFO[pch->pwm_pin];
    if(on) {
        gpio_clk_en(&info->pin, on);
        
        init.Pin   = info->pin.pin;
        init.Mode  = (pch->mode==MODE_OD)?GPIO_MODE_AF_OD:GPIO_MODE_AF_PP;
        init.Pull  = GPIO_NOPULL;
        init.Speed = GPIO_SPEED_FREQ_LOW;
        init.Alternate = info->Alternate;
        HAL_GPIO_Init(info->pin.grp, &init); 
    }
    else {
        HAL_GPIO_DeInit(info->pin.grp, info->pin.pin);
    }
}
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* htim)
{
    pwm_handle_t *h=get_handle(htim);
    
    irq_clk_set(h, 1);
}

void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* htim)
{
    pwm_handle_t *h=get_handle(htim);
    
    irq_clk_set(h, 0);
}

#define GET_POLARITY(__HANDLE__, __CHANNEL__) \
  (((__CHANNEL__) == TIM_CHANNEL_1) ? (((__HANDLE__)->Instance->CCER>>(1))   & 0x101):\
   ((__CHANNEL__) == TIM_CHANNEL_2) ? (((__HANDLE__)->Instance->CCER>>(1+4)) & 0x11) :\
   ((__CHANNEL__) == TIM_CHANNEL_3) ? (((__HANDLE__)->Instance->CCER>>(1+8)) & 0x11) :\
   (((__HANDLE__)->Instance->CCER>>(1+12)) & 0x101))

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)     // 当计数溢出s时更新中断回调函数
{

}
void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim)   // 输出比较中断回调函数
{

}


void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)        // 输入捕获中断回调函数
{
    int r;
    U8 ch,pwm_pin;
    U32 v,polarity,al;
    U32 period,htime,ltime;
    pwm_handle_t *h=get_handle(htim);
    
    al = __HAL_TIM_GET_AUTORELOAD(htim);
    r = get_tim_para(htim, &ch, &pwm_pin);
    v = HAL_TIM_ReadCapturedValue(htim, ch);
    polarity = GET_POLARITY(htim, ch);
    
    if(h->cnt>=3) {
        if(polarity==TIM_ICPOLARITY_RISING) {
            period = (v<h->rise)?(al-h->rise+v):(v-h->rise);
            ltime  = (v<h->fail)?(al-h->fail+v):(v-h->fail);
            htime  = period - ltime;
        }
        else {
            period = (v<h->fail)?(al-h->fail+v):(v-h->fail);
            htime  = (v<h->rise)?(al-h->rise+v):(v-h->rise);
        }
        
        if(h->cfg.callback) {
            F32 dr = (htime*1.0F)/period;
            U32 freq = sys_freq/period;
            h->cfg.callback(h, pwm_pin, freq, dr);
        }
    }
    
    if(polarity==TIM_ICPOLARITY_RISING) {
        h->rise = v;
    }
    else {
        h->fail = v;
    }
    
    if(polarity==TIM_ICPOLARITY_RISING) {    //获得高上升沿时刻
        __HAL_TIM_SET_CAPTUREPOLARITY(htim, ch, TIM_ICPOLARITY_FALLING);
    }
    else {                                  //获得下降沿时刻
        __HAL_TIM_SET_CAPTUREPOLARITY(htim, ch, TIM_ICPOLARITY_RISING);
    }
    //HAL_TIM_IC_Start_IT(htim, ch);
    
    h->cnt++;
}
void HAL_TIM_TriggerCallback(TIM_HandleTypeDef *htim)           // 触发中断回调函数
{
 
}
void TIM2_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&pwm_handle[TIMER_1]->htim);
}
void TIM3_IRQHandler(void){HAL_TIM_IRQHandler(&pwm_handle[TIMER_2]->htim);}
void TIM4_IRQHandler(void){HAL_TIM_IRQHandler(&pwm_handle[TIMER_3]->htim);}
void TIM5_IRQHandler(void){HAL_TIM_IRQHandler(&pwm_handle[TIMER_4]->htim);}
#define PERIOD_OF(freq)             ((sys_freq)/(freq)-1)
#define PULSE_OF(period, dr)        ((U32)((period)*(dr)))

static int base_init(pwm_handle_t *h)
{
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    
    h->period = PERIOD_OF(h->cfg.para.freq);
    h->htim.Instance = h->info->TIM;
    h->htim.Init.Prescaler = 0;
    h->htim.Init.CounterMode = TIM_COUNTERMODE_UP;
    h->htim.Init.Period = h->period;
    h->htim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    h->htim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_Base_Init(&h->htim) != HAL_OK) {
        return -1;
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&h->htim, &sClockSourceConfig) != HAL_OK) {
        return -1;
    }
    
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&h->htim, &sMasterConfig) != HAL_OK) {
        return -1;
    }
    
    return 0;
}
    
    

static int oc_ic_init(pwm_handle_t *h)
{
    int i, ic_flag=0,oc_flag=0;
    pwm_ch_t *pch=h->cfg.para.pch;
    TIM_IC_InitTypeDef ic = {0};
    TIM_OC_InitTypeDef oc = {0};
    pwm_info_t *info;
    
    for(i=0; i<PCH_MAX; i++) {
        if(pch[i].type!=TYPE_NO) {
            
            info = &PWM_INFO[pch[i].pwm_pin];
            switch(pch[i].type) {
                case TYPE_IC:
                {
                    if(ic_flag==0) {
                        if (HAL_TIM_IC_Init(&h->htim) != HAL_OK) {
                            return -1;
                        }
                        ic_flag = 1;
                    }
                    
                    ic.ICPolarity = TIM_ICPOLARITY_RISING;
                    ic.ICSelection = TIM_ICSELECTION_DIRECTTI;
                    ic.ICPrescaler = TIM_ICPSC_DIV1;
                    ic.ICFilter = 0;
                    if (HAL_TIM_IC_ConfigChannel(&h->htim, &ic, info->chn) != HAL_OK) {
                        return -1;
                    }
                    
                    if(HAL_TIM_IC_Start_IT(&h->htim, info->chn) != HAL_OK) {
                        return -1;
                    }
                    
                    pin_init(&pch[i], 1);
                }
                break;
                
                case TYPE_OC:
                {
                    if(oc_flag==0) {
                        if (HAL_TIM_PWM_Init(&h->htim) != HAL_OK){
                            return -1;
                        }
                        oc_flag = 1;
                    }
                    
                    oc.OCMode = TIM_OCMODE_PWM1;
                    oc.Pulse  = PULSE_OF(h->period,h->cfg.para.dr);
                    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
                    oc.OCFastMode = TIM_OCFAST_DISABLE;
                    if (HAL_TIM_PWM_ConfigChannel(&h->htim, &oc, info->chn) != HAL_OK) {
                        return -1;
                    }
                    HAL_TIM_PWM_Start(&h->htim, info->chn);
                    
                    pin_init(&pch[i], 1);
                }
                
                default:
                break;
            }
        }
    }
    
	return 0;
}


static void set_freq_dr(pwm_handle_t *h, U8 channel, F32 freq, F32 dr)
{
    h->period = PERIOD_OF(freq);
    
    __HAL_TIM_SET_AUTORELOAD(&h->htim, h->period);
    __HAL_TIM_SET_COMPARE(&h->htim, channel, PULSE_OF(h->period, dr));
}


//////////////////////////////////////////////////////////////////////////


handle_t pwm_init(pwm_cfg_t *cfg)
{
    int r;
    pwm_handle_t *h;
    pwm_info_t *info;
    
    if(!cfg) {
        return NULL;
    }
    
    info = get_info(cfg->para.pch);
    if(!info) {
        LOGE("____ pwm para is invalid!\n");
        return NULL;
    }
    
    h = calloc(1, sizeof(pwm_handle_t));
    if(!h) {
        return NULL;
    }
    pwm_handle[info->idx] = h;
    
    h->cfg  = *cfg;
    h->info = info;
    if(base_init(h)) {
        LOGE("____ pwm base_init failed!\n");
        goto failed;
    }
      
    r = oc_ic_init(h);
    if(r) {
        goto failed;
    }
    
    return h;
    
failed:
    free(h);
    return NULL;
}


int pwm_start(handle_t h, U8 pwm_pin)
{
    U8 type;
    pwm_info_t *info;
    pwm_handle_t *ph=(pwm_handle_t*)h;
    
    if(!ph || pwm_pin>=PWM_PIN_MAX) {
        return -1;
    }
    
    info = &PWM_INFO[pwm_pin];
    type = get_pwm_type(h, pwm_pin);
    if(type==TYPE_OC) {
        HAL_TIM_PWM_Start(&ph->htim, info->chn);
    }
    else if(type==TYPE_IC) {
        HAL_TIM_IC_Start_IT(&ph->htim, info->chn);
    }
    else {
        return -1;
    }
    
    return 0;
}


int pwm_stop(handle_t h, U8 pwm_pin)
{
    U8 type;
    pwm_info_t *info;
    pwm_handle_t *ph=(pwm_handle_t*)h;
    
    if(!ph || pwm_pin>=PWM_PIN_MAX) {
        return -1;
    }
    
    info = &PWM_INFO[pwm_pin];
    type = get_pwm_type(h, pwm_pin);
    if(type==TYPE_OC) {
        HAL_TIM_PWM_Stop(&ph->htim, info->chn);
    }
    else if(type==TYPE_IC) {
        HAL_TIM_IC_Stop_IT(&ph->htim, info->chn);
    }
    else {
        return -1;
    }
    
    return 0;
}

int pwm_set(handle_t h, U8 pwm_pin, U32 freq, F32 dr)
{
    U8 type;
    pwm_info_t *info;
    pwm_handle_t *ph=(pwm_handle_t*)h;
    
    if(!ph || pwm_pin>=PWM_PIN_MAX) {
        return -1;
    }
    
    info = &PWM_INFO[pwm_pin];
    type = get_pwm_type(h, pwm_pin);
    if(type==TYPE_OC) {
        HAL_TIM_PWM_Stop(&ph->htim, info->chn);
        set_freq_dr(h, pwm_pin, freq, dr);
        HAL_TIM_PWM_Start(&ph->htim, info->chn);
    }
    
    return 0;
}





















