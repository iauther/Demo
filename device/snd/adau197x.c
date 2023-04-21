#include "dal_si2c.h"
#include "dal_delay.h"
#include "snd/adau197x.h"
#include "dal_gpio.h"
#include "cfg.h"


extern handle_t i2c1Handle;
extern handle_t i2c2Handle;
static U8 adauI2cAddr[ADAU197X_ID_MAX]={0x11, 0x31};


static int read_reg(handle_t hi2c, U8 id, U8 reg, U8 *value)
{
    int r;
    
    r = dal_si2c_write(hi2c, adauI2cAddr[id], &reg, 1, 0);
    if(r==0) {
        r = dal_si2c_read(hi2c, adauI2cAddr[id], value, 1, 1);
    }
    
    return r;
}
static int write_reg(handle_t hi2c, U8 id, U8 reg, U8 value)
{
    U8 tmp[2]={reg,value};
    
    return dal_si2c_write(hi2c, adauI2cAddr[id], tmp, 2, 1);
}


static void io_init(void)
{
    GPIO_InitTypeDef init = {0};
    __HAL_RCC_GPIOH_CLK_ENABLE();					// ����GPIOHʱ��
	__HAL_RCC_GPIOG_CLK_ENABLE();					// ����GPIOGʱ��
    
    init.Pin = GPIO_PIN_3;				// PH3
    init.Mode = GPIO_MODE_OUTPUT_PP;  	// �������
    init.Pull = GPIO_NOPULL;         	// û����/����
    init.Speed = GPIO_SPEED_FREQ_LOW;  	// ����
    HAL_GPIO_Init(GPIOH, &init);     	// ��ʼ��GPIOH.3
    
    init.Pin = GPIO_PIN_10;				// PG10
    init.Mode = GPIO_MODE_OUTPUT_PP;  	// �������
    init.Pull = GPIO_NOPULL;         	// û����/����
    init.Speed = GPIO_SPEED_FREQ_LOW;  	// ����
    HAL_GPIO_Init(GPIOG, &init);     	// ��ʼ��GPIOG.10
    
	HAL_GPIO_WritePin(GPIOH, GPIO_PIN_3, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_10, GPIO_PIN_SET);
	dal_delay_us(500); // ����I2C�����ź�֮ǰ�����ٵȴ�tC(�����Ҫ��200us)ʱ��
}

static void hw_reset(void)
{
    HAL_GPIO_WritePin(GPIOH, GPIO_PIN_3, GPIO_PIN_RESET);  
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_10, GPIO_PIN_RESET);  
	dal_delay_ms(100); // tD(�����Ҫ��37.8ms)ʱ���ڣ���λ������뱣�ֵ͵�ƽ������ʹ�ں���ȷ��ʼ��
	HAL_GPIO_WritePin(GPIOH, GPIO_PIN_3, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_10, GPIO_PIN_SET);
	dal_delay_us(500); // ����I2C�����ź�֮ǰ�����ٵȴ�tC(�����Ҫ��200us)ʱ��

}

static int power_on(handle_t hi2c, U8 id, U8 onoff)
{
    return write_reg(hi2c, id, M_POWER_REG, onoff);
}
static int set_config(handle_t hi2c, U8 id)
{
    int r;
    /*****************************   ADC1����    ******************************/

    /* B[6]		1 - PLLδ����ʱ�Զ�����
       B[4]		0 - MCLK��ΪPLL����
       B[2:0]	1 - ��ʱ��Ϊ��256 x Fs 
       ����Ĭ������0x41 */
    
    /* BLOCK_POWER_SAI ����Ĭ������0x3F
        B[7]	0 - LR_POL��LRCLK������Ϊ�ȵͺ��
        B[6]	0 - BCLKEDGE��������ʱ���½��ظı�
        B[5]	1 - LDOʹ��
        B[4]	1 - ��׼��Դʹ��
        B[3:0]	F - ʹ������4��ͨ�� */
    
    /* B[7:6]	1 - SDATA_FMT���������������
       B[5:3]	3 - SAI������TDM8��ʽ
       B[2:0]	2 - FS������Ƶ�ʲο�32kHz~48kHz���䣬ʵ��Ϊ51.2kHz */
    r = write_reg(hi2c, id, SAI_CTRL0_REG, 0x5A);
    
    /*	B[7]	0 - SDATA_SEL��SDATAOUT1�������
        B[6:5]	1 - SLOT_WIDTH��ÿ��ʱ϶24��BCLK
        B[4]	0 - DATA_WIDTH, �������λ��Ϊ24bit
        B[3]	1 - LR_MODE������LRCLKģʽΪ����ģʽ��Ϊ��BCLK��������
        B[2]	0 - SAI_MSB������������MSB���ȷ�ʽ
        B[1]	0 - BCLKRATE������������ģʽ
        B[0]	0 - SAI_MS�����ù����ڴ�ģʽ�� */
    r = write_reg(hi2c, id, SAI_CTRL1_REG, 0x28);
    
    /* SAI_CMAP12������Ĭ������0x10
       SAI_CMAP34������Ĭ������0x32 */
       
    /* 	B[7:4]	F - SAI_DRV����������ͨ���������������������
        B[3]	1 - DRV_HIZ��δ�õ�SAIͨ������Ϊ����ģʽ��������ADC���ã�
        B[0]	���¹���״̬�����������쳣ʱ��ȡ��λȷ���Ƿ���� */
    r = write_reg(hi2c, id, SAI_OVERTEMP_REG, 0xF8);
    /* MISC_CONTROL������Ĭ������0x02 */
    /* DC_HPF_CAL������Ĭ������0x00 */
    
    return r;
}



int adau197x_init(void)
{
    int r;
    
    io_init();
    hw_reset();
    
    r = power_on(i2c1Handle, ADAU197X_ID_1, 1);
    if(r==0) {
        r = set_config(i2c1Handle, ADAU197X_ID_1);
    }
    
    
    r = power_on(i2c1Handle, ADAU197X_ID_2, 1);
    if(r==0) {
        r = set_config(i2c1Handle, ADAU197X_ID_2);
    }
    
    return 0;
}


int adau197x_config(void)
{
    set_config(i2c1Handle, ADAU197X_ID_1);
    set_config(i2c1Handle, ADAU197X_ID_2);
    
    return 0;
}



int adau1978_get_status(U8 *status)
{
    read_reg(i2c1Handle, ADAU197X_ID_1, SAI_OVERTEMP_REG, status);
    read_reg(i2c1Handle, ADAU197X_ID_2, SAI_OVERTEMP_REG, status);
    
    return 0;
}


