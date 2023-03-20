#include "dal/si2c.h"
#include "dal/delay.h"
#include "snd/adau197x.h"
#include "dal/gpio.h"


extern handle_t i2c1Handle;
extern handle_t i2c2Handle;
static U8 adauI2cAddr[ADAU197X_ID_MAX]={0x11, 0x31};


static int read_reg(U8 id, U8 reg, U8 *value)
{
    handle_t h[2]={i2c1Handle,i2c2Handle};
    
    si2c_write(h[id], adauI2cAddr[id], &reg, 1);
    return si2c_read(h, adauI2cAddr[id], value, 1);
}
static int write_reg(U8 id, U8 reg, U8 value)
{
    U8 tmp[2]={reg,value};
    handle_t h[2]={i2c1Handle,i2c2Handle};
    
    return si2c_write(h[id], adauI2cAddr[id], tmp, 2);
}


static void io_init()
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOH_CLK_ENABLE();					// 开启GPIOH时钟
	__HAL_RCC_GPIOG_CLK_ENABLE();					// 开启GPIOG时钟
    
    GPIO_InitStruct.Pin = GPIO_PIN_3;				// PH3
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;  	// 推挽输出
    GPIO_InitStruct.Pull = GPIO_NOPULL;         	// 没有上/下拉
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;  	// 低速
    HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);     	// 初始化GPIOH.3
    
    GPIO_InitStruct.Pin = GPIO_PIN_10;				// PG10
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;  	// 推挽输出
    GPIO_InitStruct.Pull = GPIO_NOPULL;         	// 没有上/下拉
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;  	// 低速
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);     	// 初始化GPIOG.10
    
	HAL_GPIO_WritePin(GPIOH, GPIO_PIN_3, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_10, GPIO_PIN_SET);
	delay_us(500); // 发送I2C控制信号之前，至少等待tC(规格书要求200us)时间
}

static void hw_reset()
{
    HAL_GPIO_WritePin(GPIOH, GPIO_PIN_3, GPIO_PIN_RESET);  
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_10, GPIO_PIN_RESET);  
	delay_ms(100); // tD(规格书要求37.8ms)时间内，复位脉冲必须保持低电平，才能使内核正确初始化
	HAL_GPIO_WritePin(GPIOH, GPIO_PIN_3, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_10, GPIO_PIN_SET);
	delay_us(500); // 发送I2C控制信号之前，至少等待tC(规格书要求200us)时间

}

static int power_on(U8 id, U8 onoff)
{
    return write_reg(id, M_POWER_REG, onoff);
}
static int set_config(U8 id)
{
    int r;
    /*****************************   ADC1配置    ******************************/

    /* B[6]		1 - PLL未锁定时自动静音
       B[4]		0 - MCLK作为PLL输入
       B[2:0]	1 - 主时钟为：256 x Fs 
       采用默认设置0x41 */
    
    /* BLOCK_POWER_SAI 采用默认设置0x3F
        B[7]	0 - LR_POL，LRCLK极性设为先低后高
        B[6]	0 - BCLKEDGE，数据在时钟下降沿改变
        B[5]	1 - LDO使能
        B[4]	1 - 基准电源使能
        B[3:0]	F - 使能所有4个通道 */
    
    /* B[7:6]	1 - SDATA_FMT，串行数据左对齐
       B[5:3]	3 - SAI，采用TDM8方式
       B[2:0]	2 - FS，采样频率参考32kHz~48kHz区间，实际为51.2kHz */
    r = write_reg(id, SAI_CTRL0_REG, 0x5A);
    
    /*	B[7]	0 - SDATA_SEL，SDATAOUT1用于输出
        B[6:5]	1 - SLOT_WIDTH，每个时隙24个BCLK
        B[4]	0 - DATA_WIDTH, 输出数据位宽为24bit
        B[3]	1 - LR_MODE，设置LRCLK模式为脉冲模式，为单BCLK周期脉冲
        B[2]	0 - SAI_MSB，设置数据已MSB优先方式
        B[1]	0 - BCLKRATE，不工作在主模式
        B[0]	0 - SAI_MS，设置工作在从模式下 */
    r = write_reg(id, SAI_CTRL1_REG, 0x28);
    
    /* SAI_CMAP12，采用默认设置0x10
       SAI_CMAP34，采用默认设置0x32 */
       
    /* 	B[7:4]	F - SAI_DRV，配置所有通道均驱动到数据输出总线
        B[3]	1 - DRV_HIZ，未用的SAI通道配置为高阻模式（与其他ADC共用）
        B[0]	过温故障状态，可在数据异常时读取本位确定是否过温 */
    r = write_reg(id, SAI_OVERTEMP_REG, 0xF8);
    /* MISC_CONTROL，采用默认设置0x02 */
    /* DC_HPF_CAL，采用默认设置0x00 */
    
    return r;
}



int adau197x_init(U8 id)
{
    io_init();
    hw_reset();
    
    power_on(id, 1);
    
    return 0;
}


int adau197x_config(U8 id)
{
    return set_config(id);
}



int adau1978_get_status(U8 id, U8 *status)
{
    return read_reg(id, SAI_OVERTEMP_REG, status);
}
