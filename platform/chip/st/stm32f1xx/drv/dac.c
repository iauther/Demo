#include "dac.h"

//https://www.cnblogs.com/armfly/p/12180331.html


typedef struct {
    DAC_HandleTypeDef hdac;
    
    U32 channel;
}dac_handle_t;


static ADC_TypeDef* getDAC(gpio_pin_t *pin)
{
    switch((U32)pin->grp) {
        
        case (U32)GPIOA:
        {
            //if(pin->pin==GPIO_PIN_0 || pin->pin==GPIO_PIN_1 || pin->pin==GPIO_PIN_2) {
                //return DAC1;
            //}
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
                return DAC_CHANNEL_1;
            }
            else if(pin->pin==GPIO_PIN_1) {
                return DAC_CHANNEL_1;
            }
            else if(pin->pin==GPIO_PIN_2) {
                return DAC_CHANNEL_1;
            }
        }
        break;
    }
    
    return DAC_CHANNEL_1;
}


handle_t dac_init(dac_cfg_t *cfg)
{
    
    dac_handle_t *h=(dac_handle_t*)malloc(sizeof(dac_handle_t));
    
    if(!cfg || !h) {
        return NULL;
    }

/*
    h->channel = getChannel(&cfg->pin);
    h->hdac.Instance = getADC(&cfg->pin);
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
*/
    return h;
    
fail:
    free(h);
    return NULL;
}


int dac_deinit(handle_t *h)
{
    dac_handle_t **dh=(dac_handle_t**)h;
    
    if(!dh || !(*dh)) {
        return -1;
    }
    
    HAL_DAC_DeInit(&(*dh)->hdac);
    free(*dh);
    //dma_deinit();
    
    return 0;
}


int dac_set(handle_t h, F32 v)
{
    U32 i;

    /*
    adc_data_t dat;
    ADC_ChannelConfTypeDef sc = {0};
    adc_handle_t *ah=(adc_handle_t*)h;
    
    sc.Channel = ah->channel;
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
    */
    
    return 0;
}




void HAL_DAC_MspInit(DAC_HandleTypeDef* adcHandle)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if(adcHandle->Instance==DAC1) {
        /* DAC1 clock enable */
        //__HAL_RCC_DAC1_CLK_ENABLE();

        //__HAL_RCC_GPIOC_CLK_ENABLE();
        //__HAL_RCC_GPIOA_CLK_ENABLE();

        //GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
        //GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        //HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        
        //dma_config();
        
        /* ADC1 interrupt Init */
        //HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
        //HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
    }
}

void HAL_DAC_MspDeInit(DAC_HandleTypeDef* adcHandle)
{
    if(adcHandle->Instance==DAC1) {
        /* Peripheral clock disable */
        //__HAL_RCC_DAC1_CLK_DISABLE();
        //HAL_GPIO_DeInit(GPIOA, GPIO_PIN_0|GPIO_PIN_1);
        
        //HAL_DMA_DeInit(adcHandle->DMA_Handle);

        /* ADC1 interrupt Deinit */
        //HAL_NVIC_DisableIRQ(DMA1_Channel1_IRQn);
    }
} 

