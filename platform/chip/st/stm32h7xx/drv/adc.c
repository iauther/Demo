#include "drv/adc.h"

//http://t.zoukankan.com/armfly-p-12180331.html


ADC_HandleTypeDef hadc1;


#define BUF_LEN   1000

typedef struct {
    ADC_HandleTypeDef hadc;
    DMA_HandleTypeDef hdma;
}adc_t;

typedef struct {
    adc_t   adc1_2;
    adc_t   adc3;
    
    U16 adcBuf[BUF_LEN];
    F32 volBuf[BUF_LEN];
}adc_handle_t;

#define S_ADCTO(v)    ((3.3F/65536)*v)
#define D_ADCTO(v)    (((2.0F*v)/65536-1)*3.3F)

static void adc_conv(adc_handle_t *h)
{
    int i;
    
    for(i=0; i<BUF_LEN; i++) {
        h->volBuf[i] = D_ADCTO(h->adcBuf[i]);
    }
}


static adc_handle_t adcHandle={0};

static int adc_dma_init(adc_t *adc)
{
    //__HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_DMA2_CLK_ENABLE();

    adc->hdma.Instance = DMA2_Stream0;
    adc->hdma.Init.Request = DMA_REQUEST_ADC1;
    adc->hdma.Init.Direction = DMA_PERIPH_TO_MEMORY;
    adc->hdma.Init.PeriphInc = DMA_PINC_DISABLE;
    adc->hdma.Init.MemInc = DMA_MINC_ENABLE;
    adc->hdma.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    adc->hdma.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    adc->hdma.Init.Mode = DMA_CIRCULAR;
    adc->hdma.Init.Priority = DMA_PRIORITY_LOW;
    adc->hdma.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    
    if (HAL_DMA_DeInit(&adc->hdma) != HAL_OK){
		return -1;
	}
    
    if (HAL_DMA_Init(&adc->hdma) != HAL_OK) {
        return -1;
    }
    
    HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
    
    __HAL_LINKDMA(&adc->hadc, DMA_Handle, adc->hdma);
    
    return 0;
}
static int adc_clk_set(void)
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


static int adc_ctl_init(adc_t *adc)
{
    ADC_MultiModeTypeDef mInit = {0};
    ADC_ChannelConfTypeDef sConfig = {0};
    
    adc->hadc.Instance = ADC1;
    //adc->hadc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV1;     //AHB clock
    adc->hadc.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV4;           //ADC_CLOCK_SYNC_PCLK_DIV1
    adc->hadc.Init.Resolution = ADC_RESOLUTION_16B;
    adc->hadc.Init.ScanConvMode = ADC_SCAN_ENABLE;
    adc->hadc.Init.EOCSelection = ADC_EOC_SEQ_CONV;
    adc->hadc.Init.LowPowerAutoWait = DISABLE;
    adc->hadc.Init.ContinuousConvMode = ENABLE;
    adc->hadc.Init.NbrOfConversion = 1;
    adc->hadc.Init.DiscontinuousConvMode = DISABLE;
    adc->hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    adc->hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    adc->hadc.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DMA_CIRCULAR;
    adc->hadc.Init.Overrun = ADC_OVR_DATA_PRESERVED;
    adc->hadc.Init.LeftBitShift = ADC_LEFTBITSHIFT_NONE;
    adc->hadc.Init.OversamplingMode = DISABLE;
    if (HAL_ADC_Init(&adc->hadc) != HAL_OK) {
        return -1;
    }

#if 0
    // Configure the ADC multi-mode
    mInit.Mode = ADC_MODE_INDEPENDENT;      //ADC_DUALMODE_REGSIMULT
	mInit.DualModeData = ADC_DUALMODEDATAFORMAT_32_10_BITS;  /* ADC and DMA configured in resolution 32 bits to match with both ADC master and slave resolution */
	mInit.TwoSamplingDelay = ADC_TWOSAMPLINGDELAY_1CYCLE;
    if (HAL_ADCEx_MultiModeConfigChannel(&adc->hadc, &multimode) != HAL_OK) {
        return -1;
    }
#endif

    //Configure Regular Channel
    sConfig.Channel = ADC_CHANNEL_18;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
    sConfig.SingleDiff = ADC_DIFFERENTIAL_ENDED;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0;
    sConfig.OffsetSignedSaturation = DISABLE;
    if (HAL_ADC_ConfigChannel(&adc->hadc, &sConfig) != HAL_OK) {
        return -1;
    }
    
    __HAL_ADC_ENABLE_IT(&hadc1, ADC_IT_EOS);
    //HAL_NVIC_SetPriority(ADC_IRQn, 0, 0);
    //HAL_NVIC_EnableIRQ(ADC_IRQn);
    
    //__HAL_ADC_CLEAR_FLAG(&hadc1, (ADC_FLAG_EOC | ADC_FLAG_EOS | ADC_FLAG_OVR));
	//__HAL_ADC_ENABLE_IT(&hadc1, ADC_IT_OVR);
    
    //HAL_ADCEx_Calibration_Start(&adc->hadc, ADC_CALIB_OFFSET, ADC_DIFFERENTIAL_ENDED);
    
    return 0;
}





void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
{
    GPIO_InitTypeDef init = {0};
    
    if(hadc->Instance==ADC1) {
        /* Peripheral clock enable */
        __HAL_RCC_ADC12_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        
        init.Pin = GPIO_PIN_4|GPIO_PIN_5;
        init.Mode = GPIO_MODE_ANALOG;
        init.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &init); 
    }
}


void HAL_ADC_MspDeInit(ADC_HandleTypeDef* hadc)
{
    if(hadc->Instance==ADC1) {
        /* Peripheral clock disable */
        __HAL_RCC_ADC12_CLK_DISABLE();

        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_4|GPIO_PIN_5);

        /* ADC1 DMA DeInit */
        HAL_DMA_DeInit(hadc->DMA_Handle);
    }

}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    adc_conv(&adcHandle);
}

void ADC_IRQHandler(void)
{
    HAL_ADC_IRQHandler(&adcHandle.adc1_2.hadc);
}

void DMA2_Stream0_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&adcHandle.adc1_2.hdma);
}
void BDMA_Channel0_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&adcHandle.adc3.hdma);
}

int adc_init(gpio_pin_t *pin)
{
    adc_clk_set();
    
    adc_dma_init(&adcHandle.adc1_2);
    adc_ctl_init(&adcHandle.adc1_2);
    
    adc_start();
    
    return 0;
}



int adc_start(void)
{
    HAL_ADC_Start_DMA(&adcHandle.adc1_2.hadc, (U32*)adcHandle.adcBuf, BUF_LEN);
    
    return 0;
}

int adc_stop(void)
{
    HAL_ADC_Stop_DMA(&adcHandle.adc1_2.hadc);
    
    return 0;
}


int adc_get(U32 *adc)
{
    
    return 0;
}












