#include "drv/i2c.h"
#include "drv/log.h"
#include "drv/gpio.h"
#include "drv/delay.h"
#include "dev/drv260x.h"

#define DRV260X_EN_PIN          HAL_GPIO_2
#define DRV260X_SCL_PIN         HAL_GPIO_36
#define DRV260X_SDA_PIN         HAL_GPIO_37



static i2c_info_t drv260x_i2c_info;
static int bus_init(void)
{
    int r;
    i2c_cfg_t cfg;
    
    gpio_init(DRV260X_EN_PIN, HAL_GPIO_DIRECTION_OUTPUT, HAL_GPIO_DATA_HIGH);
    delay_ms(2);
    
    gpio_init(HAL_GPIO_38, HAL_GPIO_DIRECTION_INPUT, HAL_GPIO_DATA_LOW);
    gpio_init(HAL_GPIO_39, HAL_GPIO_DIRECTION_INPUT, HAL_GPIO_DATA_LOW);
    
    cfg.pin.scl_pin = DRV260X_SCL_PIN;
    cfg.pin.sda_pin = DRV260X_SDA_PIN;
    cfg.freq = HAL_I2C_FREQUENCY_100K;
    r = i2c_init(&cfg, &drv260x_i2c_info);
    
    return r;
}

static int read_regs(U8 reg, U8 *data, U8 len)
{
    int r=0;
    
    r |= i2c_write(drv260x_i2c_info.port, DRV260X_ADDR, &reg, 1);
    r |= i2c_read(drv260x_i2c_info.port,  DRV260X_ADDR, data, len);
    
    return r;
}
static int write_regs(U8 reg, U8 *data, U8 len)
{
    int r=0;
    U8 tmp[len+1];
    
    tmp[0] = reg;
    memcpy(tmp+1, data, len);
    r |= i2c_write(drv260x_i2c_info.port, DRV260X_ADDR, tmp, len+1);
    return r;
}

static U8 read_reg(U8 reg)
{
    U8 tmp;
    read_regs(reg, &tmp, 1);
    
    return tmp;
}
static int write_reg(U8 reg, U8 data)
{
    return write_regs(reg, &data, 1);
}
static void print_reg(char *s)
{
    U8 i,tmp;
    
    
    for(i=0; i<=0x22; i++) {
        read_regs(i, &tmp, 1);
        LOGD("%s reg[0x%02x]: 0x%02x\r\n", s, i, tmp);
    };
}



int drv260x_init(void)
{
    U8 id=0;
    U8 i,tmp[0x22];
    
    bus_init();
    id = read_reg(DRV260X_REG_STATUS);
    LOGD("__________chipid: 0x%02x\r\n", id);
  
    //print_reg("111");

#if 0
    write_reg(DRV260X_REG_MODE, 0x00);      // out of standby
    write_reg(DRV260X_REG_RTPIN, 0x00);     // no real-time-playback
    write_reg(DRV260X_REG_WAVESEQ1, 1);     // strong click
    write_reg(DRV260X_REG_WAVESEQ2, 0);     // end sequence
    write_reg(DRV260X_REG_OVERDRIVE, 0);    // no overdrive
    write_reg(DRV260X_REG_SUSTAINPOS, 0);
    write_reg(DRV260X_REG_SUSTAINNEG, 0);
    write_reg(DRV260X_REG_BREAK, 0);
    write_reg(DRV260X_REG_AUDIOMAX, 0x64);

    // ERM open loop
    
    // turn off N_ERM_LRA
    write_reg(DRV260X_REG_FEEDBACK, read_reg(DRV260X_REG_FEEDBACK)&0x7F);
    // turn on ERM_OPEN_LOOP
    write_reg(DRV260X_REG_CONTROL3, read_reg(DRV260X_REG_CONTROL3)|0x20);
#else
    write_reg(DRV260X_REG_MODE, 0x00);
    write_reg(DRV260X_REG_LIBRARY, 3);
    write_reg(DRV260X_REG_MODE, DRV260X_MODE_INTTRIG);
    write_reg(DRV260X_REG_WAVESEQ1, 7);
    
    write_reg(DRV260X_REG_GO, 1);
     
    //print_reg("222");
    
    
    
    
#endif

    return 0;
}


U8 drv260x_get_chipid(void)
{
    return read_reg(DRV260X_REG_STATUS);
}



/*
  @brief Select the haptic waveform to use.
  @param slot The waveform slot to set, from 0 to 7
  @param w The waveform sequence value, refers to an index in the ROM library.

    Playback starts at slot 0 and continues through to slot 7, stopping if it
  encounters a value of 0. 
*/
int drv260x_set_waveform(U8 slot, U8 wave)
{
    return write_reg(DRV260X_REG_WAVESEQ1+slot, wave);
}


/*
  @brief Select the waveform library to use.
  @param lib Library to use, 0 = Empty, 1-5 are ERM, 6 is LRA.
*/
int drv260x_set_library(U8 lib) {
    return write_reg(DRV260X_REG_LIBRARY, lib);
}


/*
  @brief Set the device mode.
  @param mode Mode value, see datasheet section 7.6.2
    0: Internal trigger, call go() to start playback
    1: External trigger, rising edge on IN pin starts playback
    2: External trigger, playback follows the state of IN pin
    3: PWM/analog input
    4: Audio
    5: Real-time playback
    6: Diagnostics\n
    7: Auto calibration
*/
int drv260x_set_mode(U8 mode)
{
    return write_reg(DRV260X_REG_MODE, mode);
}


/*
  @brief Set the realtime value when in RTP mode, used to directly drive the
  haptic motor.
  @param rtp 8-bit drive value.
*/
int drv260x_set_realtime(U8 rtp)
{
    return write_reg(DRV260X_REG_RTPIN, rtp);
}



/*
  ERM: Eccentric Rotating Mass mode
  LRA: Linear Resonance Actuator mode
*/
int drv260x_set_type(int type) 
{
    U8 val,tmp;
    
    val = read_reg(DRV260X_REG_FEEDBACK);
    if(type==ERM) {
        val &= 0x7f;
    }
    else if(type==LRA) {
        val |= 0x80;
    }
    else {
        return -1;
    }
    
    return write_reg(DRV260X_REG_FEEDBACK, val);
}


int drv260x_start(void)
{
    return write_reg(DRV260X_REG_GO, 1);
}


int drv260x_stop(void) 
{
    return write_reg(DRV260X_REG_GO, 0);
}


///////////////////////////////////////////////////
static U8 effect=1;
static void drv_loop(void)
{
    int r;
    
    if(effect>117) {
        effect = 1;
    }
    
    LOGD("effect: %d\r\n", effect);
    
    r = drv260x_set_waveform(0, effect++);  // play effect 
    r = drv260x_set_waveform(1, effect++);
    r = drv260x_set_waveform(2, effect++);
    r = drv260x_set_waveform(3, effect++);
    r = drv260x_set_waveform(4, effect++);
    r = drv260x_set_waveform(5, effect++);
    r = drv260x_set_waveform(6, effect++);
    r = drv260x_set_waveform(7, 0);         // end waveform
    
    r = drv260x_start();
}
static void drv_setup(void)
{
    int r;
    
    
}
int drv260x_test(void)
{
    int r;
    
    r = drv260x_init();
    
    //r = drv260x_set_mode(DRV260X_MODE_INTTRIG);
    //r = drv260x_set_library(1);  
    
    while(1) {
        drv_loop();
        //LOGD("status 0x%02x\r\n", drv260x_get_chipid());
	    delay_ms(200);
    }
    
    return 0;
}


