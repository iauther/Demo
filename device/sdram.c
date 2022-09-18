#include "sdram.h"
#include "drv/delay.h"

static SDRAM_HandleTypeDef SDRAM_Handler = {0};

static int sdram_send_cmd(U8 cmd, U8 refresh, U16 regval)
{
    FMC_SDRAM_CommandTypeDef Command = {0};
    
    Command.CommandMode = cmd;
    Command.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK2; // 目标SDRAM存储区域
    Command.AutoRefreshNumber = refresh;				// 自刷新次数
    Command.ModeRegisterDefinition = regval;			// 要写入模式寄存器的值
    
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
    // SDRAM控制器初始化完成以后还需要按照如下顺序初始化SDRAM
    sdram_send_cmd(FMC_SDRAM_CMD_CLK_ENABLE, 1, 0); 		// 时钟配置使能
    delay_us(500);                                  		// 至少延时500us
	timeout = 0xFFFF;
	while((__FMC_SDRAM_GET_FLAG(FMC_SDRAM_DEVICE, HAL_SDRAM_STATE_BUSY)) && (timeout > 0))
	{
		timeout--;
	}
	sdram_send_cmd(FMC_SDRAM_CMD_PALL, 1, 0);       		// 对所有存储区预充电
	timeout = 0xFFFF;
	while((__FMC_SDRAM_GET_FLAG(FMC_SDRAM_DEVICE, HAL_SDRAM_STATE_BUSY)) && (timeout > 0))
	{
		timeout--;
	}
    sdram_send_cmd(FMC_SDRAM_CMD_AUTOREFRESH_MODE, 8, 0);	// 设置自刷新次数 
    
	// 配置模式寄存器
	// bit0~bit2为指定突发访问的长度
	// bit3为指定突发访问的类型，bit4~bit6为CAS值，bit7和bit8为运行模式
	// bit9为指定的写突发模式，bit10和bit11位保留位
	tmpmrd = (U32)(SDRAM_MODEREG_BURST_LENGTH_1  |		// 设置突发长度:1(可以是1/2/4/8)
              SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL   |		// 设置突发类型:连续(可以是连续/交错)
              SDRAM_MODEREG_CAS_LATENCY_2           |		// 设置CAS值:2(可以是2/3)
              SDRAM_MODEREG_OPERATING_MODE_STANDARD |   	// 设置操作模式:0标准模式
              SDRAM_MODEREG_WRITEBURST_MODE_SINGLE);     	// 设置突发写模式:1单点访问
		timeout = 0xFFFF;
		while((__FMC_SDRAM_GET_FLAG(FMC_SDRAM_DEVICE, HAL_SDRAM_STATE_BUSY)) && (timeout > 0))
	{
		timeout--;
	}
    
	sdram_send_cmd(FMC_SDRAM_CMD_LOAD_MODE, 1, tmpmrd);		// 设置SDRAM的模式寄存器
    
    // 刷新频率计数器(以SDCLK频率计数)，计算方法:
	// COUNT=SDRAM刷新周期/行数-20=SDRAM刷新周期(us)*SDCLK频率(Mhz)/行数-20
    // SDRAM刷新周期为64ms，SDCLK=96Mhz，行数为8192(2^13).
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

	/*S1：使能FMC时钟和GPIO时钟*/
    __HAL_RCC_FMC_CLK_ENABLE();                 	    // 使能FMC时钟
	__HAL_RCC_GPIOC_CLK_ENABLE();               	    // 使能GPIOC时钟
    __HAL_RCC_GPIOD_CLK_ENABLE();               	    // 使能GPIOD时钟
    __HAL_RCC_GPIOE_CLK_ENABLE();               	    // 使能GPIOE时钟
    __HAL_RCC_GPIOF_CLK_ENABLE();               	    // 使能GPIOF时钟
    __HAL_RCC_GPIOG_CLK_ENABLE();               	    // 使能GPIOG时钟
	__HAL_RCC_GPIOH_CLK_ENABLE();               	    // 使能GPIOH时钟
	__HAL_RCC_GPIOI_CLK_ENABLE();               	    // 使能GPIOI时钟   
    
    /*S2：配置GPIO*/	
    init.Mode = GPIO_MODE_AF_PP;          	// 推挽复用
    init.Pull = GPIO_PULLUP;              	// 上拉
    init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;	// 高速
    init.Alternate = GPIO_AF12_FMC;			// 复用为FMC 

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
    SDRAM_Handler.Init.SDBank = FMC_SDRAM_BANK2;							// SDRAM接在BANK6上
    SDRAM_Handler.Init.ColumnBitsNumber = FMC_SDRAM_COLUMN_BITS_NUM_9;     	// 列数量9
    SDRAM_Handler.Init.RowBitsNumber = FMC_SDRAM_ROW_BITS_NUM_13;          	// 行数量13
    SDRAM_Handler.Init.MemoryDataWidth = FMC_SDRAM_MEM_BUS_WIDTH_32;       	// 数据宽度为32位
    SDRAM_Handler.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;  	// 4个BANK
    SDRAM_Handler.Init.CASLatency = FMC_SDRAM_CAS_LATENCY_2;               	// CAS为2
    SDRAM_Handler.Init.WriteProtection = FMC_SDRAM_WRITE_PROTECTION_DISABLE;// 失能写保护
    SDRAM_Handler.Init.SDClockPeriod = FMC_SDRAM_CLOCK_PERIOD_2;           	// SDRAM时钟为HCLK/2=192MHz/2=96MHz=10.417ns
    SDRAM_Handler.Init.ReadBurst = FMC_SDRAM_RBURST_ENABLE;                	// 使能突发
    SDRAM_Handler.Init.ReadPipeDelay = FMC_SDRAM_RPIPE_DELAY_2;            	// RPIPE设置为2个时钟周期延迟，以将突发读传输长度设置为4个字
    
    SDRAM_Timing.LoadToActiveDelay = 2;		                                // TMRD，加载模式寄存器到激活时间的延迟为2个时钟周期
    SDRAM_Timing.ExitSelfRefreshDelay = 7;	                                // TXSR，从发出自刷新命令到发出激活命令之间的延迟为7个时钟周期
    SDRAM_Timing.SelfRefreshTime = 5;		                                // TRAS，自刷新时间为5个时钟周期
    SDRAM_Timing.RowCycleDelay = 6;			                                // TRC，刷新命令和激活命令之间的延迟为6个时钟周期
    SDRAM_Timing.WriteRecoveryTime = 3;		                                // TWR，写命令和预充电命令之间的延迟为3个时钟周期，保证数据可靠写入
    SDRAM_Timing.RPDelay = 2;				                                // TRP，预充电命令与其它命令之间的延迟为2个时钟周期
    SDRAM_Timing.RCDDelay = 2;				                                // TRCD，激活命令与读/写命令之间的延迟为2个时钟周期
    HAL_SDRAM_Init(&SDRAM_Handler, &SDRAM_Timing);  
	
	sdram_config(&SDRAM_Handler);
    
    return 0;
}

