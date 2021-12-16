#include "drv/si2c.h"
#include "drv/delay.h"
#include "drv/gpio.h"

/*
        �豸��ַ	       ��дλ	    ����
        0000 000	       0	        �㲥��ַ
        0000 000	       1	        �����ֽ�
        0000 001	       X	        CBUS ��ַ
        0000 010	       X	        Ԥ������ͬ�����߸�ʽ
        0000 011	       X	        Ԥ��δ����չʹ��
        0000 1XX	       1	        Hs-mode ������
        1111 1XX	       1	        �豸 ID
        1111 0XX	       X	        10 λ�ӵ�ַ
        
        
        ��SCL�ߵ�ƽ�ڼ�, SDA�뱣���ȶ�
        ��SCL�͵�ƽ�ڼ�, SDA��������
        I2C�涨��SCL��Ϊ�ߵ�ƽ�ڼ�, SDA���ɸߵ�ƽ��͵�ƽ�ı仯��ʾ��ʼ�ź�; 
                 SCL��Ϊ�ߵ�ƽ�ڼ�, SDA���ɵ͵�ƽ��ߵ�ƽ�ı仯��ʾ��ֹ�ź�  
                 ��ˣ����뱣֤��ÿ��ʱ�������ڶ�������SDA��������, �����������½��ض���һ��
                 Ŀ���Ǽ����ʼ����ֹ�ź�
        
        
        ��λ ����ͨ�ù㲥��ַ����� 06h �ֽ�, �Ϳ���ʹ���豸��λ, ��������ܲ�������оƬ��֧��
*/


static U32 i2c_us=0;
static U32 i2c_cnt=0;
static si2c_cfg_t s_si2c;

#define SCL_OUT()          gpio_set_dir(&s_si2c.scl, MODE_OUTPUT)
#define SCL_IN()           gpio_set_dir(&s_si2c.scl, MODE_INPUT)
#define SCL_HIGH()         gpio_set_hl(&s_si2c.scl, 1)
#define SCL_LOW()          gpio_set_hl(&s_si2c.scl, 0)
#define SCL_READ()         gpio_get_hl(&s_si2c.scl)

#define SDA_OUT()          gpio_set_dir(&s_si2c.sda, MODE_OUTPUT)
#define SDA_IN()           gpio_set_dir(&s_si2c.sda, MODE_INPUT)
#define SDA_HIGH()         gpio_set_hl(&s_si2c.sda, 1)
#define SDA_LOW()          gpio_set_hl(&s_si2c.sda, 0)
#define SDA_READ()         gpio_get_hl(&s_si2c.sda)

#define SYS_FREQ()         hal_clock_get_mcu_clock_frequency()

//��ʱn��1/4 i2c clock
static void delay_qclk(U8 n)
{
    delay_us(i2c_us*n);
}

static void set_scl(U8 hl)
{
    SCL_OUT();
    if(hl>0) SCL_HIGH();
    else     SCL_LOW();
}
static void set_sda(U8 hl)
{
    SDA_OUT();
    if(hl>0) SDA_HIGH();
    else     SDA_LOW();
}

static int get_scl(void)
{
    SCL_IN();
    return SCL_READ();
}
static int get_sda(void)
{
    SDA_IN();
    return  SDA_READ();
}

static void i2c_reset(void)
{
    set_scl(1);
    set_sda(1);
    for(U8 i=0;i<9;i++) {//ǿ�Ƹ�λI2C����
        set_scl(0);
        delay_qclk(2);
        set_scl(1);
        delay_qclk(2);
    }
}


//SCLΪ��ʱ��SDA�Ӹߵ�������
static void i2c_start(void)
{
	set_scl(1);
 	set_sda(1);
    
    delay_qclk(1);
	set_sda(0);
}	  


//SCLΪ��ʱ��SDA�ӵ͵�������
static void i2c_stop(void)
{
	set_scl(1);
 	set_sda(0);
    
    delay_qclk(1);
	set_sda(1);
}

/*
    �ȴ�Ӧ���źŵ���
    0: ����Ӧ��ʧ��
    1: ����Ӧ��ɹ�
*/
static U8 get_ack(void)
{
    U8 r=0;
    U32 t=0;
    
	set_scl(0);
    delay_qclk(2);
    
	set_scl(1);
    while(t++<i2c_cnt) {
        if(r==0) {
            r = (get_sda()==0);
        }
	}
    
	return r;  
} 

/*
ACK:  �ھŸ�ʱ�������ڼ�SDA���ͣ����ڴ�ʱ������ĸߵ�ƽ�ڼ䱣���ȶ��ĵ͵�ƽ
NACK: �ھŸ�ʱ�������ڼ�SDA����, ���� NACK �����������������
    1. ������û�б�������������ַ�Ľ�����,���û���豸��ӦӦ��
    2. �������޷�ִ�н��ջ��Ͳ���
    3. �ڴ�������У��������յ�Ӧ��Э�鲻�������ݻ�����
    4. �ڴ����ڼ�,�������޷��ٽ��ո�����Ч�����ֽ�
    5. ����������NACK֪ͨ�ӷ�������������
x: 0: no ack    1: ack
*/
static void i2c_ack(U8 x)
{
	set_scl(0);
    delay_qclk(1);
    
	set_sda((x>0)?1:0);
    delay_qclk(1);
    
	set_scl(1);
    delay_qclk(2);
}

				 				     
/*
    1: ��Ӧ��
    0: ��Ӧ��	
*/
static U8 write_byte(U8 data, U8 stretch)
{                        
    S8 i;
    
    for(i=7; i>=0; i--) {
        set_scl(0);
        delay_qclk(1);
        
		if (data&(1<<i)) set_sda(1);
		else             set_sda(0);
        delay_qclk(1);
		
		set_scl(1);
        delay_qclk(2);
    }
    
    if(stretch) {
        while(get_scl()==0);
    }
    
    return get_ack();
} 	    
//��1���ֽڣ�ack=1ʱ������ACK��ack=0������nACK   
static U8 read_byte(U8 ack, U8 stretch)
{
	S8 i; U8 data=0;
    
    for(i=7; i>=0; i--) {
        set_scl(0);
        delay_qclk(1);
		
        if(get_sda()) data |= 1<<i; 
		delay_qclk(1);
        
        set_scl(1);
        delay_qclk(2);
    }
    
    if(stretch) {
        while(get_scl()==0);
    }
    i2c_ack(ack);
    
    return data;
}


int si2c_init(si2c_cfg_t *si)
{
    U32 sys_freq=SYS_FREQ();
    if(!si) {
        return -1;
    }
    
    s_si2c = *si;
    i2c_us = 1000000/(s_si2c.freq*4);
    i2c_cnt = sys_freq/(s_si2c.freq*2)+1;
    gpio_init(&s_si2c.scl, MODE_OUTPUT);
    gpio_set_hl(&s_si2c.scl, 1);
    
    gpio_init(&s_si2c.sda, MODE_OUTPUT);
    gpio_set_hl(&s_si2c.sda, 1);
    
    i2c_reset();
    
    return 0;
}


int si2c_read(U8 addr, U8 *data, U32 len, U8 stop)
{
    int r=0;
    U32 i;
    U8  ack;
    
    i2c_start();
    ack = write_byte((addr<<1)|1, 0);
    if(ack) {
        for(i=0; i<len; i++) {
            data[i] = read_byte(1, 1);
        }
    }
    else {
        return -1;
    }
    
    if(stop) {
        i2c_stop();
    }
    
    return 0;
}


int si2c_write(U8 addr, U8 *data, U32 len, U8 stop)
{
    int r=0;
    U32 i;
    U8  ack;
    
    i2c_start();
    ack = write_byte(addr<<1, 0);
    if(ack) {
        for(i=0; i<len; i++) {
            ack = write_byte(data[i], 1);
            if(!ack) {
                r = -1;
            }
        }
    }
    else {
      r = -1;  
    }
    
    if(stop) {
        i2c_stop();
    }
    
    return r;
}



