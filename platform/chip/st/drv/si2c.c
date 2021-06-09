#include "drv/clk.h"
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

typedef struct {
    U32         delay;
    U32         count;
    si2c_cfg_t  cfg;
}si2c_handle_t;


#define SCL_OUT(pin)          gpio_set_dir(&pin->scl, MODE_OUTPUT)
#define SCL_IN(pin)           gpio_set_dir(&pin->scl, MODE_INPUT)
#define SCL_HIGH(pin)         gpio_set_hl(&pin->scl, 1)
#define SCL_LOW(pin)          gpio_set_hl(&pin->scl, 0)
#define SCL_READ(pin)         gpio_get_hl(&pin->scl)

#define SDA_OUT(pin)          gpio_set_dir(&pin->sda, MODE_OUTPUT)
#define SDA_IN(pin)           gpio_set_dir(&pin->sda, MODE_INPUT)
#define SDA_HIGH(pin)         gpio_set_hl(&pin->sda, 1)
#define SDA_LOW(pin)          gpio_set_hl(&pin->sda, 0)
#define SDA_READ(pin)         gpio_get_hl(&pin->sda)

#define SYS_FREQ()            clk2_get_freq()

//��ʱn��1/4 i2c clock
static void delay_qclk(si2c_handle_t *h, U8 n)
{
    delay_us(h->delay*n);
}

static void set_scl(si2c_handle_t *h, U8 hl)
{
    SCL_OUT(&h->cfg.pin);
    if(hl>0) SCL_HIGH(h);
    else     SCL_LOW(h);
}
static void set_sda(si2c_handle_t *h, U8 hl)
{
    SDA_OUT(h);
    if(hl>0) SDA_HIGH(h);
    else     SDA_LOW(h);
}

static int get_scl(si2c_handle_t *h)
{
    SCL_IN(h);
    return SCL_READ(h);
}
static int get_sda(si2c_handle_t *h)
{
    SDA_IN(h);
    return  SDA_READ(h);
}

static void i2c_reset(si2c_handle_t *h)
{
    set_scl(h, 1);
    set_sda(h, 1);
    for(U8 i=0;i<9;i++) {//ǿ�Ƹ�λI2C����
        set_scl(h, 0);
        delay_qclk(h, 2);
        set_scl(h, 1);
        delay_qclk(h, 2);
    }
}


//SCLΪ��ʱ��SDA�Ӹߵ�������
static void i2c_start(si2c_handle_t *h)
{
	set_scl(h, 1);
 	set_sda(h, 1);
    
    delay_qclk(h, 1);
	set_sda(h, 0);
}	  


//SCLΪ��ʱ��SDA�ӵ͵�������
static void i2c_stop(si2c_handle_t *h)
{
	set_scl(h, 1);
 	set_sda(h, 0);
    
    delay_qclk(h, 1);
	set_sda(h, 1);
}

/*
    �ȴ�Ӧ���źŵ���
    0: ����Ӧ��ʧ��
    1: ����Ӧ��ɹ�
*/
static U8 get_ack(si2c_handle_t *h)
{
    U8 r=0;
    U32 t=0;
    
	set_scl(h, 0);
    delay_qclk(h, 2);
    
	set_scl(h, 1);
    while(t++<h->count) {
        if(get_sda(h)==0) {
            r = 1;
            break;
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
static void i2c_ack(si2c_handle_t *h, U8 x)
{
	set_scl(h, 0);
    delay_qclk(h, 1);
    
	set_sda(h, (x>0)?1:0);
    delay_qclk(h, 1);
    
	set_scl(h, 1);
    delay_qclk(h, 2);
}

				 				     
/*
    1: ��Ӧ��
    0: ��Ӧ��	
*/
static U8 write_byte(si2c_handle_t *h, U8 data, U8 stretch)
{                        
    S8 i;
    
    for(i=7; i>=0; i--) {
        set_scl(h, 0);
        delay_qclk(h, 1);
        
		if (data&(1<<i)) set_sda(h, 1);
		else             set_sda(h, 0);
        delay_qclk(h, 1);
		
		set_scl(h, 1);
        delay_qclk(h, 2);
    }
    
    if(stretch) {
        while(get_scl(h)==0);
    }
    
    return get_ack(h);
} 	    
//��1���ֽڣ�ack=1ʱ������ACK��ack=0������nACK   
static U8 read_byte(si2c_handle_t *h, U8 ack, U8 stretch)
{
	S8 i; U8 data=0;
    
    for(i=7; i>=0; i--) {
        set_scl(h, 0);
        delay_qclk(h, 1);
		
        if(get_sda(h)) data |= 1<<i; 
		delay_qclk(h, 1);
        
        set_scl(h, 1);
        delay_qclk(h, 2);
    }
    
    if(stretch) {
        while(get_scl(h)==0);
    }
    i2c_ack(h, ack);
    
    return data;
}


handle_t si2c_init(si2c_cfg_t *sc)
{
    U32 freq=SYS_FREQ();
    si2c_handle_t *h;
    
    if(!sc) {
        return NULL;
    }
    
    h = calloc(1, sizeof(si2c_handle_t));
    if(!h) {
        return NULL;
    }
    
    h->cfg = *sc;
    h->delay = 1000000/(h->cfg.freq*4);
    h->count = freq/(h->cfg.freq*2)+1;
    gpio_init(&h->cfg.scl, MODE_OUTPUT);
    gpio_init(&h->cfg.sda, MODE_OUTPUT);
    i2c_reset(h);
    
    return h;
}


int si2c_deinit(handle_t *h)
{
    si2c_handle_t **sh=(si2c_handle_t**)h;
    
    if(!sh || !(*sh)) {
        return -1;
    }
    
    free(sh);
    
    return 0;
}



int si2c_read(handle_t h, U8 addr, U8 *data, U32 len)
{
    int r=0;
    U32 i;
    U8  ack;
    si2c_handle_t *sh=(si2c_handle_t*)h;
    
    i2c_start(sh);
    ack = write_byte(sh, (addr<<1)|1, 0);
    if(ack) {
        for(i=0; i<len; i++) {
            data[i] = read_byte(sh, 1, 1);
        }
    }
    else {
        return -1;
    }
    i2c_stop(sh);
    
    return 0;
}


int si2c_write(handle_t h, U8 addr, U8 *data, U32 len)
{
    int r=0;
    U32 i;
    U8  ack;
    si2c_handle_t *sh=(si2c_handle_t*)h;
    
    i2c_start(sh);
    ack = write_byte(sh, addr<<1, 0);
    if(ack) {
        for(i=0; i<len; i++) {
            ack = write_byte(sh, data[i], 1);
            if(!ack) {
                r = -1;
            }
        }
    }
    else {
      r = -1;  
    }
    i2c_stop(sh);
    
    return r;
}



