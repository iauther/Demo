#include "bio/ads8866.h"
#include "drv/gpio.h"
#include "drv/spim.h"
#include "log.h"
#include "myCfg.h"



static handle_t ads8866_spi_handle;
int ads8866_init(void)
{
    int r;
    gpio_pin_t p={SPI_CS_PIN};
    spim_cfg_t cfg;
    
    cfg.pin.cs   = HAL_GPIO_MAX;//here we control cs by user
    cfg.pin.clk  = SPI_CLK_PIN;
    cfg.pin.miso = SPI_MISO_PIN;
    cfg.pin.mosi = SPI_MOSI_PIN;
    
    cfg.para.freq = AD8233_FREQ;
    cfg.para.lsb = HAL_SPI_MASTER_MSB_FIRST;
    cfg.para.cpha = HAL_SPI_MASTER_CLOCK_PHASE0;
    cfg.para.cpol = HAL_SPI_MASTER_CLOCK_POLARITY0;
    cfg.para.sport = HAL_SPI_MASTER_SLAVE_0;      //???
    cfg.para.endian = HAL_SPI_MASTER_LITTLE_ENDIAN;
    cfg.para.cs_polarity = HAL_SPI_MASTER_CHIP_SELECT_HIGH;
    cfg.para.delay_tick = HAL_SPI_MASTER_NO_GET_TICK_MODE;
    cfg.para.sample_edge = HAL_SPI_MASTER_SAMPLE_NEGATIVE;//HAL_SPI_MASTER_SAMPLE_POSITIVE;
    
    cfg.useDMA = 0;
    ads8866_spi_handle = spim_init(&cfg);
    LOGI("ads8866_init %s\r\n", (ads8866_spi_handle)?"ok":"failed");
    
    gpio_init(&p, MODE_OUTPUT);
    
    return ads8866_spi_handle?0:-1;
}


int ads8866_read(U8 *data, U32 len)
{
    int r;
    gpio_pin_t p={SPI_CS_PIN};
    
    gpio_set_hl(&p, 1);
    //r = spim_read(ads8866_spi_info.port, data, len);
    r = spim_read(ads8866_spi_handle, data, len);
    gpio_set_hl(&p, 0);
    return r; 
}


int ads8866_check(void)
{
    int r;
    U8  tmp[8];
    gpio_pin_t p={SPI_CS_PIN};
    
    gpio_set_hl(&p, 1);
    r = spim_read(ads8866_spi_handle, tmp, sizeof(tmp));
    gpio_set_hl(&p, 0);
    return r; 
}


int ads8866_reset(void)
{
    return ads8866_init();
}

