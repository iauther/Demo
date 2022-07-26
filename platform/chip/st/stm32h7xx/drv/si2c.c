#include "drv/clk.h"
#include "drv/si2c.h"
#include "drv/delay.h"
#include "drv/gpio.h"

/*
        设备地址	       读写位	    描述
        0000 000	       0	        广播地址
        0000 000	       1	        启动字节
        0000 001	       X	        CBUS 地址
        0000 010	       X	        预留给不同的总线格式
        0000 011	       X	        预留未来扩展使用
        0000 1XX	       1	        Hs-mode 主代码
        1111 1XX	       1	        设备 ID
        1111 0XX	       X	        10 位从地址
        
        
        在SCL高电平期间, SDA须保持稳定
        在SCL低电平期间, SDA可以跳变
        I2C规定：SCL线为高电平期间, SDA线由高电平向低电平的变化表示起始信号; 
                 SCL线为高电平期间, SDA线由低电平向高电平的变化表示终止信号  
                 因此，必须保证在每个时钟周期内对数据线SDA采样两次, 即在上升和下降沿都采一次
                 目的是检测起始和终止信号
        
        
        复位 ：当通用广播地址后面跟 06h 字节, 就可以使从设备软复位, 但这个功能并非所有芯片都支持
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

//延时n个1/4 i2c clock
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
    for(U8 i=0;i<9;i++) {//强制复位I2C总线
        set_scl(h, 0);
        delay_qclk(h, 2);
        set_scl(h, 1);
        delay_qclk(h, 2);
    }
}


//SCL为高时，SDA从高到低跳变
static void i2c_start(si2c_handle_t *h)
{
	set_scl(h, 1);
 	set_sda(h, 1);
    
    delay_qclk(h, 1);
	set_sda(h, 0);
}	  


//SCL为高时，SDA从低到高跳变
static void i2c_stop(si2c_handle_t *h)
{
	set_scl(h, 1);
 	set_sda(h, 0);
    
    delay_qclk(h, 1);
	set_sda(h, 1);
}

/*
    等待应答信号到来
    0: 接收应答失败
    1: 接收应答成功
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
ACK:  第九个时钟脉冲期间SDA拉低，并在此时钟脉冲的高电平期间保持稳定的低电平
NACK: 第九个时钟脉冲期间SDA拉高, 导致 NACK 产生的条件有五个：
    1. 总线上没有报文中所包含地址的接收器,因此没有设备响应应答。
    2. 接收器无法执行接收或发送操作
    3. 在传输过程中，接收器收到应用协议不理解的数据或命令
    4. 在传输期间,接收器无法再接收更多有效数据字节
    5. 主接收器用NACK通知从发送器结束传输
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
    1: 有应答
    0: 无应答	
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
//读1个字节，ack=1时，发送ACK，ack=0，发送nACK   
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



