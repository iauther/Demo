/***************************************************************************//**
 *   @file   ad9834.c
 *   @brief  Implementation of ad9834 Driver.
 *   @author Mihai Bancisor
 *   SVN Revision: 538
*******************************************************************************/

//https://blog.csdn.net/qq_36024066/article/details/89667751
//


#include "ad9834.h"		//ad9834 definitions
#include "drv/spi.h"
#include "drv/gpio.h"
#include "myCfg.h"


/* GPIO pins */


#define PSEL_LOW        	// Add your code here
#define PSEL_HIGH       	// Add your code here
#define FSEL_LOW        	// Add your code here
#define FSEL_HIGH       	// Add your code here



extern handle_t spi1Handle;
static handle_t adHandle=NULL;
static void Gpio_Init(void)
{
    
}
static U8 SPI_Init(U8 lsbFirst, U32 clockFreq, U8 clockPol, U8 clockPha)
{
    spi_cfg_t cfg;
    
    cfg.port = SPI_1;
    
    
    
    //adHandle = spi_init();
    return 0;
}
static U8 SPI_Write(U8* data, U8 bytesNumber)
{

    return 0;
}
static U8 SPI_Read(U8* data, U8 bytesNumber)
{

    return 0;
}

//////////////////////////////////////////////////
static void write_reg(U16 value)
{
	U8 data[2];	
	
	data[0] = (U8)((value & 0xFF00) >> 8);
	data[1] = (U8)((value & 0x00FF));
	
	//ADI_CS_LOW;	    
	SPI_Write(data, 2);
	//ADI_CS_HIGH;
}

int ad9834_init(void)
{
    adHandle = spi1Handle;
    
    Gpio_Init();
    //SPI_Init(0, 1000000, 0, 1);
    
    write_reg(AD9834_REG_CMD | AD9834_RESET);
	
    return 0;
}



void ad9834_set_reset(void)
{
    write_reg(AD9834_REG_CMD | AD9834_RESET);
}



void ad9834_clr_reset(void)
{
	write_reg(AD9834_REG_CMD);
}






void ad9834_set_freq(U16 reg, U32 val)
{
	U16 freqHi = reg;
	U16 freqLo = reg;
	gpio_pin_t psel=PSEL_PIN;
	gpio_pin_t fsel=FSEL_PIN;
	
	gpio_set_hl(&psel, 0);
	gpio_set_hl(&fsel, 0);
	
	freqHi |= (val & 0xFFFC000) >> 14 ;
	freqLo |= (val & 0x3FFF);
	write_reg(AD9834_B28);
	write_reg(freqLo);
	write_reg(freqHi);
}



void ad9834_set_phase(U16 reg, U16 val)
{
	U16 phase = reg|val;
	
	write_reg(phase);
}


void ad9834_setup(U16 freq, U16 phase, U16 type, U16 cmdType)
{
	U16 val = 0;
	gpio_pin_t psel=PSEL_PIN;
	gpio_pin_t fsel=FSEL_PIN;
	
	val = freq | phase | type | cmdType;
	if(cmdType) {
		if(freq) {
			gpio_set_hl(&fsel, 1);
		}	
		else {
			gpio_set_hl(&fsel, 0);
		}
		
		if(phase) {
			gpio_set_hl(&psel, 1);
		}	
		else {
			gpio_set_hl(&psel, 0);
		}
	}
	write_reg(val);
}

