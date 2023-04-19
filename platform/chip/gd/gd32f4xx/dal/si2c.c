#include "dal/dal.h"
#include "dal/si2c.h"
#include "dal/delay.h"
#include "dal/gpio.h"

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
typedef enum {
    I2C_1x=0,          //SCL:PH4   SDA:PH5
    I2C_2x,            //SCL:PB6   SDA:PB7
    
}I2Cx;


typedef struct {
    U32         delay;
    U32         count;
    si2c_cfg_t  cfg;
    
    I2Cx        i2cx;
}si2c_handle_t;


#if 0
#define SCL_OUT(pin)          gpio_set_dir(&(pin)->scl, MODE_OUTPUT)
#define SCL_IN(pin)           gpio_set_dir(&(pin)->scl, MODE_INPUT)
#define SCL_HIGH(pin)         gpio_set_hl(&(pin)->scl, 1)
#define SCL_LOW(pin)          gpio_set_hl(&(pin)->scl, 0)
#define SCL_READ(pin)         gpio_get_hl(&(pin)->scl)

#define SDA_OUT(pin)          gpio_set_dir(&(pin)->sda, MODE_OUTPUT)
#define SDA_IN(pin)           gpio_set_dir(&(pin)->sda, MODE_INPUT)
#define SDA_HIGH(pin)         gpio_set_hl(&(pin)->sda, 1)
#define SDA_LOW(pin)          gpio_set_hl(&(pin)->sda, 0)
#define SDA_READ(pin)         gpio_get_hl(&(pin)->sda)

#define GET_FREQ()            dal_get_freq()

//延时n个1/4 i2c clock
static void delay_qclk(si2c_handle_t *h, U8 n)
{
    delay_us(h->delay*n);
}

static void set_scl(si2c_handle_t *h, U8 hl)
{
    SCL_OUT(&(h->cfg.pin));
    if(hl>0) SCL_HIGH(&h->cfg.pin);
    else     SCL_LOW(&h->cfg.pin);
}
static void set_sda(si2c_handle_t *h, U8 hl)
{
    SDA_OUT(&h->cfg.pin);
    if(hl>0) SDA_HIGH(&h->cfg.pin);
    else     SDA_LOW(&h->cfg.pin);
}

static int get_scl(si2c_handle_t *h)
{
    SCL_IN(&h->cfg.pin);
    return SCL_READ(&h->cfg.pin);
}
static int get_sda(si2c_handle_t *h)
{
    SDA_IN(&h->cfg.pin);
    return  SDA_READ(&h->cfg.pin);
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
    U32 freq=GET_FREQ();
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
    gpio_init(&h->cfg.pin.scl, MODE_OUTPUT);
    gpio_init(&h->cfg.pin.sda, MODE_OUTPUT);
    i2c_reset(h);
    
    return h;
}


int si2c_deinit(handle_t h)
{
    si2c_handle_t *sh=(si2c_handle_t*)h;
    
    if(!sh) {
        return -1;
    }
    
    free(sh);
    
    return 0;
}



int si2c_read(handle_t h, U8 addr, U8 *data, U32 len, U8 bstop)
{
    int r=0;
    U32 i;
    U8  ack;
    si2c_handle_t *sh=(si2c_handle_t*)h;
    
    if(!h) {
        return -1;
    }
    
    i2c_start(sh);
    ack = write_byte(sh, (addr<<1)|1, 0);
    if(ack) {
        for(i=0; i<len; i++) {
            data[i] = read_byte(sh, 1, 0);
        }
    }
    else {
        return -1;
    }
    
    if(bstop) {
        i2c_stop(sh);
    }
    
    return 0;
}


int si2c_write(handle_t h, U8 addr, U8 *data, U32 len, U8 bstop)
{
    int r=0;
    U32 i;
    U8  ack;
    si2c_handle_t *sh=(si2c_handle_t*)h;
    
    if(!h) {
        return -1;
    }
    
    i2c_start(sh);
    ack = write_byte(sh, addr<<1, 0);
    if(ack) {
        for(i=0; i<len; i++) {
            ack = write_byte(sh, data[i], 0);
            if(!ack) {
                r = -1;
            }
        }
    }
    else {
      r = -1;  
    }
    
    if(bstop) {
        i2c_stop(sh);
    }
    
    return r;
}
#else

static inline void IIC1_SDA_IN()       {GPIOH->MODER&=~(3<<(5*2));GPIOH->MODER|=0<<5*2;}	// PH5输入模式
static inline void IIC1_SDA_OUT()      {GPIOH->MODER&=~(3<<(5*2));GPIOH->MODER|=1<<5*2;}    // PH5输出模式

static inline void IIC2_SDA_IN()       {GPIOB->MODER&=~(3<<(7*2));GPIOB->MODER|=0<<7*2;}	// PB7输入模式
static inline void IIC2_SDA_OUT()      {GPIOB->MODER&=~(3<<(7*2));GPIOB->MODER|=1<<7*2;}    // PB7输出模式

#define IIC1_SCL(n)         (n?HAL_GPIO_WritePin(GPIOH,GPIO_PIN_4,GPIO_PIN_SET):HAL_GPIO_WritePin(GPIOH,GPIO_PIN_4,GPIO_PIN_RESET)) // I2C_1_SCL
#define IIC1_SDA(n)         (n?HAL_GPIO_WritePin(GPIOH,GPIO_PIN_5,GPIO_PIN_SET):HAL_GPIO_WritePin(GPIOH,GPIO_PIN_5,GPIO_PIN_RESET)) // I2C_1_SDA

#define IIC2_SCL(n)         (n?HAL_GPIO_WritePin(GPIOB,GPIO_PIN_6,GPIO_PIN_SET):HAL_GPIO_WritePin(GPIOB,GPIO_PIN_6,GPIO_PIN_RESET)) // I2C_2_SCL
#define IIC2_SDA(n)         (n?HAL_GPIO_WritePin(GPIOB,GPIO_PIN_7,GPIO_PIN_SET):HAL_GPIO_WritePin(GPIOB,GPIO_PIN_7,GPIO_PIN_RESET)) // I2C_2_SDA

#define IIC1_READ_SDA()     HAL_GPIO_ReadPin(GPIOH,GPIO_PIN_5)  // 读I2C_1_SDA
#define IIC2_READ_SDA()     HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_7)  // 读I2C_2_SDA


///////////////////////////////////////////////////////////////////////////////

#define IIC_SCL(i2c,n)      ((i2c==I2C_1x)?IIC1_SCL(n):IIC2_SCL(n))
#define IIC_SDA(i2c,n)      ((i2c==I2C_1x)?IIC1_SDA(n):IIC2_SDA(n))

#define IIC_SCL_IN(i2c)     ((i2c==I2C_1x)?IIC1_SCL_IN():IIC2_SCL_IN())
#define IIC_SCL_OUT(i2c)    ((i2c==I2C_1x)?IIC1_SCL_OUT():IIC2_SCL_OUT())

#define IIC_SDA_IN(i2c)     ((i2c==I2C_1x)?IIC1_SDA_IN():IIC2_SDA_IN())
#define IIC_SDA_OUT(i2c)    ((i2c==I2C_1x)?IIC1_SDA_OUT():IIC2_SDA_OUT())

#define IIC_READ_SDA(i2c)   ((i2c==I2C_1x)?IIC1_READ_SDA():IIC2_READ_SDA())



static void IIC_Start(I2Cx i2c);				    // 发送I2C开始信号
static void IIC_Stop(I2Cx i2c);				        // 发送I2C停止信号
static BOOL IIC_Wait_Ack(I2Cx i2c);			        // I2C等待ACK信号
static void IIC_Ack(I2Cx i2c);					    // I2C发送ACK信号
static void IIC_NAck(I2Cx i2c);				        // I2C不发送ACK信号



/*
IIC_Start
------------------
描述：产生I2C起始信号
*/
static void IIC_Start(I2Cx i2c)
{
	IIC_SDA_OUT(i2c); // SDA线输出
	IIC_SDA(i2c,1);	  	  
	IIC_SCL(i2c,1);
	delay_us(4);
 	IIC_SDA(i2c,0); // START:when CLK is high,DATA change form high to low 
	delay_us(4);
	IIC_SCL(i2c,0); // 钳住I2C总线，准备发送或接收数据 
}


/*
IIC_Stop
------------------
描述：产生I2C停止信号
*/
static void IIC_Stop(I2Cx i2c)
{
	IIC_SDA_OUT(i2c); // SDA线输出
	IIC_SCL(i2c,0);
	IIC_SDA(i2c,0); // STOP: when CLK is high DATA change form low to high
 	delay_us(4);
	IIC_SCL(i2c,1); 
	IIC_SDA(i2c,1); // 发送I2C总线结束信号
	delay_us(4);							   	
}


/*
IIC_Wait_Ack
------------------
描述：等待应答信号到来
返回值：TRUE  接收应答成功
		FALSE 接收应答失败   
*/
static BOOL IIC_Wait_Ack(I2Cx i2c)
{
	INT8U ucErrTime = 0;
    
	IIC_SDA_IN(i2c); // SDA设置为输入
	IIC_SDA(i2c,1);
	delay_us(1);
	IIC_SCL(i2c,1);
	delay_us(1);
	while (IIC_READ_SDA(i2c)) {
		ucErrTime++;
		if (ucErrTime > 250) {
			IIC_Stop(i2c);
			return FALSE;
		}
	}
	IIC_SCL(i2c,0); // 时钟输出0
	return TRUE;  
}


/*
IIC_Ack
------------------
描述：I2C产生ACK应答
*/
static void IIC_Ack(I2Cx i2c)
{
	IIC_SCL(i2c,0);
	IIC_SDA_OUT(i2c);
	IIC_SDA(i2c,0);
	delay_us(2);
	IIC_SCL(i2c,1);
	delay_us(2);
	IIC_SCL(i2c,0);
}


/*
IIC_NAck
------------------
描述：I2C不产生ACK应答
*/
static void IIC_NAck(I2Cx i2c)
{
	IIC_SCL(i2c,0);
	IIC_SDA_OUT(i2c);
	IIC_SDA(i2c,1);
	delay_us(2);
	IIC_SCL(i2c,1);
	delay_us(2);
	IIC_SCL(i2c,0);
}


/*
IIC_Send_Byte
------------------
描述：I2C发送一个字节
*/
static void IIC_Send_Byte(I2Cx i2c, INT8U txd)
{                        
    INT8U t;
    
	IIC_SDA_OUT(i2c); 	    
    IIC_SCL(i2c,0);
    for (t=0; t<8; t++) {              
        IIC_SDA(i2c, (txd & 0x80)>>7);
        txd<<=1; 	  
		delay_us(2);
		IIC_SCL(i2c,1);
		delay_us(2); 
		IIC_SCL(i2c,0);	
		delay_us(2);
    }	 
}



/*
IIC_Read_Byte
------------------
描述：I2C读取1个字节
Note：ack=1时，发送ACK
      ack=0时，发送nACK
*/
static INT8U IIC_Read_Byte(I2Cx i2c, INT8U ack)
{
	INT8U i, receive = 0;
    
	IIC_SDA_IN(i2c); // SDA设置为输入
    for (i=0; i<8; i++) {
        IIC_SCL(i2c,0); 
        delay_us(2);
		IIC_SCL(i2c,1);
        receive <<= 1;
        if (IIC_READ_SDA(i2c))
			receive++;   
		delay_us(1); 
    }					 
    if (!ack)
        IIC_NAck(i2c); // 发送nACK
    else
        IIC_Ack(i2c); // 发送ACK
	
    return receive;
}





handle_t si2c_init(si2c_cfg_t *cfg)
{
    GPIO_InitTypeDef init = {0};
    si2c_handle_t *h=calloc(1, sizeof(si2c_handle_t));
    
    if(!h) {
        return NULL;
    }
    h->cfg = *cfg;
    
    init.Mode = GPIO_MODE_OUTPUT_PP;		    // 推挽输出
    init.Pull = GPIO_PULLUP;				    // 上拉
    init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;	    // 快速    
    
    if(h->cfg.pin.scl.grp==GPIOH && h->cfg.pin.scl.pin==GPIO_PIN_4) {
        init.Pin = GPIO_PIN_4 | GPIO_PIN_5;		// PH4 PH5
        
        __HAL_RCC_GPIOH_CLK_ENABLE();
        HAL_GPIO_Init(GPIOH, &init);
        IIC2_SDA(1); IIC2_SCL(1);
        
        h->i2cx = I2C_1x;
    }
    else {
        
        init.Pin = GPIO_PIN_6 | GPIO_PIN_7;		// PB6 PB7
        
        __HAL_RCC_GPIOB_CLK_ENABLE();
        HAL_GPIO_Init(GPIOB, &init);
        IIC1_SDA(1); IIC1_SCL(1); 
        
        h->i2cx = I2C_2x;
    }
    
    return h;
}



static int read_byte(I2Cx i2c, U8 addr, U8 reg, U8* val)
{
    IIC_Start(i2c);
	IIC_Send_Byte(i2c,(addr<<1)|0); // 发送I2C器件地址+写命令
	if (!IIC_Wait_Ack(i2c))
		return -1;
    IIC_Send_Byte(i2c,addr); // 发送寄存器地址
	if (!IIC_Wait_Ack(i2c))
		return -1;
	IIC_Start(i2c); 	   
	IIC_Send_Byte(i2c,(addr<<1)|1); // 发送I2C器件地址+读命令			   
	if (!IIC_Wait_Ack(i2c))
		return -1;
    *val = IIC_Read_Byte(i2c,0); // 读取寄存器数据
    IIC_Stop(i2c);
    
	return 0;
}

static int write_byte(I2Cx i2c, U8 addr, U8 reg, U8 val)
{
    IIC_Start(i2c);
	IIC_Send_Byte(i2c,(addr<<1)|0); // ??I2C????+???
	if (!IIC_Wait_Ack(i2c))
		return -1;
    IIC_Send_Byte(i2c,reg); // ???????
	if (!IIC_Wait_Ack(i2c))
		return -1;										  		   
	IIC_Send_Byte(i2c,val); // ???????							   
	if (!IIC_Wait_Ack(i2c)) // ! ADAU1978???? ?????????ACK
		return -1;
    IIC_Stop(i2c);
	
	return 0;
}

int si2c_deinit(handle_t h)
{
    si2c_handle_t *sh=(si2c_handle_t*)h;
    
    if(!sh) {
        return -1;
    }
    
    free(sh);
    
    return 0;
}



int si2c_read(handle_t h, U8 addr, U8 *data, U32 len, U8 bstop)
{
    U32 i;
    si2c_handle_t *sh=(si2c_handle_t*)h;
    
	IIC_Start(sh->i2cx); 	   
	IIC_Send_Byte(sh->i2cx,(addr<<1)|1); // 发送I2C器件地址+读命令			   
	if (!IIC_Wait_Ack(sh->i2cx))
		return -1;
    
    for(i=0; i<len; i++) {
        data[i] = IIC_Read_Byte(sh->i2cx,0);
    }
    
    if(bstop) {
        IIC_Stop(sh->i2cx);
    }
    
    return 0;
}


int si2c_write(handle_t h, U8 addr, U8 *data, U32 len, U8 bstop)
{
    U32 i;
    si2c_handle_t *sh=(si2c_handle_t*)h;
    
	IIC_Start(sh->i2cx); 	   
	IIC_Send_Byte(sh->i2cx,(addr<<1)|0); // 发送I2C器件地址+读命令			   
	if (!IIC_Wait_Ack(sh->i2cx))
		return -1;
    
    for(i=0; i<len; i++) {
        IIC_Send_Byte(sh->i2cx,data[i]);
        if (!IIC_Wait_Ack(sh->i2cx))
            return -1;
    }
    
    if(bstop) {
        IIC_Stop(sh->i2cx);
    }
    
    return 0;
}


#endif




