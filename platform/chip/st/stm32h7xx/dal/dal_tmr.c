#include "dal_tmr.h"

static dal_tmr_cfg_t tmrCfg;
static TIM_HandleTypeDef TIM2_Handler;
static TIM_HandleTypeDef TIM8_Handler;

/*
������ת�ٷ�Ƶʱ���ö�ʱ�����ú���
@param	pPingData 	ת��DMA Ping Buf
@param	pPongData 	ת��DMA Pong Buf
@param	SpdDatNum 	һ���Ի�ȡ��ͨ�����ݸ���
@param	u32Period	ת�ٷ�Ƶֵ-1
*/
static int tmr_cfg_div(U32* buf, U16 cnt, U32 u32Period)
{
	TIM_ClockConfigTypeDef sClockSourceConfig_TIM2 = {0};
	TIM_ClockConfigTypeDef sClockSourceConfig_TIM8 = {0};	
	TIM_MasterConfigTypeDef sMasterConfig = {0};
	TIM_SlaveConfigTypeDef sSlaveConfig = {0};
	TIM_IC_InitTypeDef TIM2_IC1Config = {0};
	
	/*S1:���ö�ʱ��TIM8 ETR����*/
	GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_0;            		// PA0 PIN40
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;      		// ��������
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;        		// ����
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; 		// ����
    GPIO_InitStruct.Alternate = GPIO_AF3_TIM8;   		// PA0����ΪTIM8
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*S2:���ö�ʱ��TIM8*/
	TIM8_Handler.Instance = TIM8;					// ͨ�ö�ʱ��8
	TIM8_Handler.Init.Prescaler = 0;				// ���Զ�����ת���ٷ�Ƶ
	TIM8_Handler.Init.CounterMode = TIM_COUNTERMODE_UP;
	TIM8_Handler.Init.Period = u32Period;
	TIM8_Handler.Init.ClockDivision = TIM_CLOCKDIVISION_DIV4;
	if (HAL_TIM_Base_DeInit(&TIM8_Handler) != HAL_OK) {
		return -1;
	}
	if (HAL_TIM_Base_Init(&TIM8_Handler) != HAL_OK) {
		return -1;
	}
	
	/*S3:���ö�ʱ����ʱ��ԴTIM8*/
	sClockSourceConfig_TIM8.ClockSource = TIM_CLOCKSOURCE_ETRMODE2; // �ⲿʱ��Դģʽ2
	sClockSourceConfig_TIM8.ClockPolarity = TIM_CLOCKPOLARITY_NONINVERTED;
	sClockSourceConfig_TIM8.ClockPrescaler = TIM_CLOCKPRESCALER_DIV1;
	sClockSourceConfig_TIM8.ClockFilter = 0x4; // ������ת���ź��˲�4MHz (fSAMPLING=fDTS/2��N=6)
	if (HAL_TIM_ConfigClockSource(&TIM8_Handler, &sClockSourceConfig_TIM8) != HAL_OK) {
		return -1;
	}
	
	/*S4:��TIM8����Ϊ��ģʽ��ÿ�η��������¼�UEVʱ�����һ�������Դ����ź�*/
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_ENABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&TIM8_Handler, &sMasterConfig) != HAL_OK) {
		return -1;
	}	
    
	/*S5:���ö�ʱ��TIM2*/	
    TIM2_Handler.Instance = TIM2;								//ͨ�ö�ʱ��2
    TIM2_Handler.Init.Prescaler = 0;							//��Ƶ
    TIM2_Handler.Init.CounterMode = TIM_COUNTERMODE_UP;			//���ϼ���
    TIM2_Handler.Init.Period = 0xffffffff;						//�Զ�װ��ֵARR
    TIM2_Handler.Init.ClockDivision = TIM_CLOCKDIVISION_DIV4;
	if (HAL_TIM_IC_DeInit(&TIM2_Handler) != HAL_OK) {
		return -1;
	}
	if (HAL_TIM_IC_Init(&TIM2_Handler) != HAL_OK) {
		return -1;
	}
	
	/*S6:���ö�ʱ����ʱ��ԴTIM2*/
	sClockSourceConfig_TIM2.ClockSource = TIM_CLOCKSOURCE_INTERNAL; // 192MHzʱ�Ӷ������
	if (HAL_TIM_ConfigClockSource(&TIM2_Handler, &sClockSourceConfig_TIM2) != HAL_OK) {
		return -1;
	}
    
	/*S7:��ITR1��ΪTRC����Ϊ����������ź�*/
	sSlaveConfig.SlaveMode = TIM_SLAVEMODE_DISABLE;
	sSlaveConfig.InputTrigger = TIM_TS_ITR1; // ����ʱ��8��TRGO��Ϊ��ʱ��2�����벶���ź�TRC
	if (HAL_TIM_SlaveConfigSynchronization(&TIM2_Handler, &sSlaveConfig) != HAL_OK) {
		return FALSE;
	}
 	
	/*S8:���ö�ʱ��2�����벶����*/
    TIM2_IC1Config.ICPolarity = TIM_ICPOLARITY_RISING;    		//�����ز���
    TIM2_IC1Config.ICSelection = TIM_ICSELECTION_TRC;			//IC1 ӳ�䵽TRC��
    TIM2_IC1Config.ICPrescaler = TIM_ICPSC_DIV1;          		//���������Ƶ������Ƶ
    TIM2_IC1Config.ICFilter = 0;                          		//���������˲���(û���õ�)
    if (HAL_TIM_IC_ConfigChannel(&TIM2_Handler, &TIM2_IC1Config, TIM_CHANNEL_1) != HAL_OK) {
		return -1;
	}
	
	/*S9:���ö�ʱ��2��DMA����*/
	if (HAL_TIM_IC_Start_DMA(&TIM2_Handler, TIM_CHANNEL_1, buf, cnt) != HAL_OK) {
		return -1;
	}
	
	return 0;
}

/*
������ת���޷�Ƶʱ���ö�ʱ�����ú���
@param	pPingData 	ת��DMA Ping Buf
@param	pPongData 	ת��DMA Pong Buf
@param	SpdDatNum 	һ���Ի�ȡ��ͨ�����ݸ���
*/
static int tmr_cfg(U32* buf, U16 cnt)
{
	TIM_IC_InitTypeDef TIM2_IC1Config = {0};
	TIM_ClockConfigTypeDef sClockSourceConfig = {0};
 
	/*S1:���ö�ʱ��TIM2 CH1����*/	
	GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_0;            		// PA0
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;      		// ��������
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;        		// ����
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; 		// ����
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;   		// PA0����ΪTIM2
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	
	/*S2:���ö�ʱ��2*/	
    TIM2_Handler.Instance = TIM2;								//ͨ�ö�ʱ��2
    TIM2_Handler.Init.Prescaler = 0;							//��Ƶ
    TIM2_Handler.Init.CounterMode = TIM_COUNTERMODE_UP;			//���ϼ���
    TIM2_Handler.Init.Period = 0xffffffff;						//�Զ�װ��ֵARR
    TIM2_Handler.Init.ClockDivision = TIM_CLOCKDIVISION_DIV4;
	if (HAL_TIM_IC_DeInit(&TIM2_Handler) != HAL_OK) {
		return FALSE;
	}
	if (HAL_TIM_IC_Init(&TIM2_Handler) != HAL_OK) {
		return FALSE;
	}
	
	/*S3:���ö�ʱ����ʱ��Դ*/
	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;	// 192MHzʱ�Ӷ������
	if (HAL_TIM_ConfigClockSource(&TIM2_Handler, &sClockSourceConfig) != HAL_OK) {
		return FALSE;
	}
    
	/*S4:���ö�ʱ�������벶����*/
    TIM2_IC1Config.ICPolarity = TIM_ICPOLARITY_RISING;    		// �����ز���
    TIM2_IC1Config.ICSelection = TIM_ICSELECTION_DIRECTTI;		// ӳ�䵽TI1��
	TIM2_IC1Config.ICPrescaler = TIM_ICPSC_DIV1;				// ���������Ƶ������Ƶ
    TIM2_IC1Config.ICFilter = 0x4; 								// ������ת���ź��˲�4MHz (fSAMPLING=fDTS/2��N=6)
    if (HAL_TIM_IC_ConfigChannel(&TIM2_Handler, &TIM2_IC1Config, TIM_CHANNEL_1) != HAL_OK) {
		return FALSE;
	}
	
	/*S5:���ö�ʱ����DMA����*/
	if (HAL_TIM_IC_Start_DMA(&TIM2_Handler, TIM_CHANNEL_1, buf, cnt) != HAL_OK) {
		return FALSE;
	}
	
	return TRUE;
}



void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    
}

void HAL_TIM_IC_CaptureHalfCpltCallback(TIM_HandleTypeDef *htim)
{
    
}




int dal_tmr_init(void)
{
    __HAL_RCC_TIM2_CLK_ENABLE();					// ʹ��TIM2ʱ��
    __HAL_RCC_TIM8_CLK_ENABLE();            		// ʹ��TIM8ʱ��
    __HAL_RCC_GPIOA_CLK_ENABLE();					// ����GPIOAʱ��
    
    return 0;
}


int dal_tmr_config(dal_tmr_cfg_t *cfg)
{
    tmrCfg = *cfg;
    
    //tmr_cfg();
    //tmr_cfg_div();
    
    return 0;
}



int dal_tmr_start(void)
{
    TIM_CCxChannelCmd(TIM2_Handler.Instance, TIM_CHANNEL_1, TIM_CCx_ENABLE);
   
	/* Enable the Peripheral */
	__HAL_TIM_ENABLE(&TIM2_Handler);
	__HAL_TIM_ENABLE(&TIM8_Handler);
	
    return 0;
}


int dal_tmr_stop(void)
{
    if (HAL_TIM_IC_Stop_DMA(&TIM2_Handler, TIM_CHANNEL_1) != HAL_OK) {
		return -1;     
    }
    return 0;
}

