
#include "adc2.h"

#define VDD_APPLI                      ((uint32_t) 3300)    /* Value of analog voltage supply Vdda (unit: mV) */
#define RANGE_8BITS                    ((uint32_t)  255)    /* Max digital value with a full range of 8 bits */
#define RANGE_12BITS                   ((uint32_t) 4095)    /* Max digital value with a full range of 12 bits */
#define RANGE_16BITS                   ((uint32_t)65535)    /* Max digital value with a full range of 16 bits */

/* ADC parameters */
#define ADC_BUF_LEN                     (256)    /* Size of array containing ADC converted values */

#define COMPUTATION_DUALMODEINTERLEAVED_ADCMASTER_RESULT(DATA)                ((DATA) & 0x0000FFFF)
#define COMPUTATION_DUALMODEINTERLEAVED_ADCSLAVE_RESULT(DATA)                 ((DATA) >> 16)

ADC_HandleTypeDef    AdcHandle_master;
ADC_HandleTypeDef    AdcHandle_slave;


typedef struct {
    uint32_t ping[ADC_BUF_LEN+8];
    uint32_t pong[ADC_BUF_LEN+8];
}adc_buf_t;
adc_buf_t adcBuffer;


DMA_HandleTypeDef  DmaHandle;
static uint32_t *get_buffer(void)
{
    DMA_Stream_TypeDef *pInstanceDMA = (DMA_Stream_TypeDef *)DmaHandle.Instance;
    if((pInstanceDMA->CR & DMA_SxCR_CT) == RESET) {
        return adcBuffer.pong;
    }
    else {
        return adcBuffer.ping;
    }
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



/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);
static void Error_Handler(void)
{
    
}
static void ADC_Config(void);
static void CPU_CACHE_Enable(void);


/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */
void adc2_init(void)
{
  //CPU_CACHE_Enable();

  //HAL_Init();
  
  /* Configure the system clock to 400 MHz */
  //SystemClock_Config();
  
  ADC_Config();

  //if (HAL_ADCEx_MultiModeStart_DMA(&AdcHandle_master,(uint32_t *)pingBuffer, ADCCONVERTEDVALUES_BUFFER_SIZE) != HAL_OK)
  if (HAL_ADCEx_MultiBufferStart_DMA(&AdcHandle_master, adcBuffer.ping, adcBuffer.pong, ADC_BUF_LEN) != HAL_OK)
  {
    /* Start Error */
    Error_Handler();
  }
  SET_BIT(AdcHandle_master.Instance->CR, ADC_CR_ADSTART);
  
}


/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 400000000 (CPU Clock)
  *            HCLK(Hz)                       = 200000000 (AXI and AHBs Clock)
  *            AHB Prescaler                  = 2
  *            D1 APB3 Prescaler              = 2 (APB3 Clock  100MHz)
  *            D2 APB1 Prescaler              = 2 (APB1 Clock  100MHz)
  *            D2 APB2 Prescaler              = 2 (APB2 Clock  100MHz)
  *            D3 APB4 Prescaler              = 2 (APB4 Clock  100MHz)
  *            HSE Frequency(Hz)              = 25000000
  *            PLL_M                          = 5
  *            PLL_N                          = 160
  *            PLL_P                          = 2
  *            PLL_Q                          = 4
  *            PLL_R                          = 2
  *            VDD(V)                         = 3.3
  *            Flash Latency(WS)              = 4
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  HAL_StatusTypeDef ret = HAL_OK;
  
  /*!< Supply configuration update enable */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /* The voltage scaling allows optimizing the power consumption when the device is
     clocked below the maximum system frequency, to update the voltage scaling value
     regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
  
  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
  RCC_OscInitStruct.CSIState = RCC_CSI_OFF;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;

  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 160;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLQ = 4;

  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
  if(ret != HAL_OK)
  {
    Error_Handler();
  }
  
/* Select PLL as system clock source and configure  bus clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_D1PCLK1 | RCC_CLOCKTYPE_PCLK1 | \
                                 RCC_CLOCKTYPE_PCLK2  | RCC_CLOCKTYPE_D3PCLK1);

  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;  
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2; 
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2; 
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2; 
  ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);
  if(ret != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  ADC configuration
  * @param  None
  * @retval None
  */
static void ADC_Config(void)
{
  ADC_ChannelConfTypeDef   sConfig;
  ADC_MultiModeTypeDef     MultiModeInit;

  /* Configuration of ADC (master) init structure: ADC parameters and regular group */
  AdcHandle_master.Instance = ADC1;
  AdcHandle_slave.Instance = ADC2;

  if (HAL_ADC_DeInit(&AdcHandle_master) != HAL_OK)
  {
    /* ADC initialization error */
    Error_Handler();
  }
  
  if (HAL_ADC_DeInit(&AdcHandle_slave) != HAL_OK)
  {
    /* ADC initialization error */
    Error_Handler();
  }

  AdcHandle_master.Init.ClockPrescaler           = ADC_CLOCK_ASYNC_DIV2;           /* Asynchronous clock mode, input ADC clock divided by 2*/
  AdcHandle_master.Init.Resolution               = ADC_RESOLUTION_16B;              /* 16-bit resolution for converted data */
  AdcHandle_master.Init.ScanConvMode             = DISABLE;                         /* Sequencer disabled (ADC conversion on only 1 channel: channel set on rank 1) */
  AdcHandle_master.Init.EOCSelection             = ADC_EOC_SINGLE_CONV;             /* EOC flag picked-up to indicate conversion end */
  AdcHandle_master.Init.LowPowerAutoWait         = DISABLE;                         /* Auto-delayed conversion feature disabled */
  AdcHandle_master.Init.ContinuousConvMode       = ENABLE;                          /* Continuous mode to have maximum conversion speed (no delay between conversions) */

  AdcHandle_master.Init.NbrOfConversion          = 1;                               /* Parameter discarded because sequencer is disabled */
  AdcHandle_master.Init.DiscontinuousConvMode    = DISABLE;                         /* Parameter discarded because sequencer is disabled */
  AdcHandle_master.Init.NbrOfDiscConversion      = 1;                               /* Parameter discarded because sequencer is disabled */
  AdcHandle_master.Init.ExternalTrigConv         = ADC_SOFTWARE_START;              /* Software start to trigger the 1st conversion manually, without external event */
  AdcHandle_master.Init.ExternalTrigConvEdge     = ADC_EXTERNALTRIGCONVEDGE_NONE;   /* Parameter discarded because trigger of conversion by software start (no external event) */
  AdcHandle_master.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DMA_CIRCULAR; /* DMA circular mode selected */
  AdcHandle_master.Init.Overrun                  = ADC_OVR_DATA_OVERWRITTEN;        /* DR register is overwritten with the last conversion result in case of overrun */
  AdcHandle_master.Init.OversamplingMode         = DISABLE;                         /* No oversampling */
  
  AdcHandle_master.Init.Oversampling.Ratio	 = 1;
  AdcHandle_master.Init.Oversampling.RightBitShift  = ADC_RIGHTBITSHIFT_1;
  AdcHandle_master.Init.Oversampling.TriggeredMode = ADC_TRIGGEREDMODE_SINGLE_TRIGGER;         	/* Specifies whether or not a trigger is needed for each sample */
  AdcHandle_master.Init.Oversampling.OversamplingStopReset = ADC_TRIGGEREDMODE_SINGLE_TRIGGER; 
  
  if (HAL_ADC_Init(&AdcHandle_master) != HAL_OK)
  {
    /* ADC initialization error */
    Error_Handler();
  }

  /* Same configuration as ADC master, with continuous mode and external      */
  /* trigger disabled since ADC master is triggering the ADC slave            */
  /* conversions                                                              */
  AdcHandle_slave.Init = AdcHandle_master.Init;
  AdcHandle_slave.Init.ContinuousConvMode    = DISABLE;
  AdcHandle_slave.Init.ExternalTrigConv      = ADC_SOFTWARE_START;

  if (HAL_ADC_Init(&AdcHandle_slave) != HAL_OK)
  {
    /* ADC initialization error */
    Error_Handler();
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

  if (HAL_ADC_ConfigChannel(&AdcHandle_master, &sConfig) != HAL_OK)
  {
    /* Channel Configuration Error */
    Error_Handler();
  }

  /* Configuration of channel on ADC (slave) regular group on sequencer rank 1 */
  /* Same channel as ADCx for dual mode interleaved: both ADC are converting  */
  /* the same channel.                                                        */
  //sConfig.Channel = ADC_CHANNEL_18;
  
  if (HAL_ADC_ConfigChannel(&AdcHandle_slave, &sConfig) != HAL_OK)
  {
    /* Channel Configuration Error */
    Error_Handler();
  }
  
  /* Run the ADC calibration in single-ended mode */
  if (HAL_ADCEx_Calibration_Start(&AdcHandle_master, ADC_CALIB_OFFSET, ADC_DIFFERENTIAL_ENDED) != HAL_OK)
  {
    /* Calibration Error */
    Error_Handler();
  }

  if (HAL_ADCEx_Calibration_Start(&AdcHandle_slave, ADC_CALIB_OFFSET, ADC_DIFFERENTIAL_ENDED) != HAL_OK)
  {
    /* Calibration Error */
    Error_Handler();
  }

  /* Configuration of multimode */
  /* Multimode parameters settings and set ADCy (slave) under control of      */
  /* ADCx (master).                                                           */
  MultiModeInit.Mode = ADC_DUALMODE_INTERL;
  MultiModeInit.DualModeData = ADC_DUALMODEDATAFORMAT_32_10_BITS;  /* ADC and DMA configured in resolution 32 bits to match with both ADC master and slave resolution */
  MultiModeInit.TwoSamplingDelay = ADC_TWOSAMPLINGDELAY_1CYCLE;

  if (HAL_ADCEx_MultiModeConfigChannel(&AdcHandle_master, &MultiModeInit) != HAL_OK)
  {
    /* Multimode Configuration Error */
    Error_Handler();
  }
  
}


void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc)
{
  GPIO_InitTypeDef          GPIO_InitStruct;
  
  /*##-1- Enable peripherals and GPIO Clocks #################################*/
  /* Enable clock of GPIO associated to the peripheral channels */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  
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
    DmaHandle.Instance = DMA1_Stream1;

    DmaHandle.Init.Request             = DMA_REQUEST_ADC1;
    DmaHandle.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    DmaHandle.Init.PeriphInc           = DMA_PINC_DISABLE;
    DmaHandle.Init.MemInc              = DMA_MINC_ENABLE;
    DmaHandle.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;       /* Transfer from ADC by word to match with ADC configuration: Dual mode, ADC master contains conversion results on data register (32 bits) of ADC master and ADC slave  */
    DmaHandle.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;       /* Transfer to memory by word to match with buffer variable type: word */
    DmaHandle.Init.Mode                = DMA_CIRCULAR;              /* DMA in circular mode to match with ADC configuration: DMA continuous requests */
    DmaHandle.Init.Priority            = DMA_PRIORITY_HIGH;
  
   /* Deinitialize  & Initialize the DMA for new transfer */
    HAL_DMA_DeInit(&DmaHandle);
    HAL_DMA_Init(&DmaHandle);

    /* Associate the initialized DMA handle to the ADC handle */
    __HAL_LINKDMA(hadc, DMA_Handle, DmaHandle);
  
    /*##-4- Configure the NVIC #################################################*/

     /* NVIC configuration for DMA interrupt (transfer completion or error) */
    /* Priority: high-priority */
    HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);
  }
 
  /* NVIC configuration for ADC interrupt */
  /* Priority: high-priority */
  HAL_NVIC_SetPriority(ADC_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(ADC_IRQn);
  
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *AdcHandle)
{
    uint32_t *buf=get_buffer();
  
    SCB_InvalidateDCache_by_Addr(buf, ADC_BUFFER_SIZE*4);
  
    
}
void DMA1_Stream1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(AdcHandle_master.DMA_Handle);
}


static void CPU_CACHE_Enable(void)
{
    /* Enable I-Cache */
    SCB_EnableICache();

    /* Enable D-Cache */
    SCB_EnableDCache();
    
    SCB->CACR |= 1<<2;
}



