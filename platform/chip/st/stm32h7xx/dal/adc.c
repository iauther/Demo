#include "dal/dal.h"
#include "cfg.h"

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
static adc_pin_t adcPins[]=ADC_SAMPLE_CHANS;



#define ADC_BUF_LEN    256

#define VDD_APPLI           ((U32) 3300)    /* Value of analog voltage supply Vdda (unit: mV) */
#define RANGE_8BITS         ((U32)  255)    /* Max digital value with a full range of 8 bits */
#define RANGE_12BITS        ((U32) 4095)    /* Max digital value with a full range of 12 bits */
#define RANGE_16BITS        ((U32)65535)    /* Max digital value with a full range of 16 bits */

#define S_ADCTO(v)          ((3.3F/65535)*v)
#define D_ADCTO(v)          (((2.0F*v)/65535-1)*3.3F)


//#define ADC_TRIGGER_FROM_TIMER
//#define STORE_DATA_IN_SDRAM
#ifdef STORE_DATA_IN_SDRAM
#define STORE_LEN_MAX       (10*1024*1024)
static int store_data_len=0;
static F32 sdram_store_data[STORE_LEN_MAX] __attribute__ ((at(0xD3200000))); // SDRAM
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
    U8                  dural;
    ADC_HandleTypeDef   master;
    ADC_HandleTypeDef   slave;
    
    DMA_HandleTypeDef   hdma;
    
    TIM_HandleTypeDef   htim;
    
    U32                 buf[ADC_BUF_LEN*2];
    F32                 vol[ADC_BUF_LEN*2];
    
}adc_handle_t;


static adc_handle_t adcHandle={
    .dural = ADC_DURAL_MODE,
    .master = {0},
    .slave  = {0},
};


///////////////timer function start/////////////////////////
//TIM2~7,12~14     TIM1,8,15~17 equal sys_freq/2
void TIM4_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&adcHandle.htim);
}
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* htim)
{
    if(htim->Instance==TIM4){
        __HAL_RCC_TIM4_CLK_ENABLE();

        HAL_NVIC_SetPriority(TIM4_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(TIM4_IRQn);
    }
}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    
}
static int tim_init(U32 freq)
{
    TIM_HandleTypeDef *htim=&adcHandle.htim;
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
    return HAL_TIM_Base_Start(&adcHandle.htim);
}
static int tim_stop(void)
{
    return HAL_TIM_Base_Stop(&adcHandle.htim);
}
///////////////timer function end/////////////////////////



static void adc_conv(U32 *buf, F32 *vol, U32 count)
{
    int i;
    U16 mst,slv;
    
    for(i=0; i<count; i++) {
        
        mst = buf[i]&0xffff;
        slv = buf[i]>>16;
        
        vol[i*2]   = D_ADCTO(mst);
        vol[i*2+1] = D_ADCTO(slv);
    }
    
#ifdef STORE_DATA_IN_SDRAM
    if(store_data_len<STORE_LEN_MAX) {
        memcpy(sdram_store_data+store_data_len, h->vol, BUF_LEN*2);
        store_data_len += BUF_LEN*2;
    }
    else {
        static int store_it=0;
        
        store_it = 1;
    }
#endif
    
}


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
        if(ap[i].pin.grp==0) {
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
    adc_handle_t *h=&adcHandle;
    
    if(hadc->Instance == ADC1) {
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
    
        __HAL_RCC_ADC12_CLK_ENABLE();
        __HAL_RCC_DMA2_CLK_ENABLE();
        
        _gpio_init(1);
        
        h->hdma.Instance = DMA2_Stream0;
        h->hdma.Init.Request = DMA_REQUEST_ADC1;
        HAL_DMA_DeInit(&h->hdma);
        
        HAL_DMA_Init(&h->hdma);
        __HAL_LINKDMA(hadc, DMA_Handle, h->hdma);
        
        HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
        
        //HAL_NVIC_SetPriority(ADC_IRQn, 5, 0);
        //HAL_NVIC_EnableIRQ(ADC_IRQn);
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
    adc_handle_t *h=&adcHandle;
    
    adc_conv(h->buf+ADC_BUF_LEN, h->vol+ADC_BUF_LEN, ADC_BUF_LEN);
}
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    adc_handle_t *h=&adcHandle;
    
    //SCB_InvalidateDCache_by_Addr(buf, ADC_BUF_LEN*4);
    
    adc_conv(h->buf, h->vol, ADC_BUF_LEN);
    
}


/*
{ADC1, DMA2_Stream0_IRQn,  DMA_REQUEST_ADC1,   DMA2_Stream0},
{ADC2, DMA2_Stream2_IRQn,  DMA_REQUEST_ADC2,   DMA2_Stream2}, 
{ADC3, BDMA_Channel0_IRQn, BDMA_REQUEST_ADC3,  MDMA_Channel0}
*/
void DMA2_Stream0_IRQHandler(void)  //ADC1
{
    HAL_DMA_IRQHandler(&adcHandle.hdma);
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


static int _adc_init(void)
{
    int i=0,idx,chan[20]={0};
    ADC_ChannelConfTypeDef sConfig = {0};
    ADC_MultiModeTypeDef mInit = {0};
    adc_handle_t *h=&adcHandle;
    adc_pin_t *ap=adcPins;
    ADC_HandleTypeDef hadc;
    
    h->master.Instance = ADC1;
    
#ifdef ADC_TRIGGER_FROM_TIMER
    h->master.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV1;                //AHB clock
    h->master.Init.ExternalTrigConv = ADC_EXTERNALTRIG_T4_TRGO;
    h->master.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
    h->master.Init.ContinuousConvMode = DISABLE;
#else
    h->master.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
    h->master.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    h->master.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIG_EDGE_NONE;
    h->master.Init.ContinuousConvMode = ENABLE;
#endif
    
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
    
#ifdef USE_ADC_DEINIT
    if (HAL_ADC_DeInit(&h->master) != HAL_OK) {
        return -1;
    }
#endif
    
    if (HAL_ADC_Init(&h->master) != HAL_OK) {
        return -1;
    }
            
    if(h->dural) {
        h->slave.Instance  = ADC2;
        h->slave.Init = h->master.Init;
        h->slave.Init.ContinuousConvMode = DISABLE;
        h->slave.Init.ExternalTrigConv = ADC_SOFTWARE_START;
        
#ifdef USE_ADC_DEINIT
        if (HAL_ADC_DeInit(&h->slave) != HAL_OK) {
            return -1;
        }
#endif
        
        if (HAL_ADC_Init(&h->slave) != HAL_OK) {
            return -1;
        }
    }
            
    //add channels
    sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
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
            if (HAL_ADC_ConfigChannel(&h->master, &sConfig) != HAL_OK) {
                break;
            }
            
            if(h->dural) {
                if (HAL_ADC_ConfigChannel(&h->slave, &sConfig) != HAL_OK) {
                    break;
                }
            }
            
            chan[idx] = 1;
        }
        i++;
    }
    
    if(HAL_ADCEx_Calibration_Start(&h->master, ADC_CALIB_OFFSET, ADC_SIGNAL_MODE) != HAL_OK) {
        return -1;
    }
    
    if(h->dural) {
        if(HAL_ADCEx_Calibration_Start(&h->slave, ADC_CALIB_OFFSET, ADC_SIGNAL_MODE) != HAL_OK) {
            return -1;
        }
        
        mInit.Mode = ADC_DUALMODE_REGSIMULT;                        // ADC_MODE_INDEPENDENT
        mInit.DualModeData = ADC_DUALMODEDATAFORMAT_32_10_BITS;     /* ADC and DMA configured in resolution 32 bits to match with both ADC master and slave resolution */
        mInit.TwoSamplingDelay = ADC_TWOSAMPLINGDELAY_1CYCLE;
        if (HAL_ADCEx_MultiModeConfigChannel(&h->master, &mInit) != HAL_OK) {
            return -1;
        }        
    }
    
    //delay_ms(10);
    
    return 0;
}


static int _adc_deinit(void)
{
    int i;
    adc_handle_t *h=&adcHandle;
    
    HAL_ADC_DeInit(&h->master);
    HAL_ADC_DeInit(&h->slave);
    
    return 0;
}

static int _adc_start(void)
{
    adc_handle_t *h=&adcHandle;
    
    if (HAL_ADCEx_MultiModeStart_DMA(&h->master, h->buf, sizeof(h->buf)) != HAL_OK) {
        return -1;
    }

    //tim_start();

    return 0;
}

static int _adc_stop(void)
{
    adc_handle_t *h=&adcHandle;
    
    HAL_ADC_Stop(&h->master);
    HAL_ADCEx_MultiModeStop_DMA(&h->master);
    
    //_tim_stop();
    
    return 0;
}
///////////////timer function end/////////////////////////


int adc_init(void)
{   
    _adc_init();
    _adc_start();
    
    return 0;
}


int adc_deinit(void)
{
    _adc_stop();
    
    
    return 0;
}



int adc_start(void)
{
    return _adc_start();
}

int adc_stop(void)
{
    return _adc_stop();
}


int adc_get(U32 *adc)
{
    
    return 0;
}












