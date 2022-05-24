#include "board.h"
#include "drv/clk.h"
#include "drv/gpio.h"
#include "drv/delay.h"
#include "drv/uart.h"
#include "drv/i2c.h"
#include "drv/clk.h"
#include "paras.h"
#include "power.h"
#include "wdog.h"
#include "lcd.h"
#include "drv/adc.h"
#include "task.h"
#include "myCfg.h"
#include "drv/spi.h"

handle_t i2c1Handle=NULL;
handle_t spi1Handle=NULL;

////////////////////////////////////////////

void HAL_MspInit(void)
{
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();
    
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_AFIO_REMAP_SWJ_NOJTAG();
}


static void i2c_bus_init(void)
{
    i2c_cfg_t  ic;
    i2c_pin_t  p1={I2C1_SCL_PIN,I2C1_SDA_PIN};
    
    
    ic.pin= p1;
    ic.freq = I2C1_FREQ;
    ic.useDMA = 0;;
    ic.callback = NULL;
    ic.finish_flag=0;
    i2c1Handle = i2c_init(&ic); 
}

static void spi_bus_init(void)
{
    spi_cfg_t cfg;
    
    cfg.port = SPI_PORT;
    cfg.init.Mode = SPI_MODE_MASTER;
    cfg.init.Direction = SPI_DIRECTION_2LINES;
    cfg.init.DataSize = SPI_DATASIZE_8BIT;
    cfg.init.CLKPolarity = SPI_POLARITY_LOW;
    //cfg.init.CLKPhase = SPI_PHASE_1EDGE;  //LCD: 1EDGE
    cfg.init.CLKPhase = SPI_PHASE_2EDGE;    //AD9834: 2EDGE
    cfg.init.NSS = SPI_NSS_SOFT;
    cfg.init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    cfg.init.FirstBit = SPI_FIRSTBIT_MSB;
    cfg.init.TIMode = SPI_TIMODE_DISABLE;
    cfg.init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    cfg.init.CRCPolynomial = 10;
    
    spi1Handle = spi_init(&cfg);
}


static void bus_init(void)
{
    i2c_bus_init();
    spi_bus_init();
}




int board_init(void)
{
    int r;
    //lcd_cfg_t cfg={LCD_COLOR, LCD_WIDTH, LCD_HEIGHT};
    
    r = HAL_Init();
    r = clk2_init();
    
    bus_init();
    
    task_init();
    task_start();  
    
   
    return r;
}


int board_deinit(void)
{
    int r;
    
    r = HAL_DeInit();
    r = HAL_RCC_DeInit();
   
    return r;
}
















