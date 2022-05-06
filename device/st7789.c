#include "st7789.h"
#include "drv/spi.h"
#include "drv/gpio.h"
#include "drv/delay.h"
#include "myCfg.h"



typedef struct {
    U8      type;
    U8      data;
}st_reg_t;

enum {
    PIN_RST=0,
    PIN_CMD,
    PIN_BLK,
    
    PIN_MAX
};


static handle_t pinHandle[PIN_MAX]={NULL};


static st_reg_t init_data[]={
    
    {ST7789_CMD, 0x11},
    
    {ST7789_DLY, 150},
    
    /*  Memory Data Access Control:
        D7  MY,  page addr order         0: top to bottom              1: bottom to top
        D6  MX,  colum addr order        0: left to right              1: right to left
        D5  MV,  page/column order       0: normal mode                1: reverse mdoe
        D4  ML,  line address order      0: refresh top to bottom      1: refresh bottom to top
        D3  RGB, RGB order:              0: RGB                        1: BGR
        D2  MH,  data latch data order   0: refresh left to right      1: refresh right to left
    */
    {ST7789_CMD, 0x36},
    {ST7789_DAT, 0x70},
    
    /* RGB 5-6-5-bit  */
    {ST7789_CMD, 0x3A},
    {ST7789_DAT, 0x05},
    
    {ST7789_CMD, 0xb2},
    {ST7789_DAT, 0x0c},
    {ST7789_DAT, 0x0c},
    {ST7789_DAT, 0x00},
    {ST7789_DAT, 0x33},
    {ST7789_DAT, 0x33},
    
    {ST7789_CMD, 0xb7},
    {ST7789_DAT, 0x35},
    
    {ST7789_CMD, 0xbb},
    {ST7789_DAT, 0x19},
    
    {ST7789_CMD, 0xc0},
    {ST7789_DAT, 0x2c},
    
    {ST7789_CMD, 0xc2},
    {ST7789_DAT, 0x01},
    
    {ST7789_CMD, 0xc3},
    {ST7789_DAT, 0x12},
    
    {ST7789_CMD, 0xc4},
    {ST7789_DAT, 0x20},
    
    {ST7789_CMD, 0xc6},
    {ST7789_DAT, 0x0f},
    
    {ST7789_CMD, 0xd0},
    {ST7789_DAT, 0xa4},
    {ST7789_DAT, 0xa1},
    
    
    /* Positive Voltage Gamma Control */
    {ST7789_CMD, 0xE0},
    {ST7789_DAT, 0xD0},
    {ST7789_DAT, 0x04},
    {ST7789_DAT, 0x0D},
    {ST7789_DAT, 0x11},
    {ST7789_DAT, 0x13},
    {ST7789_DAT, 0x2B},
    {ST7789_DAT, 0x3F},
    {ST7789_DAT, 0x54},
    {ST7789_DAT, 0x4C},
    {ST7789_DAT, 0x18},
    {ST7789_DAT, 0x0D},
    {ST7789_DAT, 0x0B},
    {ST7789_DAT, 0x1F},
    {ST7789_DAT, 0x23},

    /* Negative Voltage Gamma Control */
    {ST7789_CMD, 0xE1},
    {ST7789_DAT, 0xD0},
    {ST7789_DAT, 0x04},
    {ST7789_DAT, 0x0D},
    {ST7789_DAT, 0x11},
    {ST7789_DAT, 0x13},
    {ST7789_DAT, 0x2B},
    {ST7789_DAT, 0x3F},
    {ST7789_DAT, 0x54},
    {ST7789_DAT, 0x4C},
    {ST7789_DAT, 0x18},
    {ST7789_DAT, 0x0D},
    {ST7789_DAT, 0x0B},
    {ST7789_DAT, 0x1F},
    {ST7789_DAT, 0x23},
    {ST7789_CMD, 0x21},
    
    {ST7789_CMD, 0x29},

    {ST7789_MAX, 0},
};


static void set_regs(st_reg_t *reg)
{
    int i=0;
    
    while(reg[i].type<ST7789_MAX) {
        if(reg[i].type==ST7789_DLY) {
            delay_ms(reg[i].data);
        }
        else {
            st7789_write(reg[i].type, &reg[i].data, 1);
            //delay_us(5);
        }
        
        i++;
    }   
}

static int set_rect(U16 x1, U16 y1, U16 x2, U16 y2)
{
    st_reg_t regs[]={
        {ST7789_CMD, 0x2a},
        {ST7789_DAT, x1>>8},
        {ST7789_DAT, x1},
        {ST7789_DAT, x2>>8},
        {ST7789_DAT, x2},

        {ST7789_CMD, 0x2b},
        {ST7789_DAT, y1>>8},
        {ST7789_DAT, y1},
        {ST7789_DAT, y2>>8},
        {ST7789_DAT, y2},
        
        {ST7789_CMD, 0x2c},
        
        {ST7789_MAX, 0}
        
    };
    
    set_regs(regs);
    
    return 0;
}




static void io_init(void)
{
    int i;
    gpio_cfg_t cfg;
    gpio_pin_t p[PIN_MAX]={
        LCD_RST_PIN,
        LCD_CMD_PIN,
        LCD_BLK_PIN,

    };

    for(i=0; i<PIN_MAX; i++) {
        
        cfg.pin = p[i];
        cfg.mode = MODE_OUTPUT;
        pinHandle[i] = gpio_init(&cfg);
        gpio_set_hl(pinHandle[i], (i==PIN_BLK)?0:1);
    }
}


static handle_t stHandle=NULL;
static void hw_init(void)
{
    spi_cfg_t cfg;
    
    cfg.port = SPI_1;
    cfg.init.Mode = SPI_MODE_MASTER;
    cfg.init.Direction = SPI_DIRECTION_2LINES;
    cfg.init.DataSize = SPI_DATASIZE_8BIT;
    cfg.init.CLKPolarity = SPI_POLARITY_LOW;
    cfg.init.CLKPhase = SPI_PHASE_1EDGE;
    cfg.init.NSS = SPI_NSS_SOFT;
    cfg.init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    cfg.init.FirstBit = SPI_FIRSTBIT_MSB;
    cfg.init.TIMode = SPI_TIMODE_DISABLE;
    cfg.init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    cfg.init.CRCPolynomial = 10;
    
    stHandle = spi_init(&cfg);
}




static int st_write(U8 *data, U32 len)
{
    return spi_write(stHandle, data, len, HAL_MAX_DELAY);
}



static void port_init(void)
{
    io_init();
    hw_init(); 
    
#if 0
    while(1) {
        U8 tmp=0x88;
        st_write(&tmp, 1);
    }
#endif
    
}


static int set_blk(U8 onoff)
{
    gpio_set_hl(pinHandle[PIN_BLK], onoff);
    return 0;
}


static void set_dc(U8 mode)
{
    U8 hl=(mode==ST7789_CMD)?0:1;
    
    gpio_set_hl(pinHandle[PIN_CMD], mode);
}

static int st_reset(void)
{
    gpio_pin_t p=LCD_RST_PIN;
    
    gpio_set_hl(pinHandle[PIN_RST], 0);
    delay_ms(100);
    gpio_set_hl(pinHandle[PIN_RST], 1);
    delay_ms(5000);
    
    return 0;
}

int st7789_init(void)
{
    port_init();
    st_reset();
    set_regs(init_data);
    
    return 0;
}



int st7789_write(U8 mode, U8 *data, U32 len)
{
    set_dc(mode);
    st_write(data, len);
    
    return 0;
}


int st7789_set_rect(U16 x1, U16 y1, U16 x2, U16 y2)
{
    return set_rect(x1, y1, x2, y2);
}


void st7789_set_blk(U8 on)
{
    set_blk(on);
}




