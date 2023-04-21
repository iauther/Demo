#include "dal_tmr.h"

static dal_tmr_cfg_t tmrCfg;
static TIM_HandleTypeDef TIM2_Handler;
static TIM_HandleTypeDef TIM8_Handler;

/*
描述：转速分频时所用定时器配置函数
@param	pPingData 	转速DMA Ping Buf
@param	pPongData 	转速DMA Pong Buf
@param	SpdDatNum 	一次性获取振动通道数据个数
@param	u32Period	转速分频值-1
*/
static int tmr_cfg_div(U32* buf, U16 cnt, U32 u32Period)
{
	TIM_ClockConfigTypeDef sClockSourceConfig_TIM2 = {0};
	TIM_ClockConfigTypeDef sClockSourceConfig_TIM8 = {0};	
	TIM_MasterConfigTypeDef sMasterConfig = {0};
	TIM_SlaveConfigTypeDef sSlaveConfig = {0};
	TIM_IC_InitTypeDef TIM2_IC1Config = {0};
	
	/*S1:配置定时器TIM8 ETR引脚*/
	GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_0;            		// PA0 PIN40
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;      		// 复用推挽
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;        		// 下拉
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; 		// 高速
    GPIO_InitStruct.Alternate = GPIO_AF3_TIM8;   		// PA0复用为TIM8
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*S2:配置定时器TIM8*/
	TIM8_Handler.Instance = TIM8;					// 通用定时器8
	TIM8_Handler.Init.Prescaler = 0;				// 可以对输入转速再分频
	TIM8_Handler.Init.CounterMode = TIM_COUNTERMODE_UP;
	TIM8_Handler.Init.Period = u32Period;
	TIM8_Handler.Init.ClockDivision = TIM_CLOCKDIVISION_DIV4;
	if (HAL_TIM_Base_DeInit(&TIM8_Handler) != HAL_OK) {
		return -1;
	}
	if (HAL_TIM_Base_Init(&TIM8_Handler) != HAL_OK) {
		return -1;
	}
	
	/*S3:配置定时器的时钟源TIM8*/
	sClockSourceConfig_TIM8.ClockSource = TIM_CLOCKSOURCE_ETRMODE2; // 外部时钟源模式2
	sClockSourceConfig_TIM8.ClockPolarity = TIM_CLOCKPOLARITY_NONINVERTED;
	sClockSourceConfig_TIM8.ClockPrescaler = TIM_CLOCKPRESCALER_DIV1;
	sClockSourceConfig_TIM8.ClockFilter = 0x4; // 对输入转速信号滤波4MHz (fSAMPLING=fDTS/2，N=6)
	if (HAL_TIM_ConfigClockSource(&TIM8_Handler, &sClockSourceConfig_TIM8) != HAL_OK) {
		return -1;
	}
	
	/*S4:将TIM8配置为主模式，每次发生更新事件UEV时都输出一个周期性触发信号*/
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_ENABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&TIM8_Handler, &sMasterConfig) != HAL_OK) {
		return -1;
	}	
    
	/*S5:配置定时器TIM2*/	
    TIM2_Handler.Instance = TIM2;								//通用定时器2
    TIM2_Handler.Init.Prescaler = 0;							//分频
    TIM2_Handler.Init.CounterMode = TIM_COUNTERMODE_UP;			//向上计数
    TIM2_Handler.Init.Period = 0xffffffff;						//自动装载值ARR
    TIM2_Handler.Init.ClockDivision = TIM_CLOCKDIVISION_DIV4;
	if (HAL_TIM_IC_DeInit(&TIM2_Handler) != HAL_OK) {
		return -1;
	}
	if (HAL_TIM_IC_Init(&TIM2_Handler) != HAL_OK) {
		return -1;
	}
	
	/*S6:配置定时器的时钟源TIM2*/
	sClockSourceConfig_TIM2.ClockSource = TIM_CLOCKSOURCE_INTERNAL; // 192MHz时钟对其计数
	if (HAL_TIM_ConfigClockSource(&TIM2_Handler, &sClockSourceConfig_TIM2) != HAL_OK) {
		return -1;
	}
    
	/*S7:将ITR1作为TRC，作为捕获的输入信号*/
	sSlaveConfig.SlaveMode = TIM_SLAVEMODE_DISABLE;
	sSlaveConfig.InputTrigger = TIM_TS_ITR1; // 将定时器8的TRGO作为定时器2的输入捕获信号TRC
	if (HAL_TIM_SlaveConfigSynchronization(&TIM2_Handler, &sSlaveConfig) != HAL_OK) {
		return FALSE;
	}
 	
	/*S8:配置定时器2的输入捕获功能*/
    TIM2_IC1Config.ICPolarity = TIM_ICPOLARITY_RISING;    		//上升沿捕获
    TIM2_IC1Config.ICSelection = TIM_ICSELECTION_TRC;			//IC1 映射到TRC上
    TIM2_IC1Config.ICPrescaler = TIM_ICPSC_DIV1;          		//配置输入分频，不分频
    TIM2_IC1Config.ICFilter = 0;                          		//配置输入滤波器(没有用到)
    if (HAL_TIM_IC_ConfigChannel(&TIM2_Handler, &TIM2_IC1Config, TIM_CHANNEL_1) != HAL_OK) {
		return -1;
	}
	
	/*S9:配置定时器2的DMA功能*/
	if (HAL_TIM_IC_Start_DMA(&TIM2_Handler, TIM_CHANNEL_1, buf, cnt) != HAL_OK) {
		return -1;
	}
	
	return 0;
}

/*
描述：转速无分频时所用定时器配置函数
@param	pPingData 	转速DMA Ping Buf
@param	pPongData 	转速DMA Pong Buf
@param	SpdDatNum 	一次性获取振动通道数据个数
*/
static int tmr_cfg(U32* buf, U16 cnt)
{
	TIM_IC_InitTypeDef TIM2_IC1Config = {0};
	TIM_ClockConfigTypeDef sClockSourceConfig = {0};
 
	/*S1:配置定时器TIM2 CH1引脚*/	
	GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_0;            		// PA0
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;      		// 复用推挽
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;        		// 下拉
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; 		// 高速
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;   		// PA0复用为TIM2
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	
	/*S2:配置定时器2*/	
    TIM2_Handler.Instance = TIM2;								//通用定时器2
    TIM2_Handler.Init.Prescaler = 0;							//分频
    TIM2_Handler.Init.CounterMode = TIM_COUNTERMODE_UP;			//向上计数
    TIM2_Handler.Init.Period = 0xffffffff;						//自动装载值ARR
    TIM2_Handler.Init.ClockDivision = TIM_CLOCKDIVISION_DIV4;
	if (HAL_TIM_IC_DeInit(&TIM2_Handler) != HAL_OK) {
		return FALSE;
	}
	if (HAL_TIM_IC_Init(&TIM2_Handler) != HAL_OK) {
		return FALSE;
	}
	
	/*S3:配置定时器的时钟源*/
	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;	// 192MHz时钟对其计数
	if (HAL_TIM_ConfigClockSource(&TIM2_Handler, &sClockSourceConfig) != HAL_OK) {
		return FALSE;
	}
    
	/*S4:配置定时器的输入捕获功能*/
    TIM2_IC1Config.ICPolarity = TIM_ICPOLARITY_RISING;    		// 上升沿捕获
    TIM2_IC1Config.ICSelection = TIM_ICSELECTION_DIRECTTI;		// 映射到TI1上
	TIM2_IC1Config.ICPrescaler = TIM_ICPSC_DIV1;				// 配置输入分频，不分频
    TIM2_IC1Config.ICFilter = 0x4; 								// 对输入转速信号滤波4MHz (fSAMPLING=fDTS/2，N=6)
    if (HAL_TIM_IC_ConfigChannel(&TIM2_Handler, &TIM2_IC1Config, TIM_CHANNEL_1) != HAL_OK) {
		return FALSE;
	}
	
	/*S5:配置定时器的DMA功能*/
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
    __HAL_RCC_TIM2_CLK_ENABLE();					// 使能TIM2时钟
    __HAL_RCC_TIM8_CLK_ENABLE();            		// 使能TIM8时钟
    __HAL_RCC_GPIOA_CLK_ENABLE();					// 开启GPIOA时钟
    
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

