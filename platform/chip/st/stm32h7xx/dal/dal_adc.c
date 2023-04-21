#include "dal_adc.h"
#include "cfg.h"



//#define ADC_TRIGGER_FROM_TIMER


//http://t.zoukankan.com/armfly-p-12180331.html
//http://www.bepat.de/tag/adc-dual-regular-simultaneous-mode/


/*
    A0:    {GPIOA, GPIO_PIN_0}   ADC1_P_16  
    A1:    {GPIOA, GPIO_PIN_1}   ADC1_N_16  ADC1_P_17             
    A2:    {GPIOA, GPIO_PIN_2}   ADC1_P_14  ADC2_P_14             
    A3:    {GPIOA, GPIO_PIN_3}   ADC1_P_15  ADC2_P_15             
    A4:    {GPIOA, GPIO_PIN_4}   ADC1_P_18  ADC2_P_18             
    A5:    {GPIOA, GPIO_PIN_5}   ADC1_N_18  ADC1_P_19  ADC2_N_18  ADC2_P_19
    A6:    {GPIOA, GPIO_PIN_6}   ADC1_P_3   ADC2_P_3              
    A7:    {GPIOA, GPIO_PIN_7}   ADC1_N_3   ADC1_P_7   ADC2_N_3   ADC2_P_7
                                 
    B0:    {GPIOB, GPIO_PIN_0}   ADC1_N_5   ADC1_P_9   ADC2_N_5   ADC2_P_9
    B1:    {GPIOB, GPIO_PIN_1}   ADC1_P_5   ADC2_P_5              
                                 
    C0:    {GPIOC, GPIO_PIN_0}   ADC1_P_10  ADC2_P_10  ADC3_P_10  
    C1:    {GPIOC, GPIO_PIN_1}   ADC1_N_10  ADC1_P_11  ADC2_N_10  ADC2_P_11   ADC3_N_10  ADC3_P_11
    C2:    {GPIOC, GPIO_PIN_2}   ADC3_N_1   ADC3_P_0              
    C3:    {GPIOC, GPIO_PIN_3}   ADC3_P_1                         
    C4:    {GPIOC, GPIO_PIN_4}   ADC1_P_4   ADC2_P_4              
    C5:    {GPIOC, GPIO_PIN_5}   ADC1_N_4   ADC1_P_8   ADC2_N_4   ADC2_P_8
                                 
    F3:    {GPIOF, GPIO_PIN_3}   ADC3_P_5                         
    F4:    {GPIOF, GPIO_PIN_4}   ADC3_N_5   ADC3_P_9              
    F5:    {GPIOF, GPIO_PIN_5}   ADC3_P_4                         
    F6:    {GPIOF, GPIO_PIN_6}   ADC3_N_4   ADC3_P_8              
    F7:    {GPIOF, GPIO_PIN_7}   ADC3_P_3                         
    F8:    {GPIOF, GPIO_PIN_8}   ADC3_N_3   ADC3_P_7              
    F9:    {GPIOF, GPIO_PIN_9}   ADC3_P_2                         
    F10:   {GPIOF, GPIO_PIN_10}  ADC3_N_2   ADC3_P_6              
    F11:   {GPIOF, GPIO_PIN_11}  ADC1_P_2                         
    F12:   {GPIOF, GPIO_PIN_12}  ADC1_N_2   ADC1_P_6              
    F13:   {GPIOF, GPIO_PIN_13}  ADC2_P_2                         
    F14:   {GPIOF, GPIO_PIN_14}  ADC2_N_2   ADC2_P_6              
                     
    H2:    {GPIOH, GPIO_PIN_2}   ADC3_P_13                        
    H3:    {GPIOH, GPIO_PIN_3}   ADC3_N_13  ADC3_P_14             
    H4:    {GPIOH, GPIO_PIN_4}   ADC3_N_14  ADC3_P_15             
    H5:    {GPIOH, GPIO_PIN_5}   ADC3_N_15  ADC3_P_16
    
    ADC
    
*/
extern U32 sys_freq;

//I don't know why change the adcPins size will affect the lwip net: ping will do not work
#if (ADC_DUAL_MODE==1)
static dal_adc_pin_t adcPins[]={
                                {ADC1, {GPIOA, GPIO_PIN_4}, ADC_CHANNEL_18, ADC_RESOLUTION_16B, (500*1000), ADC_REGULAR_RANK_1, ADC_DIFFERENTIAL_ENDED},
                                {ADC1, {GPIOA, GPIO_PIN_5}, ADC_CHANNEL_18, ADC_RESOLUTION_16B, (500*1000), ADC_REGULAR_RANK_1, ADC_DIFFERENTIAL_ENDED},
                                {ADC2, {GPIOB, GPIO_PIN_0}, ADC_CHANNEL_5,  ADC_RESOLUTION_16B, (500*1000), ADC_REGULAR_RANK_2, ADC_DIFFERENTIAL_ENDED},
                                {ADC2, {GPIOB, GPIO_PIN_1}, ADC_CHANNEL_5,  ADC_RESOLUTION_16B, (500*1000), ADC_REGULAR_RANK_2, ADC_DIFFERENTIAL_ENDED},
                                {NULL, {0, 0},              0,              0,                   0,         0,                  0,                    },
                           };
#else
static dal_adc_pin_t adcPins[]={
                                {ADC1, {GPIOA, GPIO_PIN_4}, ADC_CHANNEL_18, ADC_RESOLUTION_16B, (500*1000), ADC_REGULAR_RANK_1, ADC_DIFFERENTIAL_ENDED},
                                {ADC1, {GPIOA, GPIO_PIN_5}, ADC_CHANNEL_18, ADC_RESOLUTION_16B, (500*1000), ADC_REGULAR_RANK_1, ADC_DIFFERENTIAL_ENDED},
                                {ADC1, {GPIOB, GPIO_PIN_0}, ADC_CHANNEL_5,  ADC_RESOLUTION_16B, (500*1000), ADC_REGULAR_RANK_2, ADC_DIFFERENTIAL_ENDED},
                                {ADC1, {GPIOB, GPIO_PIN_1}, ADC_CHANNEL_5,  ADC_RESOLUTION_16B, (500*1000), ADC_REGULAR_RANK_2, ADC_DIFFERENTIAL_ENDED},
                                {NULL, {0, 0},              0,              0,                   0,         0,                  0,                    },
                           };
#endif
                           
                           

typedef struct {
    ADC_HandleTypeDef hadc;
    DMA_HandleTypeDef hdma;
}adc_t;
typedef struct {
    U16     adc;
    F32     vol;
}ch_data_t;

typedef struct {
    U8                  dual;
    ADC_HandleTypeDef   master;
    ADC_HandleTypeDef   slave;
    
    DMA_HandleTypeDef   hdma;
    TIM_HandleTypeDef   htim;
    
    U32                 *pBuf;
    U32                 *pAlnBuf;
    U32                 bufLen;
    
    U8                  mode;
    adc_callback_t      callback;
    U32                 value[16];
    U8                  cnt;
    U8                  chns;
}adc_handle_t;


static adc_handle_t *adcHandle=NULL;


#if 0

///////////////timer function start/////////////////////////
//TIM2~7,12~14     TIM1,8,15~17 equal sys_freq/2
void TIM4_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&adcHandle->htim);
}
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* htim)
{
    if(htim->Instance==TIM4){
        __HAL_RCC_TIM4_CLK_ENABLE();

        HAL_NVIC_SetPriority(TIM4_IRQn, 8, 0);
        HAL_NVIC_EnableIRQ(TIM4_IRQn);
    }
}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    
}
static int tim_init(U32 freq)
{
    TIM_HandleTypeDef *htim=&adcHandle->htim;
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    U32 prescaler,period,total=(sys_freq>>1)/freq;

    //(Prescaler + 1)*(Period + 1) = TimeClockFreq/freq;
    prescaler = (total>0xffff)?total/0xffff:0;
    period = (total/(prescaler+1))-1;
    
    htim->Instance = TIM4;
    htim->Init.Prescaler = prescaler;
    htim->Init.CounterMode = TIM_COUNTERMODE_UP;
    htim->Init.Period = period;
    htim->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_Base_Init(htim) != HAL_OK) {
        return -1;
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(htim, &sClockSourceConfig) != HAL_OK) {
        return -1;
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(htim, &sMasterConfig) != HAL_OK) {
        return -1;
    }
    
    //__HAL_TIM_CLEAR_IT(htim, TIM_IT_UPDATE);
    //HAL_TIM_Base_Start_IT(htim);

    return 0;
}

static int tim_start(void)
{
    return HAL_TIM_Base_Start(&adcHandle->htim);
}
static int tim_stop(void)
{
    return HAL_TIM_Base_Stop(&adcHandle->htim);
}
///////////////timer function end/////////////////////////

static int get_index(U32 ch)
{
    switch(ch) {
        case ADC_CHANNEL_19: return 19;
        case ADC_CHANNEL_18: return 18;
        case ADC_CHANNEL_17: return 17;
        case ADC_CHANNEL_16: return 16;
        case ADC_CHANNEL_15: return 15;
        case ADC_CHANNEL_14: return 14;
        case ADC_CHANNEL_13: return 13;
        case ADC_CHANNEL_12: return 12;
        case ADC_CHANNEL_11: return 11;
        case ADC_CHANNEL_10: return 10;
        case ADC_CHANNEL_9 : return 9 ;
        case ADC_CHANNEL_8 : return 8 ;
        case ADC_CHANNEL_7 : return 7 ;
        case ADC_CHANNEL_6 : return 6 ;
        case ADC_CHANNEL_5 : return 5 ;
        case ADC_CHANNEL_4 : return 4 ;
        case ADC_CHANNEL_3 : return 3 ;
        case ADC_CHANNEL_2 : return 2 ;
        case ADC_CHANNEL_1 : return 1 ;
        case ADC_CHANNEL_0 : return 0 ; 
    }
    
    return -1;
}

static int get_channels(void)
{
    int i,cnt=0;
    int channel[20]={0};
    
    for(i=0; ; i++) {
        if(adcPins[i].pin.grp==0) {
            break;
        }
        
        channel[adcPins[i].ch]++;
    }
    
    for(i=0; i<20; i++) {
        if(channel[i]>0) {
            cnt++;
        }
    }
    
    return cnt;
}
static int _gpio_init(U8 bInit)
{
    int i;
    adc_pin_t *ap = adcPins;
    GPIO_InitTypeDef init = {0};
    
    init.Mode = GPIO_MODE_ANALOG;
    init.Pull = GPIO_NOPULL;
    for(i=0; ; i++) {
        if(ap[i].adc==NULL) {
            break;
        }
        
        init.Pin = ap[i].pin.pin;
        gpio_en_clk(ap[i].pin.grp, bInit);
        if(bInit) {
            HAL_GPIO_Init(ap[i].pin.grp, &init);
        }
        else {
            HAL_GPIO_DeInit(ap[i].pin.grp, ap[i].pin.pin);
        }
    }
    
    return 0;
}


void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc)
{
    adc_handle_t *h=adcHandle;
    
    if(hadc->Instance == ADC1) {
        __HAL_RCC_ADC12_CLK_ENABLE();
        
        if(h->mode==MODE_DMA) {
            h->hdma.Init.Direction = DMA_PERIPH_TO_MEMORY;
            h->hdma.Init.PeriphInc = DMA_PINC_DISABLE;
            h->hdma.Init.MemInc = DMA_MINC_ENABLE;
            h->hdma.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
            h->hdma.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
            h->hdma.Init.Mode = DMA_CIRCULAR;
            h->hdma.Init.Priority = DMA_PRIORITY_LOW;
            h->hdma.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
            h->hdma.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
            h->hdma.Init.MemBurst = DMA_MBURST_SINGLE;
            h->hdma.Init.PeriphBurst = DMA_PBURST_SINGLE;
            
            __HAL_RCC_DMA2_CLK_ENABLE();
            
            h->hdma.Instance = DMA2_Stream0;
            h->hdma.Init.Request = DMA_REQUEST_ADC1;
            HAL_DMA_DeInit(&h->hdma);
            
            HAL_DMA_Init(&h->hdma);
            __HAL_LINKDMA(hadc, DMA_Handle, h->hdma);
            
            HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 8, 0);
            HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
        }
        else if(h->mode==MODE_IT) {
            HAL_NVIC_SetPriority(ADC_IRQn, 8, 0);
            HAL_NVIC_EnableIRQ(ADC_IRQn);
        }
    }
}
void HAL_ADC_MspDeInit(ADC_HandleTypeDef *hadc)
{
    if(hadc->Instance == ADC1) {
        __HAL_RCC_ADC12_CLK_DISABLE();
           
    }
    else if(hadc->Instance == ADC2) {
        __HAL_RCC_ADC12_CLK_DISABLE();
        
    }
    else if(hadc->Instance == ADC3) {
        __HAL_RCC_BDMA_CLK_DISABLE();
        
    }
}


//master: lower 2 bytes   slave: higher 2 bytes
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
{
    adc_handle_t *h=adcHandle;
    U8* pBuf=(U8*)h->pAlnBuf;
  
    if(h->mode==MODE_DMA) {
        SCB_InvalidateDCache_by_Addr(pBuf, h->bufLen/2);
    }
    
    if(h->callback) {
        if(h->mode==MODE_DMA) {
            h->callback(pBuf, h->bufLen/2);
        }
    }
}
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    adc_handle_t *h=adcHandle;
    U8* pBuf=(U8*)h->pAlnBuf;

    if(h->mode==MODE_DMA) {
        SCB_InvalidateDCache_by_Addr(pBuf+h->bufLen/2, h->bufLen/2);
    }
    
    if(h->callback) {
        if(h->mode==MODE_DMA) {
           h->callback(pBuf+h->bufLen/2, h->bufLen/2);
        }
        else if(h->mode==MODE_IT) {
            if(h->cnt<h->chns) {
                h->value[h->cnt++] = HAL_ADC_GetValue(&h->master);
            }
            else {
                //do something...
            }
        }
    }
}


/*
{ADC1, DMA2_Stream0_IRQn,  DMA_REQUEST_ADC1,   DMA2_Stream0},
{ADC2, DMA2_Stream2_IRQn,  DMA_REQUEST_ADC2,   DMA2_Stream2}, 
{ADC3, BDMA_Channel0_IRQn, BDMA_REQUEST_ADC3,  MDMA_Channel0}
*/
void DMA2_Stream0_IRQHandler(void)  //ADC1
{
    HAL_DMA_IRQHandler(&adcHandle->hdma);
}

void ADC_IRQHandler(void)
{
    HAL_ADC_IRQHandler(&adcHandle->master);
}


//#define USE_ADC_DEINIT
#define IS_POWER_CH(mask,ch) (mask&(ch<<1))

static void io_switch(U32 chMask)
{
    int delay=0;
    GPIO_PinState hl;

#if 0
	if(IS_POWER_CH(chMask, )) {	//channel 1 on
        hl = GPIO_PIN_RESET;
	}
    else {	                                                //channel 2 on
        hl = GPIO_PIN_SET;
	}
    
    if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7) != hl) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, hl);
        delay = 1;
    }
       
    ////////////////////////////////////////////////////
	if(IS_ISO_CHN_BIT(chBits,DAU_CHNINDEC_ISO_CHN_03)) {	//channel 3 on
        hl = GPIO_PIN_RESET;
	}
    else {	                                                //channel 4 on
        hl = GPIO_PIN_SET;
	}
    
    if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6) != hl) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, hl);
        delay = 1;
    }
    
    if(delay>0) {
        delay_ms(1);
    }
#endif
}

///////////////timer function end/////////////////////////

int dal_adc_init(void)
{
    adc_handle_t *h=NULL;
    ADC_HandleTypeDef hadc;
    
    _gpio_init(1);
    
    h=calloc(1, sizeof(adc_handle_t));
    if(!h) {
        return -1;
    }
    
    h->master.Instance = ADC1; 
#ifdef ADC_TRIGGER_FROM_TIMER
    h->master.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV1;                //AHB clock
    h->master.Init.ExternalTrigConv = ADC_EXTERNALTRIG_T4_TRGO;
    h->master.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
    h->master.Init.ContinuousConvMode = DISABLE;
//#else
    h->master.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
    h->master.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    h->master.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIG_EDGE_NONE;
    h->master.Init.ContinuousConvMode = ENABLE;
#endif
    
#if 0
    h->master.Init.Resolution = ADC_RESOLUTION_16B;
    h->master.Init.ScanConvMode = ADC_SCAN_ENABLE;
    h->master.Init.EOCSelection = ADC_EOC_SINGLE_CONV;//ADC_EOC_SEQ_CONV;
    h->master.Init.LowPowerAutoWait = DISABLE;
    
    h->master.Init.NbrOfConversion = 1;//sizeof(adcPins)/sizeof(adc_pin_t)-1;
    h->master.Init.DiscontinuousConvMode = DISABLE;
    h->master.Init.NbrOfDiscConversion = 0;
    
    h->master.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DMA_CIRCULAR;
    h->master.Init.LeftBitShift = ADC_LEFTBITSHIFT_NONE;
    
    h->master.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;   //ADC_OVR_DATA_PRESERVED
    h->master.Init.OversamplingMode = ENABLE;
    
    h->master.Init.Oversampling.Ratio = 1;
    h->master.Init.Oversampling.RightBitShift  = ADC_RIGHTBITSHIFT_1;
    h->master.Init.Oversampling.TriggeredMode  = ADC_TRIGGEREDMODE_SINGLE_TRIGGER;
    h->master.Init.Oversampling.OversamplingStopReset = ADC_REGOVERSAMPLING_CONTINUED_MODE;
#endif
    
    
	h->master.Init.ClockPrescaler        = ADC_CLOCK_ASYNC_DIV1;          		/* 采用PLL异步时钟，1分频，Y版本             即20.48MHz */
	h->master.Init.Resolution            = ADC_RESOLUTION_16B;            		/* 16位分辨率 */
	h->master.Init.ScanConvMode          = ADC_SCAN_DISABLE;              		/* 禁止扫描*/
	h->master.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;           		/* EOC转换结束标志 */
	h->master.Init.LowPowerAutoWait      = DISABLE;                       		/* 禁止低功耗自动延迟特性 */
	h->master.Init.ContinuousConvMode    = ENABLE;                       		/* 使能连续转换 */
	h->master.Init.NbrOfConversion       = 1;                             		/* 使用1个转换通道 */
	h->master.Init.DiscontinuousConvMode = DISABLE;                       		/* 禁止不连续模式 */
	h->master.Init.NbrOfDiscConversion   = 1;                             		/* 禁止不连续模式后，此参数忽略，此位是用来配置不连续子组中通道数 */
	h->master.Init.ExternalTrigConv      = ADC_SOFTWARE_START;            		/* 软件触发 */
	h->master.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;    	/* 无触发 */
	h->master.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DMA_CIRCULAR; 	/* DMA循环模式接收ADC转换的数据 */
	h->master.Init.Overrun                  = ADC_OVR_DATA_OVERWRITTEN;       	/* ADC转换溢出的话，覆盖ADC的数据寄存器 */
	h->master.Init.OversamplingMode         = ENABLE;                	        /* Oversampling enabled */
    
    h->master.Init.Oversampling.Ratio	 = 1;
    h->master.Init.Oversampling.RightBitShift  = ADC_RIGHTBITSHIFT_1;
	
	h->master.Init.Oversampling.TriggeredMode = ADC_TRIGGEREDMODE_SINGLE_TRIGGER;         			/* Specifies whether or not a trigger is needed for each sample */
	h->master.Init.Oversampling.OversamplingStopReset = ADC_REGOVERSAMPLING_CONTINUED_MODE;
    
    
    //delay_ms(10);
    adcHandle = h;
    
    return 0;
}


int dal_adc_config(dal_adc_cfg_t *cfg)
{
    int i=0,idx,chan[20]={0};
    adc_handle_t *h=adcHandle;
    adc_pin_t *ap=adcPins;
    ADC_ChannelConfTypeDef sConfig = {0};
    ADC_MultiModeTypeDef mInit = {0};
    
    if(!h || !cfg) {
        return 0;
    }
    
    //h->master.Init.NbrOfConversion = 2;//sizeof(adcPins)/sizeof(adc_pin_t)-1;
    h->mode = cfg->mode;
    h->dual = cfg->dual;
    h->callback = cfg->callback;
    
    
    if (HAL_ADC_DeInit(&h->master) != HAL_OK) {
        return -1;
    }
    
    if (HAL_ADC_Init(&h->master) != HAL_OK) {
        return -1;
    }
    
    if(h->dual) {
        h->slave.Instance  = ADC2;
        h->slave.Init = h->master.Init;
        //h->slave.Init.ContinuousConvMode = DISABLE;
        //h->slave.Init.ExternalTrigConv = ADC_SOFTWARE_START;
#if 0
        if (HAL_ADC_DeInit(&h->slave) != HAL_OK) {
            return -1;
        }
#endif
        if (HAL_ADC_Init(&h->slave) != HAL_OK) {
            return -1;
        }
    }
    
#if 1
    //add channels
    sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLE_5;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0;
    sConfig.OffsetRightShift = DISABLE;
    sConfig.OffsetSignedSaturation = DISABLE;
    while(1) {
        if(ap[i].pin.grp==0 || i>=20) {
            break;
        }
        
        idx = get_index(ap[i].ch);
        if(idx<20 && chan[idx]==0) {
            sConfig.Channel = ap[i].ch;
            sConfig.Rank = ap[i].rank;;
            sConfig.SingleDiff = ap[i].sigdiff;
            if(ap[i].adc==ADC1) {
                if (HAL_ADC_ConfigChannel(&h->master, &sConfig) != HAL_OK) {
                    break;
                }
                h->chns++;
            }
            
            if(ap[i].adc==ADC2) {
                if (HAL_ADC_ConfigChannel(&h->slave, &sConfig) != HAL_OK) {
                    break;
                }
            }
            
            chan[idx] = 1;
        }
        i++;
    }
#else
    
    sConfig.Rank         = ADC_REGULAR_RANK_1;          /* 采样序列里的第1个 */
	sConfig.SingleDiff   = ADC_DIFFERENTIAL_ENDED;      /* 差分输入 */
	sConfig.OffsetNumber = ADC_OFFSET_NONE;             /* 无偏移 */ 
	sConfig.Offset = 0;                                 /* 无偏移的情况下，此参数忽略 */
	sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLE_5;
	sConfig.Channel      = ADC_CHANNEL_18;              /* 配置使用的ADC通道 */
	if (HAL_ADC_ConfigChannel(&h->master, &sConfig) != HAL_OK)
	{
		return -1;
	}
	
	sConfig.Channel      = ADC_CHANNEL_5;               /* 配置使用的ADC通道 */
	if (HAL_ADC_ConfigChannel(&h->slave, &sConfig) != HAL_OK)
	{
		return -1;
	}

#endif

    
    if(h->dual) {
        h->bufLen = cfg->samples*sizeof(U32)*h->chns*2;       //  * h->chns?
    }
    else {
        h->bufLen = cfg->samples*sizeof(U16)*h->chns*2;       //  * h->chns?
    }
    h->pBuf = (U32*)malloc(h->bufLen+32);
    h->pAlnBuf = (U32*)ALIGN_TO(h->pBuf, 32);
    
    
    if(HAL_ADCEx_Calibration_Start(&h->master, ADC_CALIB_OFFSET, ADC_SIGNAL_MODE) != HAL_OK) {
        return -1;
    }
    
    if(HAL_ADCEx_Calibration_Start(&h->slave, ADC_CALIB_OFFSET, ADC_SIGNAL_MODE) != HAL_OK) {
        return -1;
    }
    
    if(h->dual) {
        mInit.Mode = ADC_DUALMODE_REGSIMULT;                        // ADC_MODE_INDEPENDENT
        mInit.DualModeData = ADC_DUALMODEDATAFORMAT_32_10_BITS;     /* ADC and DMA configured in resolution 32 bits to match with both ADC master and slave resolution */
        mInit.TwoSamplingDelay = ADC_TWOSAMPLINGDELAY_1CYCLE;
        if (HAL_ADCEx_MultiModeConfigChannel(&h->master, &mInit) != HAL_OK) {
            return -1;
        }        
    }
    
    return 0;
}


int dal_adc_deinit(void)
{
    adc_handle_t *h=adcHandle;
    
    HAL_ADC_DeInit(&h->master);
    HAL_ADC_DeInit(&h->slave);
    free(h->pBuf);
    free(h);
    
    return 0;
}



HAL_StatusTypeDef HAL_ADCEx_MultiBufferStart_DMA(ADC_HandleTypeDef *hadc, uint32_t* pPingData, uint32_t* pPongData, uint32_t Length)
{
	HAL_StatusTypeDef tmp_hal_status = HAL_OK;
	ADC_HandleTypeDef tmphadcSlave;
	ADC_Common_TypeDef *tmpADC_Common;

	/* Check the parameters */
	assert_param(IS_ADC_MULTIMODE_MASTER_INSTANCE(hadc->Instance));
	assert_param(IS_FUNCTIONAL_STATE(hadc->Init.ContinuousConvMode));
	assert_param(IS_ADC_EXTTRIG_EDGE(hadc->Init.ExternalTrigConvEdge));

	if (ADC_IS_CONVERSION_ONGOING_REGULAR(hadc))
	{
		return HAL_BUSY;
	}
	else
	{
		/* Process locked */
		__HAL_LOCK(hadc);

		/* Set a temporary handle of the ADC slave associated to the ADC master   */
		ADC_MULTI_SLAVE(hadc, &tmphadcSlave);

		if (tmphadcSlave.Instance == NULL)
		{
			/* Update ADC state machine to error */
			SET_BIT(hadc->State, HAL_ADC_STATE_ERROR_CONFIG);

			/* Process unlocked */
			__HAL_UNLOCK(hadc);

			return HAL_ERROR;
		}

		/* Enable the ADC peripherals: master and slave (in case if not already   */
		/* enabled previously)                                                    */
		tmp_hal_status  = ADC_Enable(hadc);
		if (tmp_hal_status  == HAL_OK)
		{
			tmp_hal_status  = ADC_Enable(&tmphadcSlave);
		}

		/* Start multimode conversion of ADCs pair */
		if (tmp_hal_status == HAL_OK)
		{
			/* Update Master State */
			/* Clear HAL_ADC_STATE_READY and regular conversion results bits, set HAL_ADC_STATE_REG_BUSY bit */
			ADC_STATE_CLR_SET(hadc->State, (HAL_ADC_STATE_READY|HAL_ADC_STATE_REG_EOC|HAL_ADC_STATE_REG_OVR|HAL_ADC_STATE_REG_EOSMP), HAL_ADC_STATE_REG_BUSY);

			/* Set ADC error code to none */
			ADC_CLEAR_ERRORCODE(hadc);

			/* Set the DMA transfer complete callback */
			hadc->DMA_Handle->XferCpltCallback = ADC_DMAConvCplt;
			// ??DMA?????,PingPong????????????????
			hadc->DMA_Handle->XferM1CpltCallback = ADC_DMAConvCplt;

			/* Set the DMA half transfer complete callback */
			hadc->DMA_Handle->XferHalfCpltCallback = NULL; // ???DMA???????

			/* Set the DMA error callback */
			hadc->DMA_Handle->XferErrorCallback = ADC_DMAError ;

			/* Pointer to the common control register  */
			tmpADC_Common = ADC12_COMMON_REGISTER(hadc);

			/* Manage ADC and DMA start: ADC overrun interruption, DMA start, ADC     */
			/* start (in case of SW start):                                           */

			/* Clear regular group conversion flag and overrun flag */
			/* (To ensure of no unknown state from potential previous ADC operations) */
			__HAL_ADC_CLEAR_FLAG(hadc, (ADC_FLAG_EOC | ADC_FLAG_EOS | ADC_FLAG_OVR));

			/* Enable ADC overrun interrupt */
			__HAL_ADC_ENABLE_IT(hadc, ADC_IT_OVR);

			/* Start the DMA channel */
//			HAL_DMA_Start_IT(hadc->DMA_Handle, (INT32U)&tmpADC_Common->CDR, (INT32U)pData, Length);
			HAL_DMAEx_MultiBufferStart_IT(hadc->DMA_Handle, (uint32_t)&tmpADC_Common->CDR, (uint32_t)pPingData, (uint32_t)pPongData, Length);

			/* Enable conversion of regular group.                                    */
			/* Process unlocked */
			__HAL_UNLOCK(hadc);
			/* If software start has been selected, conversion starts immediately.    */
			/* If external trigger has been selected, conversion will start at next   */
			/* trigger event.                                                         */
//			SET_BIT(hadc->Instance->CR, ADC_CR_ADSTART); // ???ADC??
		}
		else
		{
			/* Process unlocked */
			__HAL_UNLOCK(hadc);
		}

		/* Return function status */
		return tmp_hal_status;
	}
}




int dal_adc_start(void)
{
    HAL_StatusTypeDef st;
    adc_handle_t *h=adcHandle;
    
    if(h->mode==MODE_DMA) {
        if(h->dual) {
            //HAL_ADCEx_MultiModeStart_DMA(&h->master, (uint32_t*)h->pAlnBuf, h->bufLen/sizeof(U32));
            HAL_ADCEx_MultiBufferStart_DMA(&h->master, h->pAlnBuf, h->pAlnBuf+h->bufLen/8, h->bufLen/sizeof(U32));
            SET_BIT(h->master.Instance->CR, ADC_CR_ADSTART);
        }
        else {
            HAL_ADC_Start_DMA(&h->master, (uint32_t*)h->pAlnBuf, h->bufLen/sizeof(U16));
        }
        
        //tim_start();
    }
    else if (h->mode==MODE_IT) {
        HAL_ADC_Start_IT(&h->master);
    }
    else {
        HAL_ADC_Start(&h->master);
    }
    

    return 0;
}

int dal_adc_stop(void)
{
    adc_handle_t *h=adcHandle;
    
    if(h->mode==MODE_DMA) {
        if(h->dual) {
            HAL_ADCEx_MultiModeStop_DMA(&h->master);
        }
        else {
            HAL_ADC_Stop_DMA(&h->master);
        }
        
        //_tim_stop();
    }
    else if (h->mode==MODE_IT) {
        HAL_ADC_Stop_IT(&h->master);
    }
    else {
        HAL_ADC_Stop(&h->master);
    }
    
    return 0;
}


int dal_adc_get(U32 *adc)
{
    
    return 0;
}

#endif
