#include "sd30xx.h"
#include "gd32f4xx_gpio.h"
#include "dal_gpio.h"
#include "dal_si2c.h"
#include "dal_delay.h"
#include "log.h"
#include "cfg.h"



#define DELAYP_CNT              (100)
#define RTC_Address             0x64
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


//PB2 -- SCL
//PB1 -- SDA
#define SCL_H         gpio_bit_write(GPIOB, GPIO_PIN_2, SET)
#define SCL_L         gpio_bit_write(GPIOB, GPIO_PIN_2, RESET)
#define SDA_H         gpio_bit_write(GPIOB, GPIO_PIN_1, SET)
#define SDA_L         gpio_bit_write(GPIOB, GPIO_PIN_1, RESET)
#define SDA_READ      gpio_input_bit_get(GPIOB, GPIO_PIN_1)


static void I2Cdelay(void)
{
   U32 cnt=DELAYP_CNT;      //这里可以优化速度，通常延时3~10us，可以用示波器看波形来调试
   while(cnt--);
}
static u8 I2CStart(void)
{
    SDA_H;
    I2Cdelay();
    SCL_H;
    I2Cdelay();
    if(!SDA_READ) {
        return FALSE; //SDA线为低电平则总线忙,退出
    }
    SDA_L;
    I2Cdelay();
    SCL_L;
    I2Cdelay();
    return TRUE;
}
static void I2CStop(void)
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
static void I2CAck(void)
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
static void I2CNoAck(void)
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
static u8 I2CWaitAck(u8 fail_stop)
{
    SCL_L;
    I2Cdelay();
    SDA_H;
    I2Cdelay();
    SCL_H;
    I2Cdelay();
    if(SDA_READ) {
        SCL_L;
        I2CStop();
        return FALSE;
    }
    SCL_L;
    
    return TRUE;
}
static void I2CSendByte(u8 SendByte) //数据从高位到低位
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
static u8 I2CReceiveByte(void)
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

static u8 Write_Enable(void)
{
    if(!I2CStart()) {
        LOGE("___ Write_Enable failed 111\n");
        return FALSE;
    }

    I2CSendByte(RTC_Address);
    if(!I2CWaitAck(1)) {
        LOGE("___ Write_Enable failed 222\n");
        return FALSE;
    }

    I2CSendByte(CTR2);
    if(!I2CWaitAck(1)) {
        LOGE("___ Write_Enable failed 333\n");
        return FALSE;
    }
    
    I2CSendByte(0x80);//置WRTC1=1
    if(!I2CWaitAck(1)) {
        LOGE("___ Write_Enable failed 444\n");
        return FALSE;
    }
    I2CStop();

    if(!I2CStart()) {
        LOGE("___ Write_Enable failed 555\n");
        return FALSE;
    }
    
    I2CSendByte(RTC_Address);
    if(!I2CWaitAck(1)) {
        LOGE("___ Write_Enable failed 666\n");
        return FALSE;
    }
    
    I2CSendByte(CTR1);
    if(!I2CWaitAck(1)) {
        LOGE("___ Write_Enable failed 777\n");
        return FALSE;
    }
    
    I2CSendByte(0x84);//置WRTC2,WRTC3=1
    if(!I2CWaitAck(1)) {
        LOGE("___ Write_Enable failed 888\n");
        return FALSE;
    }
    
    I2CStop();
    return TRUE;
}
static u8 Write_Disable(void)
{
    if(!I2CStart()) {
        LOGE("___ Write_Disable failed 111\n");
        return FALSE;
    }

    I2CSendByte(RTC_Address);
    if(!I2CWaitAck(1)) {
        LOGE("___ Write_Disable failed 222\n");
        return FALSE;
    }

    I2CSendByte(CTR1);//设置写地址0FH
    if(!I2CWaitAck(1)) {
        LOGE("___ Write_Disable failed 333\n");
        return FALSE;
    }
    
    I2CSendByte(0x0) ;//置WRTC2,WRTC3=0
    if(!I2CWaitAck(1)) {
        LOGE("___ Write_Disable failed 444\n");
        return FALSE;
    }
    
    I2CSendByte(0x0) ;//置WRTC1=0(10H地址)
    if(!I2CWaitAck(1)) {
        LOGE("___ Write_Disable failed 555\n");
        return FALSE;
    }
    
    I2CStop();
    return TRUE;
}
static u8 I2CWriteSerial(u8 addr, u8 reg, u8 *ps, u8 length)
{
    if(!Write_Enable()) {
        return FALSE;
    }

    if(!I2CStart()) {
        LOGE("___ I2CWriteSerial failed 111\n");
        return FALSE;
    }
    
    I2CSendByte(addr);
    if(!I2CWaitAck(1)) {
        LOGE("___ I2CWriteSerial failed 222\n");
        return FALSE;
    }

    I2CSendByte(reg);
    
    if(!I2CWaitAck(1)) {
        LOGE("___ I2CWriteSerial failed 333\n");
        return FALSE;
    }
    
    for(;(length--)>0;) {
        I2CSendByte(*(ps++));
        I2CAck();
    }
    I2CStop();

    return Write_Disable();
}

static u8 I2CReadSerial(u8 addr, u8 reg, u8 *ps, u8 length)
{
    if(!I2CStart()) {
        LOGE("___ I2CReadSerial failed 111\n");
        return FALSE;
    }
    
    I2CSendByte(addr);
    if(!I2CWaitAck(1)) {
        LOGE("___ I2CReadSerial failed 222\n");
        return FALSE;
    }

    I2CSendByte(reg);
    
    if(!I2CWaitAck(1)) {
        LOGE("___ I2CReadSerial failed 333\n");
        return FALSE;
    }
    
    I2CStart();
    I2CSendByte(addr+1);
    
    if(!I2CWaitAck(1)) {
        LOGE("___ I2CReadSerial failed 444\n");
        return FALSE;
    }
    
    for(;--length>0;ps++) {
        *ps = I2CReceiveByte();
        I2CAck();
    }
    *ps = I2CReceiveByte();
    
    I2CNoAck();
    I2CStop();
    
    return TRUE;
}
static void io_config(void)
{
    U32 pin=GPIO_PIN_1|GPIO_PIN_2;

    rcu_periph_clock_enable(RCU_GPIOB);
    
    gpio_bit_write(GPIOB, pin, SET);
    gpio_mode_set(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, pin);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_OD, GPIO_OSPEED_2MHZ, pin); 
    
    //I2CStop();
}

////////////////////////////////////////////////////
int sd30xx_init(void)
{
    int r;
    io_config();

    return 0;
}


int sd30xx_write_time(sd_time_t *tm)
{
    u8 r;
    
    if(!Write_Enable()) {
        LOGE("___ sd30xx_write_time failed 111\n");
        return -1;
    }

    if(!I2CStart()) {
        LOGE("___ sd30xx_write_time failed 222\n");
        return -1;
    }
    
    I2CSendByte(RTC_Address);
    if(!I2CWaitAck(1)) {
        LOGE("___ sd30xx_write_time failed 333\n");
        return -1;
    }

    I2CSendByte(0);                 //设置写起始地址
    if(!I2CWaitAck(1)) {
        LOGE("___ sd30xx_write_time failed 444\n");
        return -1;
    }
    
    I2CSendByte(tm->second);        //second
    if(!I2CWaitAck(1)) {
        LOGE("___ sd30xx_write_time failed 555\n");
        return -1;
    }
    
    I2CSendByte(tm->minute);        //minute
    if(!I2CWaitAck(1)) {
        LOGE("___ sd30xx_write_time failed 666\n");
        return -1;
    }
    
    I2CSendByte(tm->hour|0x80);     //hour ,同时设置小时寄存器最高位（0：为12小时制，1：为24小时制）
    if(!I2CWaitAck(1)) {
        LOGE("___ sd30xx_write_time failed 777\n");
        return -1;
    }
    
    I2CSendByte(tm->week);          //week
    if(!I2CWaitAck(1)) {
        LOGE("___ sd30xx_write_time failed 888\n");
        return -1;
    }
    
    I2CSendByte(tm->day);           //day
    if(!I2CWaitAck(1)) {
        LOGE("___ sd30xx_write_time failed 999\n");
        return -1;
    }
    
    I2CSendByte(tm->month);         //month
    if(!I2CWaitAck(1)) {
        LOGE("___ sd30xx_write_time failed aaa\n");
        return -1;
    }
    
    I2CSendByte(tm->year);          //year
    if(!I2CWaitAck(1)) {
        LOGE("___ sd30xx_write_time failed bbb\n");
        return -1;
    }
    I2CStop();

    if(!Write_Disable()) {
        LOGE("___ sd30xx_write_time failed ccc\n");
        return -1;
    }
    
    return  0;
}


int sd30xx_read_time(sd_time_t *tm)
{
    u8 r;
    
    if(!I2CStart()) {
        LOGE("___ sd30xx_read_time failed 111\n");
        return -1;
    }
    
    I2CSendByte(RTC_Address);
    if(!I2CWaitAck(1)) {
        LOGE("___ sd30xx_read_time failed 222\n");
        return -1;
    }

    I2CSendByte(0);
    if(!I2CWaitAck(1)) {
        LOGE("___ sd30xx_read_time failed 333\n");
        return -1;
    }
    
    if(!I2CStart()) {
        LOGE("___ sd30xx_read_time failed 444\n");
        return -1;
    }
    
    I2CSendByte(RTC_Address+1);
    if(!I2CWaitAck(1)) {
        LOGE("___ sd30xx_read_time failed 555\n");
        return -1;
    }
    
    tm->second=I2CReceiveByte();
    I2CAck();
    tm->minute=I2CReceiveByte();
    I2CAck();
    tm->hour=I2CReceiveByte() & 0x7F;
    I2CAck();
    tm->week=I2CReceiveByte();
    I2CAck();
    tm->day=I2CReceiveByte();
    I2CAck();
    tm->month=I2CReceiveByte();
    I2CAck();
    tm->year=I2CReceiveByte();
    
    I2CNoAck();
    I2CStop();

    return 0;
}


int sd30xx_set_countdown(sd_countdown_t *cd)
{
    u8 i;
    u8 buf[6];
    u8 tmp[6]={0};
    u8 tem=0xF0;
    
    buf[0] = (cd->im<<6)|0xB4;                          //10H
    buf[1] = cd->clk<<4;                                //11H
    buf[2] = 0;                                         //12H
    buf[3] = cd->val & 0x0000FF;                        //13H
    buf[4] = (cd->val & 0x00FF00) >> 8;                 //14H
    buf[5] = (cd->val & 0xFF0000) >> 16;                //15H
    if(!I2CWriteSerial(RTC_Address, CTR2, &tem, 1)) {
        LOGE("___ sd30xx_set_countdown failed 111\n");
        return -1;
    }
    
    if(!I2CWriteSerial(RTC_Address, CTR2, buf, 6)) {
        LOGE("___ sd30xx_set_countdown failed 222\n");
        return -1;
    }
    
    if(!I2CReadSerial(RTC_Address, CTR2, tmp, 6)) {
        LOGE("___ sd30xx_set_countdown failed 333\n");
        return -1;
    }
    
    tmp[0] |= 0x80;
    if(memcmp(buf, tmp, 6)) {
        
        LOGD("\n");
        for(i=0; i<6; i++) {
            LOGD("rtc reg[%d]: write:0x%02x, read:0x%02x\n", i, buf[i], tmp[i]);
        }
        LOGD("\n");
        
        return -1;
    }
    
    return 0;
}


int sd30xx_set_alarm(u8 en, sd_time_t *tm)
{
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
        return -1;
    }

    return 0;
}


int sd30xx_set_freq(SD_FREQ freq)
{
    u8 buf[2];

    buf[0] = 0xA1;
    buf[1] = freq;
    if(!I2CWriteSerial(RTC_Address, CTR2, buf, 2)) {
        LOGE("___ sd30xx_set_freq failed\n");
        return -1;
    }
    return 0;
}


int sd30xx_set_irq(SD_IRQ irq, u8 on)
{
    u8 buf;
    u8 flag[3]={INTFE,INTAE,INTDE};

    if(on) {
        buf = 0x80 | flag[irq];
    }
    else {
        buf = 0x80 | (~flag[irq]);
    }
    
    if(!I2CWriteSerial(RTC_Address, CTR2, &buf, 1)) {
        LOGE("___ sd30xx_set_irq failed\n");
        return -1;
    }

    return 0;
}


int sd30xx_clr_irq(void)
{
    u8 buf;

#ifdef USE_ARST
    r = I2CReadSerial(RTC_Address, CTR3, &buf, 1);
    LOGD("CTR1: 0x%02x\n", buf);
#else
    if(!I2CReadSerial(RTC_Address, CTR1, &buf, 1)) {
        LOGE("___ sd30xx_clr_irq failed 111\n");
        return -1;
    }
    
    //LOGD("CTR1: 0x%02x\n", buf);
    buf &= 0xcf;
    if(!I2CWriteSerial(RTC_Address, CTR2, &buf, 1)) {
        LOGE("___ sd30xx_clr_irq failed 222\n");
        return -1;
    }
#endif
    
    return 0;
}


int sd30xx_read_id(U8 *buf, U8 len)
{
    if(!buf || len<8) {
        return -1;
    }
    
    if(!I2CReadSerial(RTC_Address, ID_Address, buf, 8)) {
        LOGE("___ sd30xx_read_id failed\n");
        return -1;
    }

    return 0;
}



int sd30xx_set_charge(U8 on)
{
    u8 buf;

    buf = (on?Chg_enable:Chg_disable);
    if(!I2CWriteSerial(RTC_Address, Chg_MG, &buf, 1)) {
        LOGE("___ sd30xx_set_charge failed\n");
        return -1;
    }

    return 0;
}


int sd30xx_get_volt(F32 *volt)
{
    u8 buf[2];
    u16 vbat;
    f32 tmp;

    if(!I2CReadSerial(RTC_Address, Bat_H8, buf, 2)) {
        LOGE("___ sd30xx_get_volt failed\n");
        return -1;
    }

    vbat = (buf[0]>>7)*255 + buf[1];
    if(volt) *volt = vbat/100 + (vbat%100)/100.0F + (vbat%10)/100.0F;

    return 0;
}


int sd30xx_first(void)
{
    u8 buf=0;
    u8 first=0;

    if(!I2CReadSerial(RTC_Address, CTR1, &buf, 1)) {
        return -1;
    }
    
    if(buf&0x01) {
        return first=1;
    }
    
    return first;
}


int sd30xx_read(U8 reg, U8 *buf, U8 len)
{
    if(I2CReadSerial(RTC_Address, reg, buf, len)) {
        return 0;
    }
    
    return -1;
}

