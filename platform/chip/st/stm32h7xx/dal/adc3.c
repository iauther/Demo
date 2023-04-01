#include "dal/adc.h"


//该代码采集到的值ok



#define VDD_APPLI                      ((uint32_t) 3300)    /* Value of analog voltage supply Vdda (unit: mV) */
#define RANGE_8BITS                    ((uint32_t)  255)    /* Max digital value with a full range of 8 bits */
#define RANGE_12BITS                   ((uint32_t) 4095)    /* Max digital value with a full range of 12 bits */
#define RANGE_16BITS                   ((uint32_t)65535)    /* Max digital value with a full range of 16 bits */

/* ADC parameters */
#define ADC_BUF_LEN                     (256)    /* Size of array containing ADC converted values */

#define COMPUTATION_DUALMODEINTERLEAVED_ADCMASTER_RESULT(DATA)                ((DATA) & 0x0000FFFF)
#define COMPUTATION_DUALMODEINTERLEAVED_ADCSLAVE_RESULT(DATA)                 ((DATA) >> 16)

typedef struct {
    U32     *pBuf;
    U32     *pAlnBuf;
    U32     bufLen;
}buffer_t;



typedef struct {
    U8                  dual;
    ADC_HandleTypeDef   master;
    ADC_HandleTypeDef   slave;
    
    DMA_HandleTypeDef   hdma;
    TIM_HandleTypeDef   htim;
    
    U32                 *pBuf;
    U32                 *pAlnBuf;
    U32                 bufLen;
    
    buffer_t            ping;
    buffer_t            pong;
    
    
    U8                  mode;
    adc_callback_t      callback;
    U32                 value[16];
    U8                  cnt;
    U8                  chns;
}adc_handle_t;


static adc_handle_t *adcHandle=NULL;





static U32 *get_buffer(int *len)
{
    adc_handle_t *h=adcHandle;
    DMA_Stream_TypeDef *pStream;
    
    if(!h) {
        return NULL;
    }
    
    pStream = (DMA_Stream_TypeDef *)h->hdma.Instance;
    if((pStream->CR & DMA_SxCR_CT) == RESET) {
        if(len)  *len = h->pong.bufLen;
        return h->pong.pAlnBuf;
    }
    else {
        if(len)  *len = h->ping.bufLen;
        return h->ping.pAlnBuf;
    }
}
HAL_StatusTypeDef HAL_ADCEx_MultiBufferStart_DMA(ADC_HandleTypeDef *hadc, U32* pPingData, U32* pPongData, U32 cnt)
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
			HAL_DMAEx_MultiBufferStart_IT(hadc->DMA_Handle, (uint32_t)&tmpADC_Common->CDR, (uint32_t)pPingData, (uint32_t)pPongData, cnt);

			/* Enable conversion of regular group.                                    */
			/* Process unlocked */
			__HAL_UNLOCK(hadc);
			/* If software start has been selected, conversion starts immediately.    */
			/* If external trigger has been selected, conversion will start at next   */
			/* trigger event.                                                         */
			SET_BIT(hadc->Instance->CR, ADC_CR_ADSTART); // ???ADC??
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





////////////////////////////////////////////////////////////////////////////////
int adc_init(void)
{
    adc_handle_t *h=calloc(1, sizeof(adc_handle_t));
    if(!h) {
        return -1;
    }
    
    adcHandle = h;
    
    return 0;

}

int adc_config(adc_cfg_t *cfg)
{
    int bufLen;
    adc_handle_t *h=adcHandle;
    ADC_ChannelConfTypeDef   sConfig;
    ADC_MultiModeTypeDef     MultiModeInit;

    if(!h) {
        return -1;
    }
    
    h->mode = cfg->mode;
    h->dual = cfg->dual;
    h->callback = cfg->callback;
    
    if(h->dual) {
        bufLen = cfg->samples*sizeof(U32);          //  * h->chns?
    }
    else {
        h->bufLen = cfg->samples*sizeof(U16);       //  * h->chns?
    }
    
    h->ping.pBuf = (U32*)malloc(bufLen+32);
    h->ping.pAlnBuf = (U32*)ALIGN_TO(h->ping.pBuf, 32);
    h->ping.bufLen = bufLen;
    
    h->pong.pBuf = (U32*)malloc(bufLen+32);
    h->pong.pAlnBuf = (U32*)ALIGN_TO(h->pong.pBuf, 32);
    h->pong.bufLen = bufLen;
    
    /* Configuration of ADC (master) init structure: ADC parameters and regular group */
    h->master.Instance = ADC1;
    h->slave.Instance  = ADC2;

    if (HAL_ADC_DeInit(&h->master) != HAL_OK) {
        return -1;
    }

    if (HAL_ADC_DeInit(&h->slave) != HAL_OK) {
        return -1;
    }

    h->master.Init.ClockPrescaler           = ADC_CLOCK_ASYNC_DIV2;           /* Asynchronous clock mode, input ADC clock divided by 2*/
    h->master.Init.Resolution               = ADC_RESOLUTION_16B;              /* 16-bit resolution for converted data */
    h->master.Init.ScanConvMode             = DISABLE;                         /* Sequencer disabled (ADC conversion on only 1 channel: channel set on rank 1) */
    h->master.Init.EOCSelection             = ADC_EOC_SINGLE_CONV;             /* EOC flag picked-up to indicate conversion end */
    h->master.Init.LowPowerAutoWait         = DISABLE;                         /* Auto-delayed conversion feature disabled */
    h->master.Init.ContinuousConvMode       = ENABLE;                          /* Continuous mode to have maximum conversion speed (no delay between conversions) */

    h->master.Init.NbrOfConversion          = 1;                               /* Parameter discarded because sequencer is disabled */
    h->master.Init.DiscontinuousConvMode    = DISABLE;                         /* Parameter discarded because sequencer is disabled */
    h->master.Init.NbrOfDiscConversion      = 1;                               /* Parameter discarded because sequencer is disabled */
    h->master.Init.ExternalTrigConv         = ADC_SOFTWARE_START;              /* Software start to trigger the 1st conversion manually, without external event */
    h->master.Init.ExternalTrigConvEdge     = ADC_EXTERNALTRIGCONVEDGE_NONE;   /* Parameter discarded because trigger of conversion by software start (no external event) */
    h->master.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DMA_CIRCULAR; /* DMA circular mode selected */
    h->master.Init.Overrun                  = ADC_OVR_DATA_OVERWRITTEN;        /* DR register is overwritten with the last conversion result in case of overrun */
    h->master.Init.OversamplingMode         = DISABLE;                         /* No oversampling */

    h->master.Init.Oversampling.Ratio	 = 1;
    h->master.Init.Oversampling.RightBitShift  = ADC_RIGHTBITSHIFT_1;
    h->master.Init.Oversampling.TriggeredMode = ADC_TRIGGEREDMODE_SINGLE_TRIGGER;         	/* Specifies whether or not a trigger is needed for each sample */
    h->master.Init.Oversampling.OversamplingStopReset = ADC_TRIGGEREDMODE_SINGLE_TRIGGER; 

    if (HAL_ADC_Init(&h->master) != HAL_OK) {
        return -1;
    }

    /* Same configuration as ADC master, with continuous mode and external      */
    /* trigger disabled since ADC master is triggering the ADC slave            */
    /* conversions                                                              */
    h->slave.Init = h->master.Init;
    h->slave.Init.ContinuousConvMode    = DISABLE;
    h->slave.Init.ExternalTrigConv      = ADC_SOFTWARE_START;

    if (HAL_ADC_Init(&h->slave) != HAL_OK) {
        return -1;
    }


    /* Configuration of channel on ADC (master) regular group on sequencer rank 1 */
    /* Note: Considering IT occurring after each number of                      */
    /*       "ADCCONVERTEDVALUES_BUFFER_SIZE" ADC conversions (IT by DMA end    */
    /*       of transfer), select sampling time and ADC clock with sufficient   */
    /*       duration to not create an overhead situation in IRQHandler.        */
    sConfig.Channel      = ADC_CHANNEL_18;                /* Sampled channel number */
    sConfig.Rank         = ADC_REGULAR_RANK_1;          /* Rank of sampled channel number ADCx_CHANNEL */
    sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;    /* Minimum sampling time */
    sConfig.SingleDiff   = ADC_DIFFERENTIAL_ENDED;            /* Single-ended input channel */
    sConfig.OffsetNumber = ADC_OFFSET_NONE;             /* No offset subtraction */ 
    sConfig.Offset = 0;                                 /* Parameter discarded because offset correction is disabled */

    if (HAL_ADC_ConfigChannel(&h->master, &sConfig) != HAL_OK) {
        return -1;
    }

    /* Configuration of channel on ADC (slave) regular group on sequencer rank 1 */
    /* Same channel as ADCx for dual mode interleaved: both ADC are converting  */
    /* the same channel.                                                        */
    //sConfig.Channel = ADC_CHANNEL_18;

    if (HAL_ADC_ConfigChannel(&h->slave, &sConfig) != HAL_OK) {
        return -1;
    }

    /* Run the ADC calibration in single-ended mode */
    if (HAL_ADCEx_Calibration_Start(&h->master, ADC_CALIB_OFFSET, ADC_DIFFERENTIAL_ENDED) != HAL_OK) {
        return -1;
    }

    if (HAL_ADCEx_Calibration_Start(&h->slave, ADC_CALIB_OFFSET, ADC_DIFFERENTIAL_ENDED) != HAL_OK) {
        return -1;
    }

    /* Configuration of multimode */
    /* Multimode parameters settings and set ADCy (slave) under control of      */
    /* ADCx (master).                                                           */
    MultiModeInit.Mode = ADC_DUALMODE_INTERL;
    MultiModeInit.DualModeData = ADC_DUALMODEDATAFORMAT_32_10_BITS;  /* ADC and DMA configured in resolution 32 bits to match with both ADC master and slave resolution */
    MultiModeInit.TwoSamplingDelay = ADC_TWOSAMPLINGDELAY_1CYCLE;

    if (HAL_ADCEx_MultiModeConfigChannel(&h->master, &MultiModeInit) != HAL_OK) {
        /* Multimode Configuration Error */
        return -1;
    }
    
    return 0;
}



void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc)
{
    adc_handle_t *h=adcHandle;
    GPIO_InitTypeDef          GPIO_InitStruct;
  
    if(!h) {
        return;
    }
    
  /*##-1- Enable peripherals and GPIO Clocks #################################*/
  /* Enable clock of GPIO associated to the peripheral channels */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  
  /* Enable clock of ADCx peripheral */
  __HAL_RCC_ADC12_CLK_ENABLE();
    
  /* ADC Periph interface clock configuration */
  __HAL_RCC_ADC_CONFIG(RCC_ADCCLKSOURCE_CLKP);
  

  if (hadc->Instance == ADC1)
  {  
    /* Enable clock of DMA associated to the peripheral */
    __HAL_RCC_DMA1_CLK_ENABLE();

    /*##-2- Configure peripheral GPIO ##########################################*/ 
    /* Configure GPIO pins of the selected ADC channels */
    GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    /* Note: ADC slave does not need additional configuration, since it shares  */
    /*       the same clock domain, same GPIO pins (interleaved on the same     */
    /*       channel) and same DMA as ADC master.                               */
  
    /*##-3- Configure the DMA ##################################################*/
    /* Configure DMA parameters (ADC master) */
    h->hdma.Instance = DMA1_Stream1;

    h->hdma.Init.Request             = DMA_REQUEST_ADC1;
    h->hdma.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    h->hdma.Init.PeriphInc           = DMA_PINC_DISABLE;
    h->hdma.Init.MemInc              = DMA_MINC_ENABLE;
    h->hdma.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;       /* Transfer from ADC by word to match with ADC configuration: Dual mode, ADC master contains conversion results on data register (32 bits) of ADC master and ADC slave  */
    h->hdma.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;       /* Transfer to memory by word to match with buffer variable type: word */
    h->hdma.Init.Mode                = DMA_CIRCULAR;              /* DMA in circular mode to match with ADC configuration: DMA continuous requests */
    h->hdma.Init.Priority            = DMA_PRIORITY_HIGH;
  
   /* Deinitialize  & Initialize the DMA for new transfer */
    HAL_DMA_DeInit(&h->hdma);
    HAL_DMA_Init(&h->hdma);

    /* Associate the initialized DMA handle to the ADC handle */
    __HAL_LINKDMA(hadc, DMA_Handle, h->hdma);
  
    /*##-4- Configure the NVIC #################################################*/

     /* NVIC configuration for DMA interrupt (transfer completion or error) */
    /* Priority: high-priority */
    HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, 8, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);
  }
 
  /* NVIC configuration for ADC interrupt */
  /* Priority: high-priority */
  //HAL_NVIC_SetPriority(ADC_IRQn, 8, 0);
  //HAL_NVIC_EnableIRQ(ADC_IRQn);
  
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *AdcHandle)
{
    int len=0;
    adc_handle_t *h=adcHandle;
    U32 *buf=get_buffer(&len);
  
    SCB_InvalidateDCache_by_Addr(buf, ADC_BUF_LEN*4);
    
    if(h && h->callback) {
        if(h->mode==MODE_DMA) {
           h->callback((U8*)buf, len);
        }
    }
  
    
}
void DMA1_Stream1_IRQHandler(void)
{
    adc_handle_t *h=adcHandle;
    
    if(h) {
        HAL_DMA_IRQHandler(h->master.DMA_Handle);
    }
}


int adc_start(void)
{
    adc_handle_t *h=adcHandle;

    if(!h) {
        return -1;
    }
    
    if (HAL_ADCEx_MultiBufferStart_DMA(&h->master, h->ping.pAlnBuf, h->pong.pAlnBuf, h->ping.bufLen/4) != HAL_OK) {
        return -1;
    }
  
    return 0;
}








int adc_stop(void)
{
    return 0;
}



