#include "bsp.h"
#include "cfg.h"
#include "platform.h"

U32 sys_freq = 0;static U32 s_freq=0;


void HAL_MspInit(void)
{
    __HAL_RCC_SYSCFG_CLK_ENABLE();
}

static void cache_enable(void)
{
    SCB_EnableICache();	// ??I-Cache
    SCB_EnableDCache();	// ??D-Cache
    
	//SCB->CACR |= 1<<2;	// ??D-Cache??,????,?????????????	
}
static void mpu_config(void)
{
	MPU_Region_InitTypeDef MPU_InitStruct = {0};
    
    HAL_MPU_Disable();
	
#if 0
    MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.BaseAddress = 0x30000000; // DMA Ping???
    MPU_InitStruct.Size = MPU_REGION_SIZE_128KB;
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    MPU_InitStruct.Number = MPU_REGION_NUMBER0;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
    MPU_InitStruct.SubRegionDisable = 0x00;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);
	
    MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.BaseAddress = 0x30020000; // DMA Pong???
    MPU_InitStruct.Size = MPU_REGION_SIZE_128KB;
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    MPU_InitStruct.Number = MPU_REGION_NUMBER1;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
    MPU_InitStruct.SubRegionDisable = 0x00;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);
    
	MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.BaseAddress = 0xD3200000; // MDMA Ping???
    MPU_InitStruct.Size = MPU_REGION_SIZE_512KB;
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    MPU_InitStruct.Number = MPU_REGION_NUMBER2;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
    MPU_InitStruct.SubRegionDisable = 0x00;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);
	
	MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.BaseAddress = 0xD3280000; // MDMA Pong???
    MPU_InitStruct.Size = MPU_REGION_SIZE_512KB;
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    MPU_InitStruct.Number = MPU_REGION_NUMBER3;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
    MPU_InitStruct.SubRegionDisable = 0x00;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);

    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);	
#else
    /* Configure the MPU as Strongly ordered for not defined regions */
    MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.BaseAddress = 0x00;
    MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
    MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
    MPU_InitStruct.Number = MPU_REGION_NUMBER0;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
    MPU_InitStruct.SubRegionDisable = 0x87;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);

    /* Configure the MPU attributes as Device not cacheable 
     for ETH DMA descriptors */
    MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.BaseAddress = 0x30000000;
    MPU_InitStruct.Size = MPU_REGION_SIZE_1KB;
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    MPU_InitStruct.Number = MPU_REGION_NUMBER1;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
    MPU_InitStruct.SubRegionDisable = 0x00;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);

    /* Configure the MPU attributes as Normal Non Cacheable
     for LwIP RAM heap which contains the Tx buffers */
    MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.BaseAddress = 0x30004000;
    MPU_InitStruct.Size = MPU_REGION_SIZE_16KB;
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
    MPU_InitStruct.Number = MPU_REGION_NUMBER2;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
    MPU_InitStruct.SubRegionDisable = 0x00;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);
#endif

    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}



static int bsp_clk_init(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
	RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  
	// ?????,VCORE 1.2V???????VCAP????
	HAL_PWREx_ConfigSupply(PWR_EXTERNAL_SOURCE_SUPPLY);
	// Configure the main internal regulator output voltage
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
  
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
	RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.CSIState = RCC_CSI_OFF;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;

	// ?????24.576MHz
	RCC_OscInitStruct.PLL.PLLM = 4;
	RCC_OscInitStruct.PLL.PLLN = 125; // Fvco=24.576*(125/4)=768MHz
	RCC_OscInitStruct.PLL.PLLP = 2; // Fsys=768/2=384MHz
	RCC_OscInitStruct.PLL.PLLQ = 16; // Fq=768/16=48MHz
	RCC_OscInitStruct.PLL.PLLR = 2;
	RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
	RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
	RCC_OscInitStruct.PLL.PLLFRACN = 0;

	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
        //DEBUG_PRINTF("[SystemClock_Config] RCC Oscillators Config Error !!! \r\n");
		return -1;
	}
    
	// Initializes the CPU, AHB and APB buses clocks
	RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK	 | \
                                   RCC_CLOCKTYPE_HCLK	 | \
                                   RCC_CLOCKTYPE_D1PCLK1 | \
                                   RCC_CLOCKTYPE_PCLK1   | \
                                   RCC_CLOCKTYPE_PCLK2   | \
                                   RCC_CLOCKTYPE_D3PCLK1);
	
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
	RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
	RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;
	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
	{
        //DEBUG_PRINTF("[SystemClock_Config] RCC Clock Config Error !!! \r\n");
		return -1;
	}
	
	PeriphClkInitStruct.PeriphClockSelection = (RCC_PERIPHCLK_SAI1   |
												RCC_PERIPHCLK_SAI4A  |
											    RCC_PERIPHCLK_ADC    |
											    RCC_PERIPHCLK_USART3 |
												RCC_PERIPHCLK_USART6 |
												RCC_PERIPHCLK_SPI1   |
												RCC_PERIPHCLK_QSPI   |
												RCC_PERIPHCLK_SDMMC  |
												RCC_PERIPHCLK_CKPER /* |
											    RCC_PERIPHCLK_USB */);
	PeriphClkInitStruct.PLL2.PLL2M = 2;
	PeriphClkInitStruct.PLL2.PLL2N = 15;
	PeriphClkInitStruct.PLL2.PLL2P = 2; // 92.16MHz
	PeriphClkInitStruct.PLL2.PLL2Q = 16; // 11.52MHz
	PeriphClkInitStruct.PLL2.PLL2R = 3;
	PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_3;
	PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOMEDIUM;
	PeriphClkInitStruct.PLL2.PLL2FRACN = 0;
	PeriphClkInitStruct.PLL3.PLL3M = 3;
	PeriphClkInitStruct.PLL3.PLL3N = 80;
	PeriphClkInitStruct.PLL3.PLL3P = 25; // 26.2144MHz
	PeriphClkInitStruct.PLL3.PLL3Q = 8;
	PeriphClkInitStruct.PLL3.PLL3R = 32; // 20.48MHz
	PeriphClkInitStruct.PLL3.PLL3RGE = RCC_PLL3VCIRANGE_3;
	PeriphClkInitStruct.PLL3.PLL3VCOSEL = RCC_PLL3VCOWIDE;
	PeriphClkInitStruct.PLL3.PLL3FRACN = 0;
	
	PeriphClkInitStruct.Sai1ClockSelection = RCC_SAI1CLKSOURCE_PLL3; // Vib SAI
	PeriphClkInitStruct.Sai4AClockSelection = RCC_SAI4ACLKSOURCE_PLL3; // Vib SAI MCLK & FS & SCK
	PeriphClkInitStruct.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_PLL2; // RS232 USART3
	PeriphClkInitStruct.Usart16ClockSelection = RCC_USART16CLKSOURCE_PLL2; // RS485 USART6
//	PeriphClkInitStruct.UsbClockSelection = RCC_USBCLKSOURCE_PLL;
	PeriphClkInitStruct.AdcClockSelection = RCC_ADCCLKSOURCE_PLL3;
	PeriphClkInitStruct.SdmmcClockSelection = RCC_SDMMCCLKSOURCE_PLL;
	PeriphClkInitStruct.Spi123ClockSelection = RCC_SPI123CLKSOURCE_PLL2; // SPI 92.16MHz
	PeriphClkInitStruct.QspiClockSelection = RCC_QSPICLKSOURCE_CLKP; // QSPI PSRAM 64MHz
	PeriphClkInitStruct.CkperClockSelection = RCC_CLKPSOURCE_HSI;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
	{
        //DEBUG_PRINTF("[SystemClock_Config] RCC peripherals Clock Config Error !!! \r\n");
		return -1;
	}

	__HAL_RCC_CSI_ENABLE();
	__HAL_RCC_SYSCFG_CLK_ENABLE();
	// Enables the I/O Compensation Cell
	HAL_EnableCompensationCell();
	
	// AXI SRAM???????????
	// D2 SRAM is activated as soon as any peripheral(AHB1, AHB2, APB1 and APB2) in D2 is activated.
	__HAL_RCC_D2SRAM1_CLK_ENABLE();		// SRAM1
	__HAL_RCC_D2SRAM2_CLK_ENABLE();		// SRAM2
	__HAL_RCC_D2SRAM3_CLK_ENABLE();		// SRAM3
	__HAL_RCC_D3SRAM1_CLKAM_ENABLE();	// SRAM4
	
	// Enable USB Voltage detector
    //HAL_PWREx_EnableUSBVoltageDetector();
	//PowerCtrl_Init();
	
	return 0;
}


static int clk_init(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

    while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

    /** Initializes the RCC Oscillators according to the specified parameters
    * in the RCC_OscInitTypeDef structure.*/
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 2;
#if (SYS_FREQ==(480*MHZ))
    RCC_OscInitStruct.PLL.PLLN = 78;
#else
    RCC_OscInitStruct.PLL.PLLN = 62;
#endif
    RCC_OscInitStruct.PLL.PLLP = 2;
    RCC_OscInitStruct.PLL.PLLQ = 5;
    RCC_OscInitStruct.PLL.PLLR = 2;
    RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
    RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
#if (SYS_FREQ==(480*MHZ))
    RCC_OscInitStruct.PLL.PLLFRACN = 1024;
#else
    RCC_OscInitStruct.PLL.PLLFRACN = 4096;
#endif
        
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        return -1;
    }

    //Initializes the CPU, AHB and APB buses clocks
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                          |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                          |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
    RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK){
        return -1;
    }
  
    return 0;
}


int bsp_init(void)
{
    mpu_config();
    cache_enable();
    
    HAL_Init();
    bsp_clk_init();
    
    sys_freq = HAL_RCC_GetSysClockFreq();
    
    return 0;
}


U32 bsp_get_freq(void)
{
    return HAL_RCC_GetSysClockFreq();
}



