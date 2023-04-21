#include "dal_adc.h"
#include "cfg.h"
#include "platform.h"


//�ô���ɼ�����ֵֻ������ֵ��һ��


ADC_HandleTypeDef	AdcHandle_master;
ADC_HandleTypeDef	AdcHandle_slave;
DMA_HandleTypeDef   ISODMAHandle;


#define ADC_BUF_LEN 256
typedef struct {
    uint32_t ping[ADC_BUF_LEN+8];
    uint32_t pong[ADC_BUF_LEN+8];
}adc_buf_t;
adc_buf_t adcBuffer;
static U32 *get_buffer(void)
{
    DMA_Stream_TypeDef *pInstanceDMA = (DMA_Stream_TypeDef *)ISODMAHandle.Instance;
    if((pInstanceDMA->CR & DMA_SxCR_CT) == SET) {
        return adcBuffer.ping;
    }
    else {
        return adcBuffer.pong;
    }
}

INT8U dma_init(void)
{
	/*S1:?????DMA??*/
	ISODMAHandle.Instance                 = DMA2_Stream0;           	/* ???DMA1 Stream3 */
	ISODMAHandle.Init.Request             = DMA_REQUEST_ADC1;			/* ??????DMA_REQUEST_ADC1 */  
	ISODMAHandle.Init.Direction           = DMA_PERIPH_TO_MEMORY;   	/* ???????????? */  
	ISODMAHandle.Init.PeriphInc           = DMA_PINC_DISABLE;       	/* ???????? */ 
	ISODMAHandle.Init.MemInc              = DMA_MINC_ENABLE;        	/* ????????? */  
	ISODMAHandle.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;  		/* ???????????,?32bit */     
	ISODMAHandle.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;  		/* ????????????,?32bit */    
	ISODMAHandle.Init.Mode                = DMA_CIRCULAR;           	/* ???? */   
	ISODMAHandle.Init.Priority            = DMA_PRIORITY_LOW;        	/* ???? */  
	ISODMAHandle.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;    	/* ??FIFO*/
	ISODMAHandle.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL; 	/* ??FIFO??????,?????? */
	ISODMAHandle.Init.MemBurst            = DMA_MBURST_SINGLE;       	/* ??FIFO??????,??????? */
	ISODMAHandle.Init.PeriphBurst         = DMA_PBURST_SINGLE;       	/* ??FIFO??????,?????? */
	
	/*S2:???DMA*/
	if (HAL_DMA_DeInit(&ISODMAHandle) != HAL_OK)
	{
		return FALSE;
	}
    if (HAL_DMA_Init(&ISODMAHandle) != HAL_OK)
    {
		return FALSE;
    }
    
    /*S3:??DMA1 Stream3???*/
    HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 8, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
    
    /*S4:??ADC???DMA??*/
	__HAL_LINKDMA(&AdcHandle_master, DMA_Handle, ISODMAHandle);
	
	return TRUE;
}


static void io_init(void)
{
	/*S1:ʹ������ʱ��*/
    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_DMA2_CLK_ENABLE();
	__HAL_RCC_ADC12_CLK_ENABLE();
    
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	
	/*S2:����ADC����ʹ�õ�����*/	
	GPIO_InitTypeDef	GPIO_InitStruct = {0};
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	
	// CH1
	GPIO_InitStruct.Pin = GPIO_PIN_4 | GPIO_PIN_5; // PA4 PA5
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	
	// CH2
	GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1; // PB0 PB1
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	
	/*S3:����ADC������ʱ��*/
	/* ����PLL3Rʱ��Ϊ20.48MHz�������Ƶ����ADCʱ��2.56MHz */
	/* ��SystemClock_Config()��������� */
}



static int do_calibration(void)
{
	/*S1:��ʼ��ADC1��ADC2*/	
	AdcHandle_master.Instance = ADC1;
	AdcHandle_slave.Instance  = ADC2;

	AdcHandle_master.Init.ClockPrescaler        = ADC_CLOCK_ASYNC_DIV1;          		/* ����PLL�첽ʱ�ӣ�8��Ƶ��Y�汾, ��20.56MHz/8=2.56MHz */
	AdcHandle_master.Init.Resolution            = ADC_RESOLUTION_16B;            		/* 16λ�ֱ��� */
	AdcHandle_master.Init.ScanConvMode          = ADC_SCAN_DISABLE;              		/* ��ֹɨ��*/
	AdcHandle_master.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;           		/* EOCת��������־ */
	AdcHandle_master.Init.LowPowerAutoWait      = DISABLE;                       		/* ��ֹ�͹����Զ��ӳ����� */
	AdcHandle_master.Init.ContinuousConvMode    = ENABLE;                       		/* ʹ������ת�� */
	AdcHandle_master.Init.NbrOfConversion       = 1;                             		/* ʹ��1��ת��ͨ�� */
	AdcHandle_master.Init.DiscontinuousConvMode = DISABLE;                       		/* ��ֹ������ģʽ */
	AdcHandle_master.Init.NbrOfDiscConversion   = 1;                             		/* ��ֹ������ģʽ�󣬴˲������ԣ���λ���������ò�����������ͨ���� */
	AdcHandle_master.Init.ExternalTrigConv      = ADC_SOFTWARE_START;            		/* ������� */
	AdcHandle_master.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;    	/* �޴��� */
	AdcHandle_master.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DMA_CIRCULAR; 	/* DMAѭ��ģʽ����ADCת�������� */
	AdcHandle_master.Init.Overrun                  = ADC_OVR_DATA_OVERWRITTEN;       	/* ADCת������Ļ�������ADC�����ݼĴ��� */
	AdcHandle_master.Init.OversamplingMode         			 = ENABLE;                	/* Oversampling enabled */
	AdcHandle_master.Init.Oversampling.Ratio                 = 1023;    	            /* Oversampling ratio */
	AdcHandle_master.Init.Oversampling.RightBitShift         = ADC_RIGHTBITSHIFT_10;         	/* Right shift of the oversampled summation */
    AdcHandle_master.Init.Oversampling.TriggeredMode         = ADC_TRIGGEREDMODE_SINGLE_TRIGGER;         	/* Specifies whether or not a trigger is needed for each sample */
	AdcHandle_master.Init.Oversampling.OversamplingStopReset = ADC_REGOVERSAMPLING_CONTINUED_MODE; 	/* Specifies whether or not the oversampling buffer is maintained during injection sequence */    
    
	/* Configuration of ADC (slave) init structure */
	AdcHandle_slave.Init = AdcHandle_master.Init;
    
	if (HAL_ADC_DeInit(&AdcHandle_master) != HAL_OK)
	{
		return -1;
	}
	if (HAL_ADC_Init(&AdcHandle_master) != HAL_OK)
	{
		return -1;
	}
	
	if (HAL_ADC_DeInit(&AdcHandle_slave) != HAL_OK)
	{
		return -1;
	}
	if (HAL_ADC_Init(&AdcHandle_slave) != HAL_OK)
	{
		return -1;
	}
    
	/*S4:У׼ADC������ƫ��У׼*/
	if (HAL_ADCEx_Calibration_Start(&AdcHandle_master, ADC_CALIB_OFFSET_LINEARITY, ADC_SIGNAL_MODE) != HAL_OK)
	{
		return -1;
	}
	if (HAL_ADCEx_Calibration_Start(&AdcHandle_slave, ADC_CALIB_OFFSET_LINEARITY, ADC_SIGNAL_MODE) != HAL_OK)
	{
		return -1;
	}
	
    return 0;
}


static int do_config(void)
{
	ADC_MultiModeTypeDef     MultiModeInit = {0};
	ADC_ChannelConfTypeDef   sConfig = {0};
	
	/*S1:��ʼ��ADC1��ADC2*/	
	AdcHandle_master.Instance = ADC1;
	AdcHandle_slave.Instance = ADC2;
	
	/*S2:��ʼ��ADC */
//YXB dbg
	AdcHandle_master.Init.ClockPrescaler        = ADC_CLOCK_ASYNC_DIV1;          		/* ����PLL�첽ʱ�ӣ�1��Ƶ��Y�汾, ��20.48MHz/8=2.56MHz */
	AdcHandle_master.Init.Resolution            = ADC_RESOLUTION_16B;            		/* 16λ�ֱ��� */
	AdcHandle_master.Init.ScanConvMode          = ADC_SCAN_DISABLE;              		/* ��ֹɨ��*/
	AdcHandle_master.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;           		/* EOCת��������־ */
	AdcHandle_master.Init.LowPowerAutoWait      = DISABLE;                       		/* ��ֹ�͹����Զ��ӳ����� */
	AdcHandle_master.Init.ContinuousConvMode    = ENABLE;                       		/* ʹ������ת�� */
	AdcHandle_master.Init.NbrOfConversion       = 1;                             		/* ʹ��1��ת��ͨ�� */
	AdcHandle_master.Init.DiscontinuousConvMode = DISABLE;                       		/* ��ֹ������ģʽ */
	AdcHandle_master.Init.NbrOfDiscConversion   = 0;                             		/* ��ֹ������ģʽ�󣬴˲������ԣ���λ���������ò�����������ͨ���� */
	AdcHandle_master.Init.ExternalTrigConv      = ADC_SOFTWARE_START;            		/* ������� */
	AdcHandle_master.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;    	/* �޴��� */
	AdcHandle_master.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DMA_CIRCULAR; 	/* DMAѭ��ģʽ����ADCת�������� */
	AdcHandle_master.Init.Overrun                  = ADC_OVR_DATA_OVERWRITTEN;       	/* ADCת������Ļ�������ADC�����ݼĴ��� */
	AdcHandle_master.Init.OversamplingMode         = ENABLE;                	        /* Oversampling enabled */

    AdcHandle_master.Init.Oversampling.Ratio	 = 1;
    AdcHandle_master.Init.Oversampling.RightBitShift  = ADC_RIGHTBITSHIFT_1;
	
	AdcHandle_master.Init.Oversampling.TriggeredMode = ADC_TRIGGEREDMODE_SINGLE_TRIGGER;         	/* Specifies whether or not a trigger is needed for each sample */
	AdcHandle_master.Init.Oversampling.OversamplingStopReset = ADC_REGOVERSAMPLING_CONTINUED_MODE; 	/* Specifies whether or not the oversampling buffer is maintained during injection sequence */    

	/* Configuration of ADC (slave) init structure */
	AdcHandle_slave.Init = AdcHandle_master.Init;
    AdcHandle_slave.Init.ContinuousConvMode = DISABLE;

	if (HAL_ADC_DeInit(&AdcHandle_master) != HAL_OK)
	{
		return FALSE;
	}
	if (HAL_ADC_Init(&AdcHandle_master) != HAL_OK)
	{
		return FALSE;
	}
	
	if (HAL_ADC_DeInit(&AdcHandle_slave) != HAL_OK)
	{
		return FALSE;
	}
	if (HAL_ADC_Init(&AdcHandle_slave) != HAL_OK)
	{
		return FALSE;
	}
    
	/*S2:����ADC1/2ͨ��*/
	sConfig.Rank         = ADC_REGULAR_RANK_1;          /* ����������ĵ�1�� */
	sConfig.SingleDiff   = ADC_SIGNAL_MODE;      		/* ������� */
	sConfig.OffsetNumber = ADC_OFFSET_NONE;             /* ��ƫ�� */ 
	sConfig.Offset = 0;                                 /* ��ƫ�Ƶ�����£��˲������� */
	sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLE_5;

	sConfig.Channel      = ADC_CHANNEL_18;              /* ����ʹ�õ�ADCͨ�� */
	if (HAL_ADC_ConfigChannel(&AdcHandle_master, &sConfig) != HAL_OK)
	{
		return FALSE;
	}
	
	sConfig.Channel      = ADC_CHANNEL_5;               /* ����ʹ�õ�ADCͨ�� */
	if (HAL_ADC_ConfigChannel(&AdcHandle_slave, &sConfig) != HAL_OK)
	{
		return FALSE;
	}
	
	/*S4:У׼ADC������ƫ��У׼*/
	if (HAL_ADCEx_Calibration_Start(&AdcHandle_master, ADC_CALIB_OFFSET, ADC_SIGNAL_MODE) != HAL_OK)
	{
		return FALSE;
	}
	if (HAL_ADCEx_Calibration_Start(&AdcHandle_slave, ADC_CALIB_OFFSET, ADC_SIGNAL_MODE) != HAL_OK)
	{
		return FALSE;
	}
	
	/*S5:����ADC1��ADC2Ϊ˫��ģʽ*/
	/* Multimode parameters settings and set ADCy (slave) under control of ADCx (master) */
	MultiModeInit.Mode = ADC_DUALMODE_REGSIMULT;
	MultiModeInit.DualModeData = ADC_DUALMODEDATAFORMAT_32_10_BITS;  /* ADC and DMA configured in resolution 32 bits to match with both ADC master and slave resolution */
	MultiModeInit.TwoSamplingDelay = ADC_TWOSAMPLINGDELAY_1CYCLE;
    
	if (HAL_ADCEx_MultiModeConfigChannel(&AdcHandle_master, &MultiModeInit) != HAL_OK)
	{	
		return FALSE;
	}
	
	return TRUE;
}


HAL_StatusTypeDef HAL_ADCEx_MultiBufferStart_DMA(ADC_HandleTypeDef *hadc, PINT32U pPingData, PINT32U pPongData, INT32U Length)
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
			// ʹ��DMA˫���巽ʽ��PingPong�������ص�������Ӧͬһ��������
			hadc->DMA_Handle->XferM1CpltCallback = ADC_DMAConvCplt;

			/* Set the DMA half transfer complete callback */
			hadc->DMA_Handle->XferHalfCpltCallback = NULL; // ������DMA�봫������ж�

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
			HAL_DMAEx_MultiBufferStart_IT(hadc->DMA_Handle, (INT32U)&tmpADC_Common->CDR, (INT32U)pPingData, (INT32U)pPongData, Length);

			/* Enable conversion of regular group.                                    */
			/* Process unlocked */
			__HAL_UNLOCK(hadc);
			/* If software start has been selected, conversion starts immediately.    */
			/* If external trigger has been selected, conversion will start at next   */
			/* trigger event.                                                         */
			SET_BIT(hadc->Instance->CR, ADC_CR_ADSTART); // ������ADCת��
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



void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    int i;
    U32 *buf=get_buffer();
    
    SCB_InvalidateDCache_by_Addr(buf, ADC_BUF_LEN*4);
    
    //adc_conv(&adcHandle, buf);
    
    //for(i=0; i<ADC_MAX; i++) {
    //    if(adc_in_use(i)) {
    //        adc_conv(&adcHandle, buf);
    //    }
    //}
}
void DMA2_Stream0_IRQHandler(void)  //ADC1
{
    HAL_DMA_IRQHandler(&ISODMAHandle);
}


//#include "drv/delay.h"
int dal_adc_init(void)
{
    io_init();
    do_calibration();
    dma_init();
    do_config();
    
    adc_start();
}

int dal_adc_start(void)
{
    HAL_StatusTypeDef st;
    
	//HAL_ADCEx_RegularMultiModeStop_DMA(&AdcHandle_master);
    st = HAL_ADCEx_MultiBufferStart_DMA(&AdcHandle_master, adcBuffer.ping, adcBuffer.pong, ADC_BUF_LEN);
    st = 1;
    
    return 0;
}

int dal_adc_config(adc_cfg_t *cfg)
{
    return 0;
}

int dal_adc_stop(void)
{
    HAL_ADCEx_RegularMultiModeStop_DMA(&AdcHandle_master);
    return 0;
}
