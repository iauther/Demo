#include "adc.h"

//https://www.cnblogs.com/armfly/p/12180331.html


typedef struct {
    ADC_HandleTypeDef hadc;
    
    U32 channel;
}adc_handle_t;



U16 adcValue[ADC_CH];
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;
static void dma_start(ADC_HandleTypeDef *hadc)
{
    __HAL_RCC_DMA1_CLK_ENABLE();

    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 1, 2);
    HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
    
    HAL_ADC_Start_DMA(hadc,(U32*)adcValue, ADC_CH);
}

static void dma_deinit(void)
{
    __HAL_RCC_DMA1_CLK_DISABLE();
    HAL_NVIC_DisableIRQ(DMA1_Channel1_IRQn);
}

static void dma_config(ADC_HandleTypeDef *hadc, DMA_HandleTypeDef *hdma)
{
    //dma config
    hdma->Instance = DMA1_Channel1;
    hdma->Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma->Init.PeriphInc = DMA_PINC_DISABLE;
    hdma->Init.MemInc = DMA_MINC_ENABLE;
    hdma->Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma->Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma->Init.Mode = DMA_CIRCULAR;
    hdma->Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(hdma) != HAL_OK) {
        return;
    }

    //__HAL_LINKDMA(hadc, DMA_Handle, hdma);
}



void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    U8 key;
    //HAL_ADC_Stop_DMA(&hadc1);

    //HAL_ADC_Start_DMA(&hadc1,(U32*)adcValue, ADC_CH);
}


#if 0
static void timer_period_callback(TIM_HandleTypeDef *htim)
{
    ii++;
}
static void adc_tim_init(void)
{
    TIM_Base_InitTypeDef init;
    TIM_MasterConfigTypeDef cfg;

    init.Prescaler = 18000-1;
    init.CounterMode = TIM_COUNTERMODE_UP;
    init.Period = 20 ;
    init.ClockDivision = TIM_CLOCKDIVISION_DIV4;
    init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;//TIM_AUTORELOAD_PRELOAD_DISABLE;

    cfg.MasterOutputTrigger = TIM_TRGO_UPDATE;
    cfg.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    
    timer_init(TIMER_3, &init, timer_period_callback);
    timer_master_confg(TIMER_3, &cfg);
    time_dma_init(TIMER_3);
    timer_start(TIMER_3);
}
#endif


static ADC_TypeDef* getADC(gpio_pin_t *pin)
{
    switch((U32)pin->grp) {
        
        case (U32)GPIOA:
        {
            if(pin->pin==GPIO_PIN_0 || pin->pin==GPIO_PIN_1) {
                return ADC1;
            }
        }
        break;
    }
    
    return NULL;
}


static U32 getChannel(gpio_pin_t *pin)
{
    switch((U32)pin->grp) {
        
        case (U32)GPIOA:
        {
            if(pin->pin==GPIO_PIN_0) {
                return ADC_CHANNEL_0;
            }
            else if(pin->pin==GPIO_PIN_1) {
                return ADC_CHANNEL_1;
            }
        }
        break;
    }
    
    return ADC_CHANNEL_0;
}


handle_t adc_init(adc_cfg_t *cfg)
{
    
    adc_handle_t *h=(adc_handle_t*)malloc(sizeof(adc_handle_t));
    
    if(!cfg || !h) {
        return NULL;
    }
    
    h->hadc.Instance = getADC(&cfg->pin);
    h->hadc.Init.ScanConvMode = ADC_SCAN_DISABLE;
    h->hadc.Init.ContinuousConvMode = DISABLE;
    h->hadc.Init.DiscontinuousConvMode = DISABLE;
    h->hadc.Init.NbrOfConversion = 1;
    h->hadc.Init.NbrOfDiscConversion = 0;
    //h->hadc.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T3_TRGO;
    h->hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    h->hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    if (HAL_ADC_Init(&h->hadc) != HAL_OK) {
        goto fail;
    }
    
    //HAL_ADCEx_Calibration_Start(&h->hadc);

    //dma_start();
    //HAL_ADC_Start(&h->hadc);
    //HAL_ADC_Start(&h->hadc);

    return h;
    
fail:
    free(h);
    return NULL;
}


int adc_deinit(handle_t *h)
{
    adc_handle_t **ah=(adc_handle_t**)h;
    
    if(!ah || !(*ah)) {
        return -1;
    }
    
    HAL_ADC_DeInit(&(*ah)->hadc);
    free(*ah);
    //dma_deinit();
    
    return 0;
}

U32 adc_read(handle_t h, U32 ch)
{
    U32 i,v=0;
    adc_data_t dat;
    ADC_ChannelConfTypeDef sc = {0};
    adc_handle_t *ah=(adc_handle_t*)h;
    
    sc.Channel = ch;
    sc.Rank = ADC_REGULAR_RANK_1;
    sc.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    if (HAL_ADC_ConfigChannel(&ah->hadc, &sc) != HAL_OK) {
        return 0;
    }
    
    HAL_ADC_Start(&ah->hadc);
    if(HAL_ADC_PollForConversion(&ah->hadc, 500)==HAL_OK) {
        v = HAL_ADC_GetValue(&ah->hadc);
    }
    HAL_ADC_Stop(&ah->hadc);
    
    return v;
}




void HAL_ADC_MspInit(ADC_HandleTypeDef* adcHandle)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if(adcHandle->Instance==ADC1)
    {
        /* ADC1 clock enable */
        __HAL_RCC_ADC1_CLK_ENABLE();

        //__HAL_RCC_GPIOC_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        
        //dma_config();
        
        /* ADC1 interrupt Init */
        //HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
        //HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
    }
    else if(adcHandle->Instance==ADC2)  {
        
        
    }
}

void HAL_ADC_MspDeInit(ADC_HandleTypeDef* adcHandle)
{
    if(adcHandle->Instance==ADC1) {
        /* Peripheral clock disable */
        __HAL_RCC_ADC1_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_0|GPIO_PIN_1);
        
        //HAL_DMA_DeInit(adcHandle->DMA_Handle);

        /* ADC1 interrupt Deinit */
        //HAL_NVIC_DisableIRQ(DMA1_Channel1_IRQn);
    }
} 

