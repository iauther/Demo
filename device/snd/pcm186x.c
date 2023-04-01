#include "dal/si2c.h"
#include "dal/delay.h"
#include "snd/pcm186x.h"
#include "dal/gpio.h"
#include "cfg.h"


extern handle_t i2c1Handle;
extern handle_t i2c2Handle;
static U8 pcmI2cAddr[PCM186X_ID_MAX]={0x94>>1, 0x96>>1};

static int read_reg(handle_t hi2c, U8 id, U8 reg, U8 *value)
{
    int r=0;
    
    r = si2c_write(hi2c, pcmI2cAddr[id], &reg, 1, 0);
    if(r==0) {
        r = si2c_read(hi2c, pcmI2cAddr[id], value, 1, 1);
    }
    
    return r;
}
static int write_reg(handle_t hi2c, U8 id, U8 reg, U8 value)
{
    U8 tmp[2]={reg,value};
    
    return si2c_write(hi2c, pcmI2cAddr[id], tmp, 2, 1);
}


static int set_config(handle_t hi2c, U8 id)
{
    int r;
    U8 u8RegVal,u8Timeout;
    
    r = write_reg(hi2c, id, PCM186X_ADC1_INPUT_SEL_L, 0x50); // VIN1P, VIN1M
    r = write_reg(hi2c, id, PCM186X_ADC1_INPUT_SEL_R, 0x50); // VIN2P, VIN2M
    r = write_reg(hi2c, id, PCM186X_ADC2_INPUT_SEL_L, 0x60); // VIN4P, VIN4M
    r = write_reg(hi2c, id, PCM186X_ADC2_INPUT_SEL_R, 0x60); // VIN3P, VIN3M
    
    // GPIO Input
    r = write_reg(hi2c, id, PCM186X_GPIO1_0_CTRL, 0x00); // GPIO0, GPIO1
    r = write_reg(hi2c, id, PCM186X_GPIO3_2_CTRL, 0x00); // GPIO2, GPIO3
    
    // Clock设置
    r = write_reg(hi2c, id, PCM186X_CLK_CTRL, 0x4E); // slave, ADC: PLL, DSP1:PLL, DSP2:PLL, 禁用时钟自动配置
    r = write_reg(hi2c, id, PCM186X_PCM_CFG, 0x57); // 24bit, TDM, duty cycle of LRCK is 1/256
    r = write_reg(hi2c, id, PCM186X_PLL_CTRL, 0x11); // PLL Reference Clock: SCK; Enable PLL
    r = write_reg(hi2c, id, PCM186X_TDM_TX_SEL, 0x01); // DOUT1: ch1[L], ch1[R], ch2[L], ch2[R]
    r = write_reg(hi2c, id, PCM186X_TDM_TX_OFFSET, (id==PCM186X_ID_1)?0x00:0x60); // 第1个ADC输出偏移0个BCLK
    r = write_reg(hi2c, id, PCM186X_DSP1_CLK_DIV, 0x02); // DSP1 Clock Divider: 1/3
    r = write_reg(hi2c, id, PCM186X_DSP2_CLK_DIV, 0x05); // DSP2 Clock Divider: 1/6
    r = write_reg(hi2c, id, PCM186X_ADC_CLK_DIV, 0x0B); // ADC Clock Divider: 1/12
    r = write_reg(hi2c, id, PCM186X_PLL_SCK_DIV, 0x05); // PLL SCK Clock Output Divider for SCK Out: 1/6
    r = write_reg(hi2c, id, PCM186X_BCK_DIV, 0x00); // Master Clock (SCK) to BCK Divider: 1
    r = write_reg(hi2c, id, PCM186X_LRK_DIV, 0xFF); // Bit Clock (BCK) to LRCK Divider: 1/256
    r = write_reg(hi2c, id, PCM186X_PLL_P_DIV, 0x03); // PLL P=4
    r = write_reg(hi2c, id, PCM186X_PLL_R_DIV, 0x01); // PLL R=2
    r = write_reg(hi2c, id, PCM186X_PLL_J_DIV, 0x0C); // PLL J=12
    r = write_reg(hi2c, id, PCM186X_PLL_D_DIV_MSB, 0x00); // PLL D_MSB=0
    r = write_reg(hi2c, id, PCM186X_PLL_D_DIV_LSB, 0x00); // PLL D_LSB=0
    
    // PGA Gain设置
    r = write_reg(hi2c, id, PCM186X_PGA_GAIN_CTRL, 0xFF); //  Manual gain mapping
    r = write_reg(hi2c, id, PCM186X_PGA_CTRL, 0x06); // PGA Immediate change
    r = write_reg(hi2c, id, PCM186X_PGA_VAL_CH1_L, 0x08); // ADC1L Analog gain 4dB
    r = write_reg(hi2c, id, PCM186X_PGA_VAL_CH1_R, 0x08); // ADC1R Analog gain 4dB
    r = write_reg(hi2c, id, PCM186X_PGA_VAL_CH2_L, 0x08); // ADC2L Analog gain 4dB
    r = write_reg(hi2c, id, PCM186X_PGA_VAL_CH2_R, 0x08); // ADC2R Analog gain 4dB
    
    r = write_reg(hi2c, id, PCM186X_INT_ENABLE, 0x00); // 禁用所有中断
    
    r = write_reg(hi2c, id, PCM186X_FILTER_MUTE_CTRL, 0x00); // FIR, 禁用高通滤波, 所有通道不静音
    
    // select Page 3
    r = write_reg(hi2c, id, PCM186X_PAGE, 3);
    
    r = write_reg(hi2c, id, PCM186X_MIC_BIAS_CTRL, 0x00); // Mic Bias Power down
    
    // MIX1_CH1L 0dB -> -4dB
    r = write_reg(hi2c, id, PCM186X_PAGE, 1);
    r = write_reg(hi2c, id, PCM186X_PAGE, 1);
    r = write_reg(hi2c, id, PCM186X_PAGE, 1);
    
    u8RegVal = 0xFF;
    u8Timeout = 0;
    r = read_reg(hi2c, id, PCM186X_MMAP_STAT_CTRL, &u8RegVal);
    while (u8RegVal != 0x00) {
        u8Timeout++;
        if (!r || u8Timeout > 10)
            return -1;
        r = write_reg(hi2c, id, PCM186X_PAGE, 1);
        u8RegVal = 0xFF;
        r = read_reg(hi2c, id, PCM186X_MMAP_STAT_CTRL, &u8RegVal);
    }
    r = write_reg(hi2c, id, PCM186X_MMAP_ADDRESS, 0x00); // MIX1_CH1L
    r = write_reg(hi2c, id, PCM186X_MEM_WDATA0, 0x0A); // -4dB
    r = write_reg(hi2c, id, PCM186X_MEM_WDATA1, 0x18);
    r = write_reg(hi2c, id, PCM186X_MEM_WDATA2, 0x66);
    r = write_reg(hi2c, id, PCM186X_MMAP_STAT_CTRL, 0x01); // execute write operation
    
    // MIX2_CH1R 0dB -> -4dB
    r = write_reg(hi2c, id, PCM186X_PAGE, 1);
    r = write_reg(hi2c, id, PCM186X_PAGE, 1);
    r = write_reg(hi2c, id, PCM186X_PAGE, 1);
    
    u8RegVal = 0xFF;
    u8Timeout = 0;
    r = read_reg(hi2c, id, PCM186X_MMAP_STAT_CTRL, &u8RegVal);
    while (u8RegVal != 0x00) {
        u8Timeout++;
        if (!r || u8Timeout > 10)
            return -1;
        r = write_reg(hi2c, id, PCM186X_PAGE, 1);
        u8RegVal = 0xFF;
        r = read_reg(hi2c, id, PCM186X_MMAP_STAT_CTRL, &u8RegVal);
    }
    r = write_reg(hi2c, id, PCM186X_MMAP_ADDRESS, 0x07); // MIX2_CH1R
    r = write_reg(hi2c, id, PCM186X_MEM_WDATA0, 0x0A); // -4dB
    r = write_reg(hi2c, id, PCM186X_MEM_WDATA1, 0x18);
    r = write_reg(hi2c, id, PCM186X_MEM_WDATA2, 0x66);
    r = write_reg(hi2c, id, PCM186X_MMAP_STAT_CTRL, 0x01); // execute write operation
    
    // MIX3_CH2L 0dB -> -4dB
    r = write_reg(hi2c, id, PCM186X_PAGE, 1);
    r = write_reg(hi2c, id, PCM186X_PAGE, 1);
    r = write_reg(hi2c, id, PCM186X_PAGE, 1);
    
    u8RegVal = 0xFF;
    u8Timeout = 0;
    r = read_reg(hi2c, id, PCM186X_MMAP_STAT_CTRL, &u8RegVal);
    while (u8RegVal != 0x00) {
        u8Timeout++;
        if (!r || u8Timeout > 10)
            return -1;
        r = write_reg(hi2c, id, PCM186X_PAGE, 1);
        u8RegVal = 0xFF;
        r = read_reg(hi2c, id, PCM186X_MMAP_STAT_CTRL, &u8RegVal);
    }
    r = write_reg(hi2c, id, PCM186X_MMAP_ADDRESS, 0x0E); // MIX3_CH2L
    r = write_reg(hi2c, id, PCM186X_MEM_WDATA0, 0x0A); // -4dB
    r = write_reg(hi2c, id, PCM186X_MEM_WDATA1, 0x18);
    r = write_reg(hi2c, id, PCM186X_MEM_WDATA2, 0x66);
    r = write_reg(hi2c, id, PCM186X_MMAP_STAT_CTRL, 0x01); // execute write operation
    
    // MIX4_CH2R 0dB -> -4dB
    r = write_reg(hi2c, id, PCM186X_PAGE, 1);
    r = write_reg(hi2c, id, PCM186X_PAGE, 1);
    r = write_reg(hi2c, id, PCM186X_PAGE, 1);
    
    u8RegVal = 0xFF;
    u8Timeout = 0;
    r = read_reg(hi2c, id, PCM186X_MMAP_STAT_CTRL, &u8RegVal);
    while (u8RegVal != 0x00) {
        u8Timeout++;
        if (!r || u8Timeout > 10)
            return -1;
        r = write_reg(hi2c, id, PCM186X_PAGE, 1);
        u8RegVal = 0xFF;
        r = read_reg(hi2c, id, PCM186X_MMAP_STAT_CTRL, &u8RegVal);
    }
    r = write_reg(hi2c, id, PCM186X_MMAP_ADDRESS, 0x15); // MIX4_CH2R
    r = write_reg(hi2c, id, PCM186X_MEM_WDATA0, 0x0A); // -4dB
    r = write_reg(hi2c, id, PCM186X_MEM_WDATA1, 0x18);
    r = write_reg(hi2c, id, PCM186X_MEM_WDATA2, 0x66);
    r = write_reg(hi2c, id, PCM186X_MMAP_STAT_CTRL, 0x01); // execute write operation
    
    return r;
}



static int sw_reset(handle_t hi2c, U8 id)
{
    int r;
    U8 status=0;
    
    r = write_reg(hi2c, id, PCM186X_PAGE, 0);
    if (r) {
        return -1;
    }
    
    // check AVDD/DVDD/LDO Status
    r = read_reg(hi2c, id, PCM186X_SUPPLY_STATUS, &status);
    if (r || status != 0x07) {
        return -1;
    }
    
    r = write_reg(hi2c, id, PCM186X_PAGE, PCM186X_RESET);
    if (r) {
        return -1;
    }
    
    return 0;
}



int pcm186x_init(void)
{
    int r;     
    
    r = sw_reset(i2c1Handle, PCM186X_ID_1);
    if(r) {
        return r;
    }
    set_config(i2c1Handle, PCM186X_ID_1);
    
    r = sw_reset(i2c1Handle, PCM186X_ID_2); 
    if(r) {
        return r;
    }
    set_config(i2c1Handle, PCM186X_ID_2);
    
    return 0;
}


int pcm186x_config(void)
{
    int r;
    
    r = set_config(i2c1Handle, PCM186X_ID_1);
    r |= set_config(i2c1Handle, PCM186X_ID_2);
    
    return r;
}


int pcm186x_get_status(U8 *status)
{
    int r;
    r = write_reg(i2c1Handle, PCM186X_ID_1, PCM186X_PAGE, 0);
    if(r==0) {
        r = read_reg(i2c1Handle, PCM186X_ID_1, PCM186X_PLL_CTRL, status);
    }
    
    r = write_reg(i2c1Handle, PCM186X_ID_2, PCM186X_PAGE, 0);
    if(r==0) {
        r = read_reg(i2c1Handle, PCM186X_ID_2, PCM186X_PLL_CTRL, status);
    }
    
    return r;
}


