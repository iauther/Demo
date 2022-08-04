#include "drv/adc.h"
#include "drv/gpio.h"
#include "cfg.h"

//http://t.zoukankan.com/armfly-p-12180331.html
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
static U32 adcValue=0;
static adc_pin_t adcPins[]=ADC_SAMPLE_CHANS;
static dma_para_t adcDMAPara[ADC_MAX]={
    {ADC1, DMA2_Stream0_IRQn,  DMA_REQUEST_ADC1,   DMA2_Stream0},
    {ADC2, DMA1_Stream0_IRQn,  DMA_REQUEST_ADC2,   DMA1_Stream0}, 
    //{ADC3, BDMA_Channel0_IRQn, BDMA_REQUEST_ADC3,  MDMA_Channel0} 
};


#define VDD_APPLI           ((U32) 3300)    /* Value of analog voltage supply Vdda (unit: mV) */
#define RANGE_8BITS         ((U32)  255)    /* Max digital value with a full range of 8 bits */
#define RANGE_12BITS        ((U32) 4095)    /* Max digital value with a full range of 12 bits */
#define RANGE_16BITS        ((U32)65535)    /* Max digital value with a full range of 16 bits */

#define BUF_LEN             1000
#define S_ADCTO(v)          ((3.3F/65535)*v)
#define D_ADCTO(v)          (((2.0F*v)/65536-1)*3.3F)

#define ADC_TRIGGER_FROM_TIMER



typedef struct {
    ADC_HandleTypeDef hadc;
    DMA_HandleTypeDef hdma;
}adc_t;
typedef struct {
    U8      dural;
    adc_t   adc[ADC_MAX];
    TIM_HandleTypeDef htim;
    
    U32     adcBuf[BUF_LEN];
    F32     volBuf[BUF_LEN];
}adc_handle_t;


static adc_handle_t adcHandle={0};

static void get_adc(void)
{
    int i;
    
    adcValue = 0;
    for(i=0; ; i++) {
        if(adcPins[i].pin.grp==0) {
            break;
        }
        
        if(adcPins[i].adc==ADC1) {
            adcValue |= (1<<ADC_1);
        }
        else if(adcPins[i].adc==ADC2) {
            adcValue |= (1<<ADC_2);
        }
        else if(adcPins[i].adc==ADC3) {
            adcValue |= (1<<ADC_3);
        }   
    }

}
static void adc_val_conv(adc_handle_t *h)
{
    int i;
    
    for(i=0; i<BUF_LEN; i++) {
        h->volBuf[i] = D_ADCTO(h->adcBuf[i]);
    }
}

static int adc_set_clk(void)
{
    RCC_PeriphCLKInitTypeDef Perip;
    
    HAL_RCCEx_GetPeriphCLKConfig(&Perip);
    Perip.PeriphClockSelection = RCC_PERIPHCLK_ADC;
    
    Perip.PLL2.PLL2M = 2;
	Perip.PLL2.PLL2N = 15;
	Perip.PLL2.PLL2P = 2; // 92.16MHz
	Perip.PLL2.PLL2Q = 16; // 11.52MHz
	Perip.PLL2.PLL2R = 3;
	Perip.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_3;
	Perip.PLL2.PLL2VCOSEL = RCC_PLL2VCOMEDIUM;
	Perip.PLL2.PLL2FRACN = 0;
	Perip.PLL3.PLL3M = 3;
	Perip.PLL3.PLL3N = 80;
	Perip.PLL3.PLL3P = 25; // 26.2144MHz
	Perip.PLL3.PLL3Q = 8;
	Perip.PLL3.PLL3R = 32; // 20.48MHz
	Perip.PLL3.PLL3RGE = RCC_PLL3VCIRANGE_3;
	Perip.PLL3.PLL3VCOSEL = RCC_PLL3VCOWIDE;
	Perip.PLL3.PLL3FRACN = 0;
    
	
	//Perip.Sai1ClockSelection = RCC_SAI1CLKSOURCE_PLL3;                // Vib SAI
	//Perip.Sai4AClockSelection = RCC_SAI4ACLKSOURCE_PLL3;              // Vib SAI MCLK & FS & SCK
	//Perip.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_PLL2;  // RS232 USART3
	//Perip.Usart16ClockSelection = RCC_USART16CLKSOURCE_PLL2;          // RS485 USART6
    //Perip.UsbClockSelection = RCC_USBCLKSOURCE_PLL;
	Perip.AdcClockSelection = RCC_ADCCLKSOURCE_PLL2;                    // RCC_ADCCLKSOURCE_PLL3 RCC_ADCCLKSOURCE_CLKP
	//Perip.SdmmcClockSelection = RCC_SDMMCCLKSOURCE_PLL;
	//Perip.Spi123ClockSelection = RCC_SPI123CLKSOURCE_PLL2;            // SPI 92.16MHz
	//Perip.QspiClockSelection = RCC_QSPICLKSOURCE_CLKP;                // QSPI PSRAM 64MHz
	//Perip.CkperClockSelection = RCC_CLKPSOURCE_HSI;
    if (HAL_RCCEx_PeriphCLKConfig(&Perip) != HAL_OK) {
        return -1;
    }
    
    return 0;
}


static void adc_en_clk(U8 adc, U8 on)
{
    if(adc==ADC_1 || adc==ADC_2) {
        if(on) __HAL_RCC_ADC12_CLK_ENABLE();
        else   __HAL_RCC_ADC12_CLK_DISABLE();
    }
    else if(adc==ADC_3) {
        if(on) __HAL_RCC_ADC3_CLK_ENABLE();
        else   __HAL_RCC_ADC3_CLK_DISABLE();
    }
}
static void dma_en_clk(U8 adc)
{
    if(adc==ADC_1) {
        __HAL_RCC_DMA2_CLK_ENABLE();
    }
    else if(adc==ADC_2) {
        __HAL_RCC_DMA1_CLK_ENABLE();
    }
    else if(adc==ADC_3) {
        __HAL_RCC_BDMA_CLK_ENABLE();
    }
}


static int adc_dma_init(adc_handle_t *h)
{
    int i;
    ADC_HandleTypeDef *phadc;
    DMA_HandleTypeDef *phdma,hdma={0};
    
    hdma.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma.Init.MemInc = DMA_MINC_ENABLE;
    hdma.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma.Init.Mode = DMA_CIRCULAR;
    hdma.Init.Priority = DMA_PRIORITY_LOW;
    hdma.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    
    for(i=ADC_1; i<ADC_MAX; i++) {
        if(adcValue & (1<<i)) {
            hdma.Instance = adcDMAPara[i].stream;
            hdma.Init.Request = adcDMAPara[i].request;
            
            h->adc[i].hdma = hdma; phadc = &h->adc[i].hadc; phdma = &h->adc[i].hdma;
            dma_en_clk(i);
            
            HAL_NVIC_SetPriority(adcDMAPara[i].IRQn, 0, 0);
            HAL_NVIC_EnableIRQ(adcDMAPara[i].IRQn);
            
            __HAL_LINKDMA(phadc, DMA_Handle, *phdma);
        }
        
    }
    
    return 0;
}
static int adc_add_channel(ADC_HandleTypeDef *hadc, U8 dural)
{
    int i;
    adc_pin_t *ap = adcPins;
    ADC_ChannelConfTypeDef sConfig = {0};
    
    sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0;
    sConfig.OffsetRightShift = DISABLE;
    sConfig.OffsetSignedSaturation = DISABLE;
    for(i=0;; i++) {
        if(ap[i].pin.grp==0) {
            break;
        }
        
        if((dural && ((ap[i].adc==ADC1) ||(ap[i].adc==ADC2))) 
            || (ap[i].adc==hadc->Instance)) {
            sConfig.Channel = ap[i].chn;
            sConfig.Rank = ap[i].rank;;
            sConfig.SingleDiff = ap[i].sigdiff;
            if (HAL_ADC_ConfigChannel(hadc, &sConfig) != HAL_OK) {
                continue;
            }
        }
    }
    
    return 0;
}


static int adc_ctl_init(adc_handle_t *h)
{
    int i;
    ADC_MultiModeTypeDef   mInit = {0};
    
    ADC_HandleTypeDef hadc,*phadc;
    
#ifdef ADC_TRIGGER_FROM_TIMER
    hadc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV1;                //AHB clock
    hadc.Init.ExternalTrigConv = ADC_EXTERNALTRIG_T4_TRGO;
    hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
    hadc.Init.ContinuousConvMode = DISABLE;
#else
    hadc.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
    hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIG_EDGE_NONE;
    hadc.Init.ContinuousConvMode = ENABLE;
#endif
    
    hadc.Init.Resolution = ADC_RESOLUTION_16B;
    hadc.Init.ScanConvMode = ADC_SCAN_ENABLE;
    hadc.Init.EOCSelection = ADC_EOC_SEQ_CONV;
    hadc.Init.LowPowerAutoWait = DISABLE;
    
    hadc.Init.NbrOfConversion = sizeof(adcPins)/sizeof(adc_pin_t)-1;
    hadc.Init.DiscontinuousConvMode = DISABLE;
    hadc.Init.NbrOfDiscConversion = 0;
    
    hadc.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DMA_CIRCULAR;
    hadc.Init.LeftBitShift = ADC_LEFTBITSHIFT_NONE;
    
    hadc.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;   //ADC_OVR_DATA_PRESERVED
    hadc.Init.OversamplingMode = DISABLE;
    
    hadc.Init.Oversampling.Ratio = 1023;
    hadc.Init.Oversampling.RightBitShift  = ADC_RIGHTBITSHIFT_1;
    hadc.Init.Oversampling.TriggeredMode  = ADC_TRIGGEREDMODE_SINGLE_TRIGGER;
    
    hadc.Init.Oversampling.OversamplingStopReset = ADC_REGOVERSAMPLING_CONTINUED_MODE;

    if(adcHandle.dural) {
        for(i=ADC_1; i<ADC_3; i++) {
            hadc.Instance = adcDMAPara[i].adc; h->adc[i].hadc  = hadc; phadc = &h->adc[i].hadc;
            if(i==ADC_2) {
                phadc->Init.ContinuousConvMode = DISABLE;
                phadc->Init.ExternalTrigConv = ADC_SOFTWARE_START;
            }
            
            if (HAL_ADC_Init(phadc) != HAL_OK) {
                return -1;
            }
            adc_add_channel(phadc, 1);
            
            HAL_ADCEx_Calibration_Start(phadc, ADC_CALIB_OFFSET, ADC_SIGNAL_MODE);
        }
    }
    else {
        for(i=ADC_1; i<ADC_MAX; i++) {
            if(adcValue & (1<<i)) {
                hadc.Instance = adcDMAPara[i].adc; h->adc[i].hadc  = hadc; phadc = &h->adc[i].hadc;
                
                if (HAL_ADC_Init(phadc) != HAL_OK) {
                    return -1;
                }
                
                adc_add_channel(phadc, 0);
                HAL_ADCEx_Calibration_Start(phadc, ADC_CALIB_OFFSET, ADC_SIGNAL_MODE);
                //__HAL_ADC_ENABLE_IT(&h->adc[i].hadc, ADC_IT_EOS);
            } 
        }
    }

    if(adcHandle.dural) {
        mInit.Mode = ADC_DUALMODE_REGSIMULT;                        // ADC_MODE_INDEPENDENT
        mInit.DualModeData = ADC_DUALMODEDATAFORMAT_32_10_BITS;     /* ADC and DMA configured in resolution 32 bits to match with both ADC master and slave resolution */
        mInit.TwoSamplingDelay = ADC_TWOSAMPLINGDELAY_1CYCLE;
        if (HAL_ADCEx_MultiModeConfigChannel(&h->adc[ADC_1].hadc, &mInit) != HAL_OK) {return -1;}
    }
    
    return 0;
}


void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
{
    int i;
    adc_pin_t *ap = adcPins;
    GPIO_InitTypeDef init = {0};
    
    init.Mode = GPIO_MODE_ANALOG;
    init.Pull = GPIO_NOPULL;
    for(i=ADC_1; i<ADC_MAX; i++) {
        
        if(adcValue & (1<<i)) {
            if(ap[i].pin.grp==0) {
                break;
            }
        }
            
        adc_en_clk(i, 1);
        gpio_en_clk(ap[i].pin.grp, 1);
        HAL_GPIO_Init(ap[i].pin.grp, &init);
        HAL_DMA_DeInit(&adcHandle.adc[i].hdma);
    }
}


void HAL_ADC_MspDeInit(ADC_HandleTypeDef* hadc)
{
    int i;
    adc_pin_t *ap = adcPins;
    
    for(i=ADC_1; i<ADC_MAX; i++) {
        
        if(adcValue & (1<<i)) {
            if(ap[i].pin.grp==0) {
                break;
            }
        }
            
        adc_en_clk(i, 0);
        gpio_en_clk(ap[i].pin.grp, 0);
        HAL_GPIO_DeInit(ap[i].pin.grp, ap[i].pin.pin);
        HAL_DMA_DeInit(&adcHandle.adc[i].hdma);
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    
    //SCB_InvalidateDCache_by_Addr((uint32_t *) &aADCDualConvertedValues[ADCCONVERTEDVALUES_BUFFER_SIZE/2], 4*ADCCONVERTEDVALUES_BUFFER_SIZE/2);

#ifdef ADC_TRIGGER_FROM_TIMER
    adc_val_conv(&adcHandle);
#endif
}

/*void DMA1_Stream0_IRQHandler(void)  //ADC2
{
    HAL_DMA_IRQHandler(&adcHandle.adc2.hdma);
}*/
void DMA2_Stream0_IRQHandler(void)  //ADC1
{
    int i;

    if(adcHandle.dural) {
        if(adcValue&(1<<ADC_1)) {
            HAL_DMA_IRQHandler(&adcHandle.adc[ADC_1].hdma);
        }
    }
    else {
        if(adcValue&(1<<ADC_1)) {
            HAL_DMA_IRQHandler(&adcHandle.adc[ADC_1].hdma);
        }
        
        if(adcValue&(1<<ADC_2)) {
            HAL_DMA_IRQHandler(&adcHandle.adc[ADC_2].hdma);
        }
    }
}
void BDMA_Channel0_IRQHandler(void) //ADC3
{
    if(adcValue&(1<<ADC_3)) {
        HAL_DMA_IRQHandler(&adcHandle.adc[ADC_3].hdma);
    }
}



void TIM4_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&adcHandle.htim);
}
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* htim)
{
    if(htim->Instance==TIM4){
        __HAL_RCC_TIM4_CLK_ENABLE();

        HAL_NVIC_SetPriority(TIM4_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(TIM4_IRQn);
    }
}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    
}
static int tim_init(void)
{
    TIM_HandleTypeDef *htim=&adcHandle.htim;
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    htim->Instance = TIM4;
    htim->Init.Prescaler = 36000-1;
    htim->Init.CounterMode = TIM_COUNTERMODE_UP;
    htim->Init.Period = 2000-1;
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
    HAL_TIM_Base_Start_IT(htim);

    return 0;
}


int adc_init(adc_cfg_t *cfg)
{
    get_adc();
    adc_set_clk();
    
    adc_dma_init(&adcHandle);
    adc_ctl_init(&adcHandle);
    
    adc_start();
    
    return 0;
}



int adc_start(void)
{
    int i,dflag=0;
    
    for(i=ADC_1; i<ADC_MAX; i++) {
        if(adcValue & (1<<i)) {
            if(adcHandle.dural) {
                if(dflag==0 && (i==ADC_1 || i==ADC_2)) {
                    HAL_ADCEx_MultiModeStart_DMA(&adcHandle.adc[i].hadc,adcHandle.adcBuf, BUF_LEN);
                    dflag = 1;
                }
                else {
                    HAL_ADC_Start_DMA(&adcHandle.adc[i].hadc, adcHandle.adcBuf, BUF_LEN);
                }
            }
            else {
                HAL_ADC_Start_DMA(&adcHandle.adc[i].hadc, adcHandle.adcBuf, BUF_LEN);
            }
        }
    }
    
    //HAL_TIM_Base_Start(&adcHandle.htim);
    

    return 0;
}

int adc_stop(void)
{
    if(adcHandle.dural) {
        //HAL_ADCEx_MultiModeStop_DMA(&adcHandle.adc[ADC_1].hadc,(U32 *)aADCDualConvertedValues, 
        //                                 ADCCONVERTEDVALUES_BUFFER_SIZE) != HAL_OK) {
    }
    else {
        HAL_ADC_Stop_DMA(&adcHandle.adc[ADC_1].hadc);
    }
    
    //HAL_TIM_Base_Stop(&adcHandle.htim);
    
    return 0;
}


int adc_get(U32 *adc)
{
    
    return 0;
}












