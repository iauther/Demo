#include "sd30xx.h"
#include "gd32f4xx_i2c.h"
#include "gd32f4xx_gpio.h"
#include "cfg.h"
#include "log.h"



#define DELAYP_CNT              (200)
#define RTC_Address             0x64
#define GD_Address              0x28

#define ID_Address              0x72        //ID号起始地址

/********************************************************/
#define H                       1
#define L                       0
#define Chg_enable              0x82
#define Chg_disable             0

/**************** Alarm register ********************/
#define Alarm_SC                0x07
#define Alarm_MN                0x08
#define Alarm_HR                0x09
#define Alarm_WK                0x0A
#define Alarm_DY                0x0B
#define Alarm_MO                0x0C
#define Alarm_YR                0x0D
#define Alarm_EN                0x0E

/**************** Control Register *******************/
#define CTR1                    0x0F
#define CTR2                    0x10
#define CTR3                    0x11

/************* Timer Counter Register ****************/
#define Timer_Counter1          0x13
#define Timer_Counter2          0x14
#define Timer_Counter3          0x15

/**************** Battery Register ********************/
#define Chg_MG                  0x18        //充电管理寄存器地址
#define Bat_H8                  0x1A        //电量最高位寄存器地址
#define Bat_L8                  0x1B        //电量低八位寄存器地址


/****************** 报警中断宏定义 *********************/
#define sec_ALM                 0x01
#define min_ALM                 0x02
#define hor_ALM                 0x04
#define wek_ALM                 0x08
#define day_ALM                 0x10
#define mon_ALM                 0x20
#define yar_ALM                 0x40

/****************** 中断使能宏定义 **********************/
#define INTDE                   0x04        //倒计时中断
#define INTAE                   0x02        //报警中断
#define INTFE                   0x01        //频率中断

//////////////////////////////////////////////////////////////

static handle_t sdHandle=NULL;


#ifdef I2C_HARD_PIN
    #define SCL_PIN    GPIO_PIN_10
    #define SCL_PORT   GPIOB
    #define RCU_SCL    RCU_GPIOB
    
    #define SDA_PIN    GPIO_PIN_11
    #define SDA_PORT   GPIOB
    #define RCU_SDA    RCU_GPIOB
#else
    #define SCL_PIN    GPIO_PIN_2
    #define SCL_PORT   GPIOB
    #define RCU_SCL    RCU_GPIOB
    
    #define SDA_PIN    GPIO_PIN_1
    #define SDA_PORT   GPIOB 
    #define RCU_SDA    RCU_GPIOB
#endif


//PB2 -- SCL
//PB1 -- SDA
#define SCL_H         gpio_bit_write(SCL_PORT, SCL_PIN, SET)
#define SCL_L         gpio_bit_write(SCL_PORT, SCL_PIN, RESET)
#define SDA_H         gpio_bit_write(SDA_PORT, SDA_PIN, SET)
#define SDA_L         gpio_bit_write(SDA_PORT, SDA_PIN, RESET)
#define SDA_READ      gpio_input_bit_get(SDA_PORT, SDA_PIN)

static u8 I2CReadSerial(u8 addr, u8 reg, u8 *ps, u8 length);


#ifdef I2C_SOFT
static void I2Cdelay(void)
{
   U32 cnt=DELAYP_CNT;      //这里可以优化速度，通常延时3~10us，可以用示波器看波形来调试
   while(cnt--);
}
static inline u8 I2CStart(void)
{
    SDA_H;
    I2Cdelay();
    SCL_H;
    I2Cdelay();
    if(!SDA_READ) {
        LOGE("___ i2c busying\n");
        return FALSE; //SDA线为低电平则总线忙,退出
    }
    SDA_L;
    I2Cdelay();
    SCL_L;
    I2Cdelay();
    return TRUE;
}
static inline void I2CStop(void)
{
    SCL_L;
    I2Cdelay();
    SDA_L;
    I2Cdelay();
    SCL_H;
    I2Cdelay();
    SDA_H;
    I2Cdelay();
}
static inline void I2CAck(void)
{
    SCL_L;
    I2Cdelay();
    SDA_L;
    I2Cdelay();
    SCL_H;
    I2Cdelay();
    SCL_L;
    I2Cdelay();
}
static inline void I2CNoAck(void)
{
    SCL_L;
    I2Cdelay();
    SDA_H;
    I2Cdelay();
    SCL_H;
    I2Cdelay();
    SCL_L;
    I2Cdelay();
}
static inline u8 I2CWaitAck(void)
{
    u8 r=FALSE;
    u32 cnt=DELAYP_CNT*2;
    
    SCL_L;
    I2Cdelay();
    SDA_H;
    I2Cdelay();
    SCL_H;
#if 0
    while(cnt--) {
        if(!SDA_READ) {
            r = TRUE;
            break;
        }
    }
#else
    I2Cdelay();
    if(!SDA_READ) {
        r = TRUE;
    }
    else {
        LOGE("___ i2c no ack\n");
    }
#endif
    
    SCL_L;
    
    return r;
}
static inline void I2CSendByte(u8 SendByte) //数据从高位到低位
{
    u8 i=8;
    while(i--) {
        SCL_L;
        I2Cdelay();
        if(SendByte&0x80)
            SDA_H;
        else
            SDA_L;
        SendByte<<=1;
        I2Cdelay();
        SCL_H;
        I2Cdelay();
    }
    SCL_L;
}
static inline u8 I2CReceiveByte(void)
{
    u8 i=8;
    u8 ReceiveByte=0;

    SDA_H;
    while(i--) {
      ReceiveByte<<=1;
      SCL_L;
      I2Cdelay();
      SCL_H;
      I2Cdelay();
      if(SDA_READ) {
        ReceiveByte|=0x1;
      }
    }
    SCL_L;
    return ReceiveByte;
}

static u8 Write_Enable(u8 addr)
{
    u8 r=FALSE;
    u8 ctr1,ctr2;
    
    if(!I2CReadSerial(addr, CTR1, &ctr1, 1)) {
        LOGE("___ Write_Enable, read CTR1 failed\n");
        goto quit;
    }
    
    if(!I2CReadSerial(addr, CTR2, &ctr2, 1)) {
        LOGE("___ Write_Enable, read CTR2 failed\n");
        goto quit;
    }
		
    if(!I2CStart()) {
        LOGE("___ Write_Enable failed 111\n");
        goto quit;
    }

    I2CSendByte(addr);
    if(!I2CWaitAck()) {
        LOGE("___ Write_Enable failed 222\n");
        goto quit;
    }

    I2CSendByte(CTR2);
    if(!I2CWaitAck()) {
        LOGE("___ Write_Enable failed 333\n");
        goto quit;
    }
    
    I2CSendByte(ctr2|0x80);     //WRTC1=1     
    if(!I2CWaitAck()) {
        LOGE("___ Write_Enable failed 444\n");
        goto quit;
    }
    I2CStop();

    if(!I2CStart()) {
        LOGE("___ Write_Enable failed 555\n");
        goto quit;
    }
    
    I2CSendByte(addr);
    if(!I2CWaitAck()) {
        LOGE("___ Write_Enable failed 666\n");
        goto quit;
    }
    
    I2CSendByte(CTR1);
    if(!I2CWaitAck()) {
        LOGE("___ Write_Enable failed 777\n");
        goto quit;
    }
    
    I2CSendByte(ctr1|0x84); //置WRTC2,WRTC3=1
    if(!I2CWaitAck()) {
        LOGE("___ Write_Enable failed 888\n");
        goto quit;
    }
    r = TRUE;
    
quit:
    I2CStop();
    return r;
}
static u8 Write_Disable(u8 addr)
{
    u8 r=FALSE;
    u8 ctr1,ctr2;
    
    if(!I2CReadSerial(addr, CTR1, &ctr1, 1)) {
        LOGE("___ Write_Disable, read CTR1 failed\n");
        goto quit;
    }
    
    if(!I2CReadSerial(addr, CTR2, &ctr2, 1)) {
        LOGE("___ Write_Disable, read CTR2 failed\n");
        goto quit;
    }
		
    if(!I2CStart()) {
        LOGE("___ Write_Disable failed 111\n");
        goto quit;
    }

    I2CSendByte(addr);
    if(!I2CWaitAck()) {
        LOGE("___ Write_Disable failed 222\n");
        goto quit;
    }

    I2CSendByte(CTR1);
    if(!I2CWaitAck()) {
        LOGE("___ Write_Disable failed 333\n");
        goto quit;
    }
    
    I2CSendByte(ctr1&0x7b) ;//置WRTC2,WRTC3=0
    if(!I2CWaitAck()) {
        LOGE("___ Write_Disable failed 444\n");
        goto quit;
    }
    
    I2CSendByte(ctr2&0x7f) ;//置WRTC1=0(10H地址)
    if(!I2CWaitAck()) {
        LOGE("___ Write_Disable failed 555\n");
        goto quit;
    }
    r = TRUE;
    
quit:
    I2CStop();
    return r;
}
static u8 I2CWriteBytes(u8 addr, u8 reg, u8 *ps, u8 length)
{
    u8 r=FALSE;
    
    if(!I2CStart()) {
        LOGE("___ I2CWriteBytes failed 111\n");
        goto quit;
    }
    
    I2CSendByte(addr);
    if(!I2CWaitAck()) {
        LOGE("___ I2CWriteBytes failed 222\n");
        goto quit;
    }

    I2CSendByte(reg);
    
    if(!I2CWaitAck()) {
        LOGE("___ I2CWriteBytes failed 333\n");
        goto quit;
    }
    
    for(;(length--)>0;) {
        I2CSendByte(*(ps++));
        I2CAck();
    }
    r = TRUE;
    
quit:
    I2CStop();
    return r;
}
static u8 I2CWriteSerial(u8 addr, u8 reg, u8 *ps, u8 length)
{
    u8 r1,r=FALSE;
    
    if(!Write_Enable(addr)) {
        return FALSE;
    }

    r1 = I2CWriteBytes(addr, reg, ps, length);
    if(r1==FALSE) {
        LOGE("___ I2CWriteSerial failed\n");
    }
    r = Write_Disable(addr);
    
    return (r&&r1)?TRUE:FALSE;
}

static u8 I2CReadSerial(u8 addr, u8 reg, u8 *ps, u8 length)
{
    u8 r=FALSE;
    
    if(!I2CStart()) {
        LOGE("___ I2CReadSerial failed 111\n");
        goto quit;
    }
    
    I2CSendByte(addr);
    if(!I2CWaitAck()) {
        LOGE("___ I2CReadSerial failed 222\n");
        goto quit;
    }

    I2CSendByte(reg);
    
    if(!I2CWaitAck()) {
        LOGE("___ I2CReadSerial failed 333\n");
        goto quit;
    }
    
    I2CStart();
    I2CSendByte(addr+1);
    
    if(!I2CWaitAck()) {
        LOGE("___ I2CReadSerial failed 444\n");
        goto quit;
    }
    
    for(;--length>0;ps++) {
        *ps = I2CReceiveByte();
        I2CAck();
    }
    *ps = I2CReceiveByte();
    
    I2CNoAck();
    r = TRUE;
    
quit:
    I2CStop();
    return r;
}
static void io_init(void)
{
    rcu_periph_clock_enable(RCU_SCL);
    gpio_bit_write(SCL_PORT, SCL_PIN, SET);
    gpio_output_options_set(SCL_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, SCL_PIN); 
	gpio_mode_set(SCL_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, SCL_PIN);
    
    rcu_periph_clock_enable(RCU_SDA);
    gpio_bit_write(SDA_PORT, SDA_PIN, SET);
    gpio_output_options_set(SDA_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_25MHZ, SDA_PIN); 
	gpio_mode_set(SDA_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, SDA_PIN);
    
    //I2CStop();
}
static void io_deinit(void)
{
    
}
#else

#define I2C_PORT   I2C1


static u8 I2CReadBytes(u8 addr, u8 reg, u8 *ps, u8 length)
{
    u8 i;
    
    //wait bus free
    while(i2c_flag_get(I2C_PORT, I2C_FLAG_I2CBSY));
    i2c_ack_config(I2C_PORT, I2C_ACK_ENABLE);

    //send start
    i2c_start_on_bus(I2C_PORT);
    while(!i2c_flag_get(I2C_PORT, I2C_FLAG_SBSEND));

    //send dev address|w
    i2c_master_addressing(I2C_PORT, addr, I2C_TRANSMITTER);
    while(!i2c_flag_get(I2C_PORT, I2C_FLAG_ADDSEND));
    i2c_flag_clear(I2C_PORT, I2C_FLAG_ADDSEND);
    while(!i2c_flag_get(I2C_PORT, I2C_FLAG_TBE));

    //send reg address
    i2c_data_transmit(I2C_PORT, reg);
    while(!i2c_flag_get(I2C_PORT, I2C_FLAG_TBE));

    //generate a start to read data
    i2c_start_on_bus(I2C_PORT);
    while(!i2c_flag_get(I2C_PORT, I2C_FLAG_SBSEND));
    
    //send dev address|r
    i2c_master_addressing(I2C_PORT, addr, I2C_RECEIVER);
    while(!i2c_flag_get(I2C_PORT, I2C_FLAG_ADDSEND));
    i2c_flag_clear(I2C_PORT, I2C_FLAG_ADDSEND);
    
    //recive length data
    for(i=0; i<length; i++) {
        if(i==length-1) {
            //while(!i2c_flag_get(I2C_PORT, I2C_FLAG_BTC));
            i2c_ack_config(I2C_PORT, I2C_ACK_DISABLE);
        }
        
        while(!i2c_flag_get(I2C_PORT, I2C_FLAG_RBNE));
        ps[i] = i2c_data_receive(I2C_PORT);
    }

    //send stop
    i2c_stop_on_bus(I2C_PORT);
    while(I2C_CTL0(I2C_PORT) & I2C_CTL0_STOP);

    return TRUE;
}
static u8 I2CWriteBytes(u8 addr, u8 reg, u8 *ps, u8 length)
{
    u8 i;
    
    //wait bus free
    while(i2c_flag_get(I2C_PORT, I2C_FLAG_I2CBSY));
    i2c_ack_config(I2C_PORT, I2C_ACK_ENABLE);
    
    //send start
    i2c_start_on_bus(I2C_PORT);
    while(!i2c_flag_get(I2C_PORT, I2C_FLAG_SBSEND));
    
    //send dev address|w
    i2c_master_addressing(I2C_PORT, addr, I2C_TRANSMITTER);
    while(!i2c_flag_get(I2C_PORT, I2C_FLAG_ADDSEND));
    i2c_flag_clear(I2C_PORT, I2C_FLAG_ADDSEND);
    while(!i2c_flag_get(I2C_PORT, I2C_FLAG_TBE));

    //send reg address
    i2c_data_transmit(I2C_PORT, reg);
    while(!i2c_flag_get(I2C_PORT, I2C_FLAG_TBE));
    
    //write data
    for(i=0; i<length; i++) {
        i2c_data_transmit(I2C_PORT, ps[i]);
        while(!i2c_flag_get(I2C_PORT, I2C_FLAG_TBE));
    }
    
    //send stop
    i2c_stop_on_bus(I2C_PORT);
    while(I2C_CTL0(I2C_PORT) & I2C_CTL0_STOP);
    
    return TRUE;
}

static u8 Write_Enable(u8 addr)
{
    u8 r=FALSE;
    u8 i,tmp[2],bak[2];
    
    if(!I2CReadBytes(addr, CTR1, tmp, 2)) {
        LOGE("___ Write_Enable failed 111\n");
        goto quit;
    }
    
    tmp[1] |= 0x80; 
    if(!I2CWriteBytes(addr, CTR2, &tmp[1], 1)) {
        LOGE("___ Write_Enable failed 111\n");
        goto quit;
    }
    
    tmp[0] |= 0x84;
    if(!I2CWriteBytes(addr, CTR1, &tmp[0], 1)) {
        LOGE("___ Write_Enable failed 111\n");
        goto quit;
    }
    
    r = TRUE;
    
quit:
    return r;
}
static u8 Write_Disable(u8 addr)
{
    u8 r=FALSE;
    u8 i,tmp[2],bak[2];
    
    if(!I2CReadBytes(addr, CTR1, tmp, 2)) {
        LOGE("___ Write_Disable failed 111\n");
        goto quit;
    }
    
    tmp[0] &= 0x7b;  tmp[1] &= 0x7f;
    if(!I2CWriteBytes(addr, CTR1, tmp, 2)) {
        LOGE("___ Write_Disable failed 222\n");
        goto quit;
    }
    
    if(!I2CReadBytes(addr, CTR1, bak, 2)) {
        LOGE("___ Write_Disable failed 111\n");
        goto quit;
    }
    
    if(memcmp(tmp, bak, 2)) {
        LOGE("___ Write_Disable memcmp failed\n");
        goto quit;
    }
    r = TRUE;
    
quit:
    return r;
}
static u8 I2CWriteSerial(u8 addr, u8 reg, u8 *ps, u8 length)
{
    u8 t,r=FALSE;
    
    if(!Write_Enable(addr)) {
        return FALSE;
    }    
    
    t = I2CWriteBytes(addr, reg, ps, length);
    if(t==FALSE) {
        LOGE("___ I2CWriteSerial failed\n");
    }
    r = Write_Disable(addr);
    if(t && r) {
        r = TRUE;
    }
    else {
        r = FALSE;
    }
    
    return r;
}

static u8 I2CReadSerial(u8 addr, u8 reg, u8 *ps, u8 length)
{
    return I2CReadBytes(addr, reg, ps, length);
}


static void io_init(void)
{
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_I2C1);
    
    gpio_af_set(GPIOB, GPIO_AF_4, GPIO_PIN_10);  //SCL
    gpio_af_set(GPIOB, GPIO_AF_4, GPIO_PIN_11);  //SDA

    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_10);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_OD, GPIO_OSPEED_25MHZ, GPIO_PIN_10);
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_11);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_OD, GPIO_OSPEED_25MHZ, GPIO_PIN_11);
    
    i2c_clock_config(I2C_PORT, 100000, I2C_DTCY_2);
    i2c_mode_addr_config(I2C_PORT, I2C_I2CMODE_ENABLE, I2C_ADDFORMAT_7BITS, GD_Address); //?????
    i2c_enable(I2C_PORT);
    i2c_ack_config(I2C_PORT, I2C_ACK_ENABLE); 
}
static void io_deinit(void)
{
    i2c_deinit(I2C_PORT);
}



#if 0
static void i2c_irq_init(void)
{
    //nvic_priority_group_set();
    nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);
    nvic_irq_enable(I2C1_EV_IRQn, 0, 2);
    nvic_irq_enable(I2C1_ER_IRQn, 0, 1);
    
    i2c_interrupt_enable(I2C_PORT, I2C_INT_ERR);
    i2c_interrupt_enable(I2C_PORT, I2C_INT_EV);
    i2c_interrupt_enable(I2C_PORT, I2C_INT_BUF);
}
void i2c1_event_irq_handler(void)
{
    if(i2c_interrupt_flag_get(I2C_PORT, I2C_INT_FLAG_ADDSEND)) {
        /* clear the ADDSEND bit */
        i2c_interrupt_flag_clear(I2C_PORT, I2C_INT_FLAG_ADDSEND);
    } else if((i2c_interrupt_flag_get(I2C_PORT, I2C_INT_FLAG_TBE)) && (!i2c_interrupt_flag_get(I2C_PORT, I2C_INT_FLAG_AERR))) {
        /* send a data byte */
        //i2c_data_transmit(I2C_PORT, *i2c_txbuffer++);
    }
}
void i2c1_error_irq_handler(void)
{
    /* no acknowledge received */
    if(i2c_interrupt_flag_get(I2C_PORT, I2C_INT_FLAG_AERR)) {
        i2c_interrupt_flag_clear(I2C_PORT, I2C_INT_FLAG_AERR);
    }

    /* SMBus alert */
    if(i2c_interrupt_flag_get(I2C_PORT, I2C_INT_FLAG_SMBALT)) {
        i2c_interrupt_flag_clear(I2C_PORT, I2C_INT_FLAG_SMBALT);
    }

    /* bus timeout in SMBus mode */
    if(i2c_interrupt_flag_get(I2C_PORT, I2C_INT_FLAG_SMBTO)) {
        i2c_interrupt_flag_clear(I2C_PORT, I2C_INT_FLAG_SMBTO);
    }

    /* over-run or under-run when SCL stretch is disabled */
    if(i2c_interrupt_flag_get(I2C_PORT, I2C_INT_FLAG_OUERR)) {
        i2c_interrupt_flag_clear(I2C_PORT, I2C_INT_FLAG_OUERR);
    }

    /* arbitration lost */
    if(i2c_interrupt_flag_get(I2C_PORT, I2C_INT_FLAG_LOSTARB)) {
        i2c_interrupt_flag_clear(I2C_PORT, I2C_INT_FLAG_LOSTARB);
    }

    /* bus error */
    if(i2c_interrupt_flag_get(I2C_PORT, I2C_INT_FLAG_BERR)) {
        i2c_interrupt_flag_clear(I2C_PORT, I2C_INT_FLAG_BERR);
    }

    /* CRC value doesn't match */
    if(i2c_interrupt_flag_get(I2C_PORT, I2C_INT_FLAG_PECERR)) {
        i2c_interrupt_flag_clear(I2C_PORT, I2C_INT_FLAG_PECERR);
    }

    i2c_interrupt_disable(I2C_PORT, I2C_INT_ERR);
    i2c_interrupt_disable(I2C_PORT, I2C_INT_BUF);
    i2c_interrupt_disable(I2C_PORT, I2C_INT_EV);
}
#endif

#endif


////////////////////////////////////////////////////
int sd30xx_init(void)
{
    io_init();
    return 0;
}


int sd30xx_deinit(void)
{
    io_deinit();
    return 0;
}


int sd30xx_write_time(sd_time_t *tm)
{
    int r=-1;
    u8 t,buf[7];
    
    buf[0] = tm->second;
    buf[1] = tm->minute;
    buf[2] = tm->hour;
    buf[3] = tm->week;
    buf[4] = tm->day;
    buf[5] = tm->month;
    buf[6] = tm->year;
    
    if(!Write_Enable(RTC_Address)) {
        LOGE("___ sd30xx_write_time failed 111\n");
        goto quit;
    }

    t = I2CWriteSerial(RTC_Address, 0, buf, sizeof(buf));
    if(t==FALSE) {
        LOGE("___ sd30xx_write_time failed 222\n");
    }

    if(!Write_Disable(RTC_Address)) {
        LOGE("___ sd30xx_write_time failed 333\n");
        goto quit;
    }
    
    if(t==TRUE) {
        r = 0;
    }
    
quit:
    return r;
}


int sd30xx_read_time(sd_time_t *tm)
{
    u8 r,buf[7]={0};
    int x=-1;
    
    r = I2CReadSerial(RTC_Address, 0, buf, sizeof(buf));
    if(r==FALSE) {
        LOGE("___ sd30xx_read_time failed\n");
        goto quit;
    }
    tm->second = buf[0];
    tm->minute = buf[1];
    tm->hour   = buf[2];
    tm->week   = buf[3];
    tm->day    = buf[4];
    tm->month  = buf[5];
    tm->year   = buf[6];
    x = 0;
    
quit:
    return x;
}


int sd30xx_set_countdown(sd_countdown_t *cd)
{
    u8 i;
    int r=-1;
    u8 buf[6],tmp[6]={0};
    u8 tem,bak;
    
    buf[0] = (cd->im<<6)|0xB0;                          //10H
    buf[1] = cd->clk<<4;                                //11H
    buf[2] = 0;                                         //12H
    buf[3] = cd->val & 0x0000FF;                        //13H
    buf[4] = (cd->val & 0x00FF00) >> 8;                 //14H
    buf[5] = (cd->val & 0xFF0000) >> 16;                //15H
    
    if(!I2CWriteSerial(RTC_Address, CTR2+1, buf+1, 5)) {
        LOGE("___ sd30xx_set_countdown failed 111\n");
        goto quit;
    }
    
    tem = buf[0];
    if(!I2CWriteSerial(RTC_Address, CTR2, &tem, 1)) {
        LOGE("___ sd30xx_set_countdown failed 222\n");
        goto quit;
    }
    
    tem |= 0x04;
    if(!I2CWriteSerial(RTC_Address, CTR2, &tem, 1)) {
        LOGE("___ sd30xx_set_countdown failed 333\n");
        goto quit;
    }
    r = 0;
    
quit:
    return r;
}


int sd30xx_set_alarm(u8 en, sd_time_t *tm)
{
    int r=-1;
    u8 buf[10];

    buf[0] = tm->second;
    buf[1] = tm->minute;
    buf[2] = tm->hour;
    buf[3] = 0;
    buf[4] = tm->day;
    buf[5] = tm->month;
    buf[6] = tm->year;
    buf[7] = en;
    buf[8] = 0xFF;
    buf[9] = 0x92;
    
    if(!I2CWriteSerial(RTC_Address, Alarm_SC, buf, 10)) {
        LOGE("___ sd30xx_set_alarm failed\n");
        goto quit;
    }
    r = 0;
    
quit:
    return r;
}


int sd30xx_set_freq(SD_FREQ freq)
{
    int r=-1;
    u8 buf[2];

    buf[0] = 0xA1;
    buf[1] = freq;
    
    if(!I2CWriteSerial(RTC_Address, CTR2, buf, 2)) {
        LOGE("___ sd30xx_set_freq failed\n");
        goto quit;
    }
    r = 0;
    
quit:
    return r;
}


int sd30xx_set_irq(SD_IRQ irq, u8 on)
{
    u8 buf;
    int r=-1;
    u8 flag[3]={INTFE,INTAE,INTDE};

    if(on) {
        buf = 0x80 | flag[irq];
    }
    else {
        buf = 0x80 | (~flag[irq]);
    }
    
    if(!I2CWriteSerial(RTC_Address, CTR2, &buf, 1)) {
        LOGE("___ sd30xx_set_irq failed\n");
        goto quit;
    }
    r = 0;
    
quit:
    return r;
}


int sd30xx_clr_irq(void)
{
    int i,r=-1;
    u8 tmp[3],bak[3];

#ifdef USE_ARST
    r = I2CReadSerial(RTC_Address, CTR3, &tmp, 1);
    LOGD("CTR1: 0x%02x\n", buf);
#else
    if(!I2CReadSerial(RTC_Address, CTR1, tmp, 3)) {
        LOGE("___ sd30xx_clr_irq failed 111\n");
        goto quit;
    }
    tmp[0] &= 0xcf; tmp[0] |= 0x84;
    tmp[1] |= 0x80;
    
    if(!I2CWriteSerial(RTC_Address, CTR1, tmp, 3)) {
        LOGE("___ sd30xx_clr_irq failed 222\n");
        goto quit;
    }
    
    if(!I2CReadSerial(RTC_Address, CTR1, bak, 3)) {
        LOGE("___ sd30xx_clr_irq failed 333\n");
        goto quit;
    }
    
    bak[0] |= 0x84; bak[1] |= 0x80;
    if(memcmp(tmp,bak,3)) {
        LOGE("___ sd30xx_clr_irq, memcmp failed\n");
        for(i=0; i<3; i++) {
            LOGD("___reg[0x%02x] w:0x%02x, r:0x%02x\n", CTR1+i, tmp[i], bak[i]);
        }
        goto quit;
    }
#endif
    r = 0;
    
quit:
    return r;
}


int sd30xx_read_id(U8 *buf, U8 len)
{
    int r=-1;
    
    if(!buf || len<8) {
        return -1;
    }
    
    if(!I2CReadSerial(RTC_Address, ID_Address, buf, 8)) {
        LOGE("___ sd30xx_read_id failed\n");
        goto quit;
    }
    r = 0;
    
quit:
    return r;
}


int sd30xx_set_charge(U8 on)
{
    u8 buf;
    int r=-1;

    buf = (on?Chg_enable:Chg_disable);
    if(!I2CWriteSerial(RTC_Address, Chg_MG, &buf, 1)) {
        LOGE("___ sd30xx_set_charge failed\n");
        goto quit;
    }
    r = 0;
    
quit:
    return r;
}


int sd30xx_get_volt(F32 *volt)
{
    u8 buf[2];
    u16 vbat;
    f32 tmp;
    int r=-1;

    if(!I2CReadSerial(RTC_Address, Bat_H8, buf, 2)) {
        LOGE("___ sd30xx_get_volt failed\n");
        goto quit;
    }

    vbat = (buf[0]>>7)*255 + buf[1];
    if(volt) *volt = vbat/100 + (vbat%100)/100.0F + (vbat%10)/100.0F;
    r = 0;
    
quit:
    return r;
}


int sd30xx_get_info(sd_info_t *info)
{
    int r=-1;
    u8 buf[2];
    u8 irq,first;
    SD_IRQ IRQ[3]={IRQ_NONE,IRQ_COUNTDOWN,IRQ_ALARM};

    if(!info) {
        return -1;
    }
    
    if(!I2CReadSerial(RTC_Address, CTR1, buf, 2)) {
        goto quit;
    }
    
    irq   = (buf[0]&0x30)>>4;
    first = (buf[0]&0x01)?1:0;
    
    info->irq = IRQ[irq];
    info->first = first;
    r = 0;
    LOGD("___ CTR1: 0x%02x, CTR2: 0x%02x\n", buf[0], buf[1]);
    
quit:
    return r;
}


int sd30xx_read(U8 reg, U8 *buf, U8 len)
{
    int r=-1;
    
    if(I2CReadSerial(RTC_Address, reg, buf, len)) {
        r = 0;
    }
    return r;
}


int sd30xx_write(U8 reg, U8 *buf, U8 len)
{
    int r=-1;
    
    if(I2CWriteSerial(RTC_Address, reg, buf, len)) {
        r = 0;
    }
    return r;
}


void sd30xx_test(void)
{
    int i,cnt=6;
    #define CNT  2
    u8 tmp[CNT]={0x84,0x80};
    u8 bak[CNT]={0,0};
    
#if 0
    I2CWriteBytes(RTC_Address,  CTR2, &tmp[1], 1);
    I2CWriteBytes(RTC_Address,  CTR1, &tmp[0], 1);
    
    
    bak[0] = bak[1] = 0;
    I2CReadBytes(RTC_Address,  CTR1, bak, CNT);
    LOGD("___1___ CTR1:0x%02x, CTR2:0x%02x\n", bak[0], bak[1]);
    
    tmp[0] = 0x84;  tmp[1] = 0x01|0x80;
    I2CWriteBytes(RTC_Address,  CTR1, tmp, 2);

    tmp[0] &= 0x7b;  tmp[1] &= 0x7f;
    I2CWriteBytes(RTC_Address,  CTR1, tmp, 2);

    
    bak[0] = bak[1] = 0xff;
    I2CReadBytes(RTC_Address,  CTR1, bak, CNT);
    LOGD("___2___ CTR1:0x%02x, CTR2:0x%02x\n", bak[0], bak[1]);
#else
    Write_Enable(RTC_Address);
    Write_Disable(RTC_Address);

#endif
}


