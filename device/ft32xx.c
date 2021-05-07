#include "ft32xx.h"
#include "drv/delay.h"
#include "drv/i2c.h"
#include "drv/gpio.h"



static handle_t ftHandle=NULL;
static gpio_pin_t ftPin={GPIOA, GPIO_PIN_0};

static int ft_read(U8 *cmd, U16 cmdlen, U8 *data, U16 len)
{
    int r;
    
    r = i2c_write(ftHandle, FT_ADDR, cmd, cmdlen);
    if(r==0) {
        r = i2c_read(ftHandle, FT_ADDR, data, len);
    }
    
    return r;
}
static int ft_write(U8 *data, U16 len)
{
    return i2c_write(ftHandle, FT_ADDR, data, len);
}
static int ft_read_reg(U8 addr, U8 *value)
{
    return ft_read(&addr, 1, value, 1);
}
static int ft_write_reg(U8 addr, U8 value)
{
    U8 buf[2] = {addr,  value};

    return ft_write(buf, sizeof(buf));
}
static int ft_reset(void)
{   
    gpio_set_hl(&ftPin, 0);
    delay_ms(10);
    gpio_set_hl(&ftPin, 1);
    delay_ms(300);
    
    return 0;
}



static int ft_read_chipid(U8 *id)
{
    int r;
    
    r = ft_write_reg(FTS_REG_WORKMODE,  1);
    
    r = ft_read_reg(FTS_REG_CHIP_ID,  &id[0]);
    r = ft_read_reg(FTS_REG_CHIP_ID2, &id[1]);
    
    return r;
}
static int ft_read_bootid(U8 *id)
{
    int r = 0;
    U8 chip_id[2] = { 0 };
    U8 id_cmd[4] = { 0 };
    U32 id_cmd_len = 0;

    id_cmd[0] = FTS_CMD_START1;
    id_cmd[1] = FTS_CMD_START2;
    r = ft_write(id_cmd, 2);
    if (r < 0) {
        return r;
    }

    delay_ms(FTS_CMD_START_DELAY);
    id_cmd[0] = FTS_CMD_READ_ID;
    id_cmd[1] = id_cmd[2] = id_cmd[3] = 0x00;
    id_cmd_len = FTS_CMD_READ_ID_LEN;
    r = ft_read(id_cmd, id_cmd_len, chip_id, 2);
    if ((r < 0) || (0x0 == chip_id[0]) || (0x0 == chip_id[1])) {
        return -1;
    }

    id[0] = chip_id[0];
    id[1] = chip_id[1];
    return 0;
}

//////////////////////////////////////////////////


int ft32xx_init(void)
{
    U16 chipid=0;
    i2c_cfg_t  ic;
    
    ic.pin.scl.grp = GPIOF;
    ic.pin.scl.pin = GPIO_PIN_1;
    
    ic.pin.sda.grp = GPIOF;
    ic.pin.sda.pin = GPIO_PIN_0;
    
    ic.freq = 50000;
    ic.useDMA = 0;;
    ic.callback = NULL;
    ic.finish_flag=0;
    
    ftHandle = i2c_init(&ic);
    gpio_init(&ftPin, MODE_OUTPUT);
    ft_reset();
    
    //ft_read_chipid((U8*)&chipid);
    ft32xx_test();
    
    return ftHandle?0:-1;
}


int ft32xx_test(void)
{
    U8 tmp=0x47;
    
    while(1) {
        i2c_write(ftHandle, FT_ADDR, &tmp, 1);
        //delay_ms(50);
    }
}

