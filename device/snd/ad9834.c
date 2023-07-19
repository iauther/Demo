//https://blog.csdn.net/qq_36024066/article/details/89667751
//https://blog.csdn.net/weixin_46708355/article/details/108000440


#include "ad9834.h"		//ad9834 definitions
#include "drv/spi.h"
#include "drv/gpio.h"
#include "drv/dac.h"
#include "drv/delay.h"
#include "myCfg.h"


/* GPIO pins */
#define PSEL_LOW        	// Add your code here
#define PSEL_HIGH       	// Add your code here
#define FSEL_LOW        	// Add your code here
#define FSEL_HIGH       	// Add your code here

typedef struct {
    
    handle_t    hspi;
    handle_t    hdac;
    
    ad9834_para_t para;
}ad9834_handle_t;
static ad9834_handle_t adHandle={0};


static void ad_gpio_init(void)
{
    
}
//static int ad_spi_init(U8 lsbFirst, U32 clockFreq, U8 clockPol, U8 clockPha)
//SPI_Init(0, 1000000, 0, 1);
static int ad_spi_init(void)
{
    spi_cfg_t cfg;
    
    cfg.port = AD9834_SPI_PORT;
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
    
    adHandle.hspi = spi_init(&cfg);

    return 0;
}
static int ad_dac_init(void)
{
    dac_cfg_t cfg;
    
    adHandle.hdac = dac_init(&cfg);
    
    return 0;
}




static int ad_write(U8* data, U8 bytesNumber)
{
    spi_write(adHandle.hspi, data, bytesNumber, 100);
    return 0;
}
static int ad_read(U8* data, U8 bytesNumber)
{
    spi_read(adHandle.hspi, data, bytesNumber, 100);
    return 0;
}
static void write_reg(U16 value)
{
	U8 data[2];	
	
	data[0] = (U8)((value & 0xFF00) >> 8);
	data[1] = (U8)((value & 0x00FF));
	
	//ADI_CS_LOW;	    
	ad_write(data, 2);
	//ADI_CS_HIGH;
}
static int ad_reset(void)
{
    write_reg(AD9834_REG_CMD | AD9834_RESET);
    delay_ms(1);
    write_reg(AD9834_REG_CMD);
    
    return 0;
}

//////////////////////////////////////////////////
int ad9834_init(void)
{
    ad_gpio_init();
    ad_spi_init();
    ad_dac_init();
    
    write_reg(AD9834_REG_CMD | AD9834_RESET);
	
    return 0;
}



int ad9834_reset(void)
{
    ad_reset();
    
    return 0;
}


static void set_type(U8 type)
{
    U16 val[AD9834_TYPE_MAX]={0,0,0};

    if(type != adHandle.para.type) {
        write_reg(AD9834_REG_CMD | val[type]);
        
        adHandle.para.type = type;
    }
}
static void set_freq(U32 freq)
{
    U16 fh,fl;
	
    if(freq != adHandle.para.freq) {
        fh = AD9834_REG_FREQ0 | ((freq & 0xFFFC000) >> 14);
        fl = AD9834_REG_FREQ1 | (freq & 0x3FFF);
        write_reg(AD9834_B28);
        write_reg(fh);
        write_reg(fl);
        adHandle.para.freq = freq;
    }
}

static void set_phase(U16 phase)
{
	U16 v = AD9834_REG_PHASE0|phase;
	
    if(phase != adHandle.para.phase) {
        write_reg(phase);
        adHandle.para.phase = phase;
    }
}

int set_amplitude(F32 amp)
{
    if(amp != adHandle.para.amplitude) {
        dac_set(adHandle.hdac, amp);
        adHandle.para.amplitude = amp;
    }
    
    return 0;
}


int ad9834_set(ad9834_para_t *ad)
{
	if(!ad) {
        return -1;
    }
    
    set_type(ad->type);
    set_freq(ad->freq);
    set_phase(ad->phase);
    set_amplitude(ad->amplitude);
    
    return 0;
}






