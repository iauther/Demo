#include "sdram.h"
#include "drv/delay.h"

static SDRAM_HandleTypeDef SDRAM_Handler = {0};

static int sdram_send_cmd(U8 cmd, U8 refresh, U16 regval)
{
    FMC_SDRAM_CommandTypeDef Command = {0};
    
    Command.CommandMode = cmd;
    Command.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK2; // Ŀ��SDRAM�洢����
    Command.AutoRefreshNumber = refresh;				// ��ˢ�´���
    Command.ModeRegisterDefinition = regval;			// Ҫд��ģʽ�Ĵ�����ֵ
    
	if (HAL_SDRAM_SendCommand(&SDRAM_Handler, &Command, 0XFFFF) == HAL_OK) {
        return 1;  
    }
    else {
        return 0;  
    }
}
static void sdram_config(SDRAM_HandleTypeDef *hsdram)
{
	U32 tmpmrd = 0;
    U32 timeout = 0xFFFF;
	
	/* Wait until the SDRAM controller is ready */
	while((__FMC_SDRAM_GET_FLAG(FMC_SDRAM_DEVICE, HAL_SDRAM_STATE_BUSY)) && (timeout > 0))
	{
		timeout--;
	}
    // SDRAM��������ʼ������Ժ���Ҫ��������˳���ʼ��SDRAM
    sdram_send_cmd(FMC_SDRAM_CMD_CLK_ENABLE, 1, 0); 		// ʱ������ʹ��
    delay_us(500);                                  		// ������ʱ500us
	timeout = 0xFFFF;
	while((__FMC_SDRAM_GET_FLAG(FMC_SDRAM_DEVICE, HAL_SDRAM_STATE_BUSY)) && (timeout > 0))
	{
		timeout--;
	}
	sdram_send_cmd(FMC_SDRAM_CMD_PALL, 1, 0);       		// �����д洢��Ԥ���
	timeout = 0xFFFF;
	while((__FMC_SDRAM_GET_FLAG(FMC_SDRAM_DEVICE, HAL_SDRAM_STATE_BUSY)) && (timeout > 0))
	{
		timeout--;
	}
    sdram_send_cmd(FMC_SDRAM_CMD_AUTOREFRESH_MODE, 8, 0);	// ������ˢ�´��� 
    
	// ����ģʽ�Ĵ���
	// bit0~bit2Ϊָ��ͻ�����ʵĳ���
	// bit3Ϊָ��ͻ�����ʵ����ͣ�bit4~bit6ΪCASֵ��bit7��bit8Ϊ����ģʽ
	// bit9Ϊָ����дͻ��ģʽ��bit10��bit11λ����λ
	tmpmrd = (U32)(SDRAM_MODEREG_BURST_LENGTH_1  |		// ����ͻ������:1(������1/2/4/8)
              SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL   |		// ����ͻ������:����(����������/����)
              SDRAM_MODEREG_CAS_LATENCY_2           |		// ����CASֵ:2(������2/3)
              SDRAM_MODEREG_OPERATING_MODE_STANDARD |   	// ���ò���ģʽ:0��׼ģʽ
              SDRAM_MODEREG_WRITEBURST_MODE_SINGLE);     	// ����ͻ��дģʽ:1�������
		timeout = 0xFFFF;
		while((__FMC_SDRAM_GET_FLAG(FMC_SDRAM_DEVICE, HAL_SDRAM_STATE_BUSY)) && (timeout > 0))
	{
		timeout--;
	}
    
	sdram_send_cmd(FMC_SDRAM_CMD_LOAD_MODE, 1, tmpmrd);		// ����SDRAM��ģʽ�Ĵ���
    
    // ˢ��Ƶ�ʼ�����(��SDCLKƵ�ʼ���)�����㷽��:
	// COUNT=SDRAMˢ������/����-20=SDRAMˢ������(us)*SDCLKƵ��(Mhz)/����-20
    // SDRAMˢ������Ϊ64ms��SDCLK=96Mhz������Ϊ8192(2^13).
	// COUNT=64*1000*96/8192-20=730
	HAL_SDRAM_ProgramRefreshRate(&SDRAM_Handler, 730);	
	timeout = 0xFFFF;
	while((__FMC_SDRAM_GET_FLAG(FMC_SDRAM_DEVICE, HAL_SDRAM_STATE_BUSY)) && (timeout > 0))
	{
		timeout--;
	}
}



int sdram_read(U8 *buf, U32 offset, U32 len)
{
    if(!buf || !len || offset>=SDRAM_SIZE || offset+len>=SDRAM_SIZE) {
        return -1;
    }
    
	for(; len!=0; len--) {
		*buf++ = *(__IO U8 *)(BANK6_SDRAM_ADDR+offset);
		offset++;
	}
    
    return 0;
}


int sdram_write(U8 *buf, U32 offset, U32 len)
{
    if(!buf || !len || offset>=SDRAM_SIZE || offset+len>=SDRAM_SIZE) {
        return -1;
    }
    
	for(; len!=0; len--) {
		*(__IO U8 *)(BANK6_SDRAM_ADDR+offset) = *buf;
		offset++;
		buf++;
	}
    
    return 0;
}




void HAL_SDRAM_MspInit(SDRAM_HandleTypeDef *hsdram)
{
    GPIO_InitTypeDef init;

	/*S1��ʹ��FMCʱ�Ӻ�GPIOʱ��*/
    __HAL_RCC_FMC_CLK_ENABLE();                 	    // ʹ��FMCʱ��
	__HAL_RCC_GPIOC_CLK_ENABLE();               	    // ʹ��GPIOCʱ��
    __HAL_RCC_GPIOD_CLK_ENABLE();               	    // ʹ��GPIODʱ��
    __HAL_RCC_GPIOE_CLK_ENABLE();               	    // ʹ��GPIOEʱ��
    __HAL_RCC_GPIOF_CLK_ENABLE();               	    // ʹ��GPIOFʱ��
    __HAL_RCC_GPIOG_CLK_ENABLE();               	    // ʹ��GPIOGʱ��
	__HAL_RCC_GPIOH_CLK_ENABLE();               	    // ʹ��GPIOHʱ��
	__HAL_RCC_GPIOI_CLK_ENABLE();               	    // ʹ��GPIOIʱ��   
    
    /*S2������GPIO*/	
    init.Mode = GPIO_MODE_AF_PP;          	// ���츴��
    init.Pull = GPIO_PULLUP;              	// ����
    init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;	// ����
    init.Alternate = GPIO_AF12_FMC;			// ����ΪFMC 

    init.Pin = GPIO_PIN_0;
    HAL_GPIO_Init(GPIOC, &init);
	
    init.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15;  	
    HAL_GPIO_Init(GPIOD, &init);

    init.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | \
               GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOE, &init);
	
    init.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | \
               GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOF, &init);
	
    init.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_8 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOG, &init);
	
    init.Pin = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | \
               GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOH, &init);
	
    init.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | \
               GPIO_PIN_7 | GPIO_PIN_9 | GPIO_PIN_10;
    HAL_GPIO_Init(GPIOI, &init);
}


int sdram_init(void)
{
    FMC_SDRAM_TimingTypeDef SDRAM_Timing = {0};

    SDRAM_Handler.Instance = FMC_SDRAM_DEVICE;
    SDRAM_Handler.Init.SDBank = FMC_SDRAM_BANK2;							// SDRAM����BANK6��
    SDRAM_Handler.Init.ColumnBitsNumber = FMC_SDRAM_COLUMN_BITS_NUM_9;     	// ������9
    SDRAM_Handler.Init.RowBitsNumber = FMC_SDRAM_ROW_BITS_NUM_13;          	// ������13
    SDRAM_Handler.Init.MemoryDataWidth = FMC_SDRAM_MEM_BUS_WIDTH_32;       	// ���ݿ��Ϊ32λ
    SDRAM_Handler.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;  	// 4��BANK
    SDRAM_Handler.Init.CASLatency = FMC_SDRAM_CAS_LATENCY_2;               	// CASΪ2
    SDRAM_Handler.Init.WriteProtection = FMC_SDRAM_WRITE_PROTECTION_DISABLE;// ʧ��д����
    SDRAM_Handler.Init.SDClockPeriod = FMC_SDRAM_CLOCK_PERIOD_2;           	// SDRAMʱ��ΪHCLK/2=192MHz/2=96MHz=10.417ns
    SDRAM_Handler.Init.ReadBurst = FMC_SDRAM_RBURST_ENABLE;                	// ʹ��ͻ��
    SDRAM_Handler.Init.ReadPipeDelay = FMC_SDRAM_RPIPE_DELAY_2;            	// RPIPE����Ϊ2��ʱ�������ӳ٣��Խ�ͻ�������䳤������Ϊ4����
    
    SDRAM_Timing.LoadToActiveDelay = 2;		                                // TMRD������ģʽ�Ĵ���������ʱ����ӳ�Ϊ2��ʱ������
    SDRAM_Timing.ExitSelfRefreshDelay = 7;	                                // TXSR���ӷ�����ˢ�����������������֮����ӳ�Ϊ7��ʱ������
    SDRAM_Timing.SelfRefreshTime = 5;		                                // TRAS����ˢ��ʱ��Ϊ5��ʱ������
    SDRAM_Timing.RowCycleDelay = 6;			                                // TRC��ˢ������ͼ�������֮����ӳ�Ϊ6��ʱ������
    SDRAM_Timing.WriteRecoveryTime = 3;		                                // TWR��д�����Ԥ�������֮����ӳ�Ϊ3��ʱ�����ڣ���֤���ݿɿ�д��
    SDRAM_Timing.RPDelay = 2;				                                // TRP��Ԥ�����������������֮����ӳ�Ϊ2��ʱ������
    SDRAM_Timing.RCDDelay = 2;				                                // TRCD�������������/д����֮����ӳ�Ϊ2��ʱ������
    HAL_SDRAM_Init(&SDRAM_Handler, &SDRAM_Timing);  
	
	sdram_config(&SDRAM_Handler);
    
    return 0;
}

