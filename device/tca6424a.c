#include "tca6424a.h"
#include "bio/bio.h"
#include "drv/gpio.h"
#include "drv/delay.h"
#include "drv/i2c.h"
#include "log.h"
#include "myCfg.h"


extern handle_t i2c0Handle;

int tca6424a_init(void)
{
#ifdef BOARD_V00_02
    i2c_cfg_t cfg;
    
    cfg.pin.scl.pin = TEA6424A_I2C_SCL_PIN;
    cfg.pin.sda.pin = TEA6424A_I2C_SDA_PIN;
    cfg.freq = TEA6424A_I2C_FREQ;
    cfg.useDMA = 0;
    i2c0Handle = i2c_init(&cfg);
#endif
    return 0;
}


int tca6424a_reset(TCA6424A_ADDR addr)
{
    //gpio_pin_t reset_pin={(addr==TCA6424A_ADDR_L)?TCA6424A_L_RESET_PIN:TCA6424A_H_RESET_PIN};
    
	//gpio_init(&reset_pin, MODE_OUTPUT, 0);
    //delay_ms(1);
	//gpio_set(&reset_pin, 1);
    
    return 0;
}

static tca6424a_regs_t tca6424a_regs;
int tca6424a_read_reg(TCA6424A_ADDR addr, tca6424a_reg_t *reg)
{
    int r=0;
    if(reg) {
        r = tca6424a_read(addr, TCA6424A_INPUT_REG0, reg->input, sizeof(reg->input));
        r |= tca6424a_read(addr, TCA6424A_OUTPUT_REG0, reg->output, sizeof(reg->output));
        r |= tca6424a_read(addr, TCA6424A_POLARITY_REG0, reg->polarity, sizeof(reg->polarity));
        r |= tca6424a_read(addr, TCA6424A_CONFIG_REG0, reg->config, sizeof(reg->config));
    }
    
    return r;
}

int tca6424a_write_reg(TCA6424A_ADDR addr, tca6424a_reg_t *reg)
{
    int r=0;
    if(reg) {
        r = tca6424a_write(addr, TCA6424A_INPUT_REG0, reg->input, sizeof(reg->input));
        r |= tca6424a_write(addr, TCA6424A_OUTPUT_REG0, reg->output, sizeof(reg->output));
        r |= tca6424a_write(addr, TCA6424A_POLARITY_REG0, reg->polarity, sizeof(reg->polarity));
        r |= tca6424a_write(addr, TCA6424A_CONFIG_REG0, reg->config, sizeof(reg->config));
        
        if(addr==TCA6424A_ADDR_H) tca6424a_regs.h = *reg;
        else                      tca6424a_regs.l = *reg;
    }
    
    return r;
}


void tca6424a_print(char *s, tca6424a_reg_t *reg)
{
    if(reg) {
        LOGD("%s input   : 0x%02x, 0x%02x, 0x%02x \r\n", s, reg->input[0], reg->input[1], reg->input[2]);
        LOGD("%s output  : 0x%02x, 0x%02x, 0x%02x \r\n", s, reg->output[0], reg->output[1], reg->output[2]);
        LOGD("%s polarity: 0x%02x, 0x%02x, 0x%02x \r\n", s, reg->polarity[0], reg->polarity[1], reg->polarity[2]);
        LOGD("%s config  : 0x%02x, 0x%02x, 0x%02x \r\n", s, reg->config[0], reg->config[1], reg->config[2]);
    }
}


void tca6424a_print_reg(char *s, TCA6424A_ADDR addr)
{
    tca6424a_reg_t reg;
    tca6424a_read_reg(addr, &reg);
    tca6424a_print(s, &reg);
}


int tca6424a_read(TCA6424A_ADDR addr, U8 reg, U8 *data, U32 len)
{
    int r;
    U8 cmd = 0x80|reg;;
    
    r = i2c_write(i2c0Handle, addr, &cmd, 1, 1);
    //LOGD("______ i2c_write r: %d\r\n", r);
    r |= i2c_read(i2c0Handle, addr, data, len, 1);
    //LOGI("______ i2c_read r: %d\r\n", r);
    
    return r;
}


int tca6424a_write(TCA6424A_ADDR addr, U8 reg, U8 *data, U32 len)
{
    int r;
    U8 tmp[len+1];
    
    tmp[0] = 0x80|reg;
    memcpy(tmp+1, data, len);
    r = i2c_write(i2c0Handle, addr, tmp, len+1, 1);
    
    return r;
}


int tca6424a_check(void)
{
    int r;
    U8 config_h[3];
    U8 config_l[3];
    
    if(bioDevice==-1) {
        return 0;
    }
    
    if(bioDevice==AD8233) {
        r  = tca6424a_read(TCA6424A_ADDR_H, TCA6424A_CONFIG_REG0, config_h, 3);
        r |= tca6424a_read(TCA6424A_ADDR_L, TCA6424A_CONFIG_REG0, config_l, 3);
        
        if(r) {
            return -1;
        }
        
        if(memcmp(tca6424a_regs.h.config, config_h, 3) || memcmp(tca6424a_regs.l.config, config_l, 3)) {
            return -1;
        }
    }
    else {
        r = tca6424a_read(TCA6424A_ADDR_L, TCA6424A_CONFIG_REG0, config_l, 3);
        
        if(r) {
            return -1;
        }
        
        if(memcmp(tca6424a_regs.l.config, config_l, 3)) {
            return -1;
        }
    }
    
    return 0;
}




