#include "ads9120.h"
#include "dal_spi.h"
#include "dal_pwm.h"
#include "dal_gpio.h"
#include "dal_delay.h"
#include "dal_dac.h"
#include "paras.h"
#include "log.h"
#include "cfg.h"


#ifndef ADC_BORD_V11
    #define COV_SCS_TIE       //B5_G9
#endif

#if (defined SPI_PWM_CS)
    #define USE_DMA
#endif

///////////////////////////////////////////////////////


#define VREF        (4.096F)
#define LSB         ((VREF*2)/65536)
#define ADCTO(x)    (x<=0x7fff)?(x*LSB):((x-0xffff)*LSB)

#define DELAY_CNT   100

#define PWR_CTL     0x10
#define SDI_CTL     0x14
#define SDO_CTL     0x18
#define DAT_CTL     0x1c

enum {
    PIN_SCK=0,
    PIN_IO0,
    PIN_IO1,
    PIN_IO2,
    PIN_IO3,
    PIN_CS ,
    PIN_SDI,
    PIN_RST,
    PIN_COV,
    PIN_RVS,
    PIN_PWR,
    
    PIN_MAX
};
typedef struct {
    dal_gpio_cfg_t  cfg;
}ads_pin_info_t;
static ads_pin_info_t adsPinInfo[PIN_MAX]={
    {{GPIOG, GPIO_PIN_13, MODE_OUT_PP, PULL_NONE, 1}},  //SCK
    {{GPIOG, GPIO_PIN_14, MODE_IN,     PULL_NONE, 1}},  //IO0       //对应gd32 mosi, 连接ads sdo-0
    {{GPIOG, GPIO_PIN_12, MODE_IN,     PULL_NONE, 1}},  //IO1       //对应gd32 miso, 连接ads sdo-1
    {{GPIOG, GPIO_PIN_10, MODE_IN,     PULL_NONE, 1}},  //IO2
    {{GPIOG, GPIO_PIN_11, MODE_IN,     PULL_NONE, 1}},  //IO3
    {{GPIOB, GPIO_PIN_4 , MODE_OUT_PP, PULL_NONE, 1}},  //CS
    {{GPIOG, GPIO_PIN_9 , MODE_OUT_PP, PULL_NONE, 1}},  //SDI
    {{GPIOB, GPIO_PIN_3 , MODE_OUT_PP, PULL_NONE, 1}},  //RST
    {{GPIOB, GPIO_PIN_5 , MODE_OUT_PP, PULL_NONE, 1}},  //COV
    {{GPIOD, GPIO_PIN_6 , MODE_IN,     PULL_NONE, 1}},  //RVS
    
    {{GPIOA, GPIO_PIN_15, MODE_OUT_PP, PULL_NONE, 0}},  //PWR,  高开低关
    
};



typedef struct {
    ads9120_cfg_t cfg;
    handle_t      hspi;
    U8            inited;
    
    U8            spiMode;
    U8            spiQuad;
    U8            spiHard;
    U8            spiWidth;
    handle_t      hpin[PIN_MAX];
}ads_handle_t;



static void ads_set_cs(U8 hl);
static void ads_config(void);
static void ads_callback(U16 *rx, U32 cnt);


static ads_handle_t adsHandle;

static inline void ads_delay(void)
{
    int t=DELAY_CNT;
    while(t-->0);
}
static inline void ads_set_cs(U8 hl)
{
    handle_t handle=adsHandle.hpin[PIN_CS];
    dal_gpio_set_hl(handle, hl);
}
static inline void conv_start_once(void)
{
    int t=30,hl=0;
    handle_t hcov=adsHandle.hpin[PIN_COV];
    
    dal_gpio_set_hl(hcov, 0);
    while(t-->0);                    //min 30ns
    dal_gpio_set_hl(hcov, 1);
}
static inline void ads_other_init(void)
{
    dal_gpio_cfg_t gc;
    
    
#ifdef SPI_PWM_CS
    if(adsHandle.hpin[PIN_CS]) {
        
        conv_start_once();
        dal_gpio_deinit(adsHandle.hpin[PIN_CS]);
        adsHandle.hpin[PIN_CS] = NULL;
    }
    
    dal_pwm_init(adsHandle.cfg.freq);
#endif
}


static inline void ads_gpio_init(void)
{
    U8 i;
    dal_gpio_cfg_t cfg;
    
    for(i=0; i<PIN_MAX; i++) {
        adsHandle.hpin[i] = dal_gpio_init(&adsPinInfo[i].cfg);
    } 
}
static inline void sdox_deinit(void)
{
    handle_t *h = adsHandle.hpin;
    
    if(h[PIN_IO0]) {
        dal_gpio_deinit(h[PIN_IO0]);
        h[PIN_IO0] = NULL;
    }
    
    if(h[PIN_IO1]) {
        dal_gpio_deinit(h[PIN_IO1]);
        h[PIN_IO1] = NULL;
    }
    
    if(h[PIN_IO2]) {
        dal_gpio_deinit(h[PIN_IO2]);
        h[PIN_IO2] = NULL;
    }
    
    if(h[PIN_IO3]) {
        dal_gpio_deinit(h[PIN_IO3]);
        h[PIN_IO3] = NULL;
    }
}
static inline void sdox_init(void)
{
    dal_gpio_cfg_t cfg;
    handle_t *h = adsHandle.hpin;
    ads_pin_info_t *info=adsPinInfo;
    
    if(!h[PIN_IO0]) {
        cfg = info[PIN_IO0].cfg;
        
    #ifndef SPI_QUAD
        cfg.gpio.pin = GPIO_PIN_12;
    #endif  
        h[PIN_IO0] = dal_gpio_init(&cfg);
    }
    
    if(!h[PIN_IO1]) {
        cfg = info[PIN_IO1].cfg;
        
    #ifndef SPI_QUAD
        cfg.gpio.pin = GPIO_PIN_14;
    #endif
        
        h[PIN_IO1] = dal_gpio_init(&cfg);
    }
}

static inline void ads_spi_init(void)
{
    dal_spi_cfg_t cfg;
    handle_t *h = adsHandle.hpin;
    
    if(h[PIN_SCK]) {
        dal_gpio_deinit(h[PIN_SCK]);
        h[PIN_SCK] = NULL;
    }
    
    sdox_deinit();
    
    if(!adsHandle.hspi) {
        cfg.port   = SPI_5;
        cfg.mode   = (adsHandle.spiMode+1)%4;
        
        cfg.bits   = adsHandle.spiWidth;
        cfg.isMsb  = 1;
        cfg.mast   = 1;
        cfg.softCs = 1;
        cfg.quad   = adsHandle.spiQuad;
        
#ifdef USE_DMA
        cfg.useDMA = 1;
#else
        cfg.useDMA = 0;
#endif
        if(cfg.useDMA) {
            cfg.buf = adsHandle.cfg.buf;
            cfg.callback = ads_callback;
        }
        adsHandle.hspi = dal_spi_init(&cfg);
        dal_spi_enable(1);
    }
}
static inline void ads_spi_deinit(void)
{
    dal_gpio_cfg_t cfg;
    handle_t *h = adsHandle.hpin;
    ads_pin_info_t *info=adsPinInfo;
    
    if(adsHandle.hspi) {
        dal_spi_deinit(adsHandle.hspi);
        adsHandle.hspi = NULL;
    }
    
    if(!h[PIN_SCK]) {
        h[PIN_SCK] = dal_gpio_init(&info[PIN_SCK].cfg);
    }

    if(!h[PIN_CS]) {
        h[PIN_CS] = dal_gpio_init(&info[PIN_CS].cfg);
    }
    
    sdox_init();
}

static inline void ads_to_acq(void)
{
    dal_gpio_set_hl(adsHandle.hpin[PIN_CS],  1);
    dal_gpio_set_hl(adsHandle.hpin[PIN_COV], 0);
    dal_gpio_set_hl(adsHandle.hpin[PIN_SCK], 0);
    
    dal_delay_ms(2);                //1.25ms 
}
static inline void ads_hw_reset(void)
{
    //power up time: max 0.25ms
    //wake  up time: max 0.30ms
    //ads9120 max sample freq: 2.5Mhz
    handle_t hrst=adsHandle.hpin[PIN_RST];
    
    dal_gpio_set_hl(hrst, 0);
    dal_delay_ms(10);                //min 100ns
    dal_gpio_set_hl(hrst, 1);
    dal_delay_ms(10);
    
    ads_to_acq();
}


/*
SPI 四种工作模式：
模式0：CPOL= 0 CPHA=0 SCK串行时钟线空闲是为低电平，数据在SCK时钟的上升沿被采样，数据在SCK时钟的下降沿切换, 第1个边缘采样
模式1：CPOL= 0 CPHA=1 SCK串行时钟线空闲是为低电平，数据在SCK时钟的下降沿被采样，数据在SCK时钟的上升沿切换，第2个边缘采样
模式2：CPOL= 1 CPHA=0 SCK串行时钟线空闲是为高电平，数据在SCK时钟的下降沿被采样，数据在SCK时钟的上升沿切换, 第1个边缘采样
模式3：CPOL= 1 CPHA=1 SCK串行时钟线空闲是为高电平，数据在SCK时钟的上升沿被采样，数据在SCK时钟的下降沿切换, 第2个边缘采样
*/
static inline U8 set_clk_idle(U8 mode, U8 on)
{
    U8 hl;
    handle_t hsck=adsHandle.hpin[PIN_SCK];
    
    if(mode==0 || mode==1) {    //低电平空闲
        hl = on?0:1;
        dal_gpio_set_hl(hsck, hl);
    }
    else {                      //高电平空闲
        hl = on?1:0;
        dal_gpio_set_hl(hsck, hl);
    }
    ads_delay();
    
    return hl;
}
static inline void set_sdi_hl(U8 x)
{
    U8 hl=(x?1:0);
    handle_t hsdi=adsHandle.hpin[PIN_SDI];
    
    dal_gpio_set_hl(hsdi, hl);
    ads_delay();
}
static inline U8 get_sdo_hl(U8 id, U8 delay)
{
    int hl;
    handle_t hsdo=adsHandle.hpin[PIN_IO0+id];
    
    hl = dal_gpio_get_hl(hsdo);
    if(delay) ads_delay();
    
    return (hl>0)?1:0;
}
static inline U8 set_sdo_hl(U8 id, U8 hl)
{
    handle_t hsdo=adsHandle.hpin[PIN_IO0+id];
    
    dal_gpio_set_hl(hsdo, hl);
    
    return (hl>0)?1:0;
}

static inline void set_clk_hl(U8 hl)
{
    handle_t hsck=adsHandle.hpin[PIN_SCK];
    
    dal_gpio_set_hl(hsck, hl);
    ads_delay();
}


static inline void ads_sw_write_byte(U8 mode, U8 data)
{
    S8 i,j;
    U8 hl,val;
    
    hl = set_clk_idle(mode, 1);
#if 0
    if(spiQuad) {
        for(i=1; i>=0; i--) {
            
            if(mode==0 || mode==2) {
                for(j=0; j<4; j++) {
                    val = data&(1<<(i*4+j));
                    set_sdo_hl(j, val);
                }
                ads_delay();
            }
            
            hl = !hl;
            set_clk_hl(hl);
            
            if(mode==1 || mode==3) {
                for(j=0; j<4; j++) {
                    val = data&(1<<(i*4+j));
                    set_sdo_hl(j, val);
                }
                ads_delay();
            }
            
            hl = !hl;
            set_clk_hl(hl);
        }
    }
    else 
#endif
    {
        for(i=7; i>=0; i--) {
            val = data&(1<<i);
            
            if(mode==0 || mode==2) {
                set_sdi_hl(val);
            }
            
            hl = !hl;
            set_clk_hl(hl);
            
            if(mode==1 || mode==3) {
                set_sdi_hl(val);
            }
            
            hl = !hl;
            set_clk_hl(hl);
        }
    }
    
    set_clk_idle(mode, 1);
}
static inline void ads_sw_read_byte(U8 mode, U8 *data)
{
    S8 i,j;
    U8 hl,lv,val=0;
    
    hl = set_clk_idle(mode, 1);
    if(adsHandle.spiQuad) {
        for(i=1; i>=0; i--) {
            
            if(mode==0 || mode==2) {
                for(j=0; j<4; j++) {
                    lv = get_sdo_hl(j,0);
                    
                    if(lv) val |= 1<<(i*4+j);
                }
                ads_delay();
            }
            
            hl = !hl;
            set_clk_hl(hl);
            
            if(mode==1 || mode==3) {
                for(j=0; j<4; j++) {
                    lv = get_sdo_hl(j,0);
                    
                    if(lv) val |= 1<<(i*4+j);
                }
                ads_delay();
            }
            
            hl = !hl;
            set_clk_hl(hl);
        }
    }
    else {
        for(i=7; i>=0; i--) {
            if(mode==0 || mode==2) {
                lv = get_sdo_hl(0,1);
            }
            
            hl = !hl;
            set_clk_hl(hl);
            
            if(mode==1 || mode==3) {
                lv = get_sdo_hl(0,1);
            }
            
            hl = !hl;
            set_clk_hl(hl);
            
            if (lv) val |= (1<<i);
        }
    }
    set_clk_idle(mode, 1);
    
    if(data) *data = val;
}


//0: read, 1: write
static inline void ads_sw_readwrite(U8 rw, U8 mode, U8 *data, int len)
{
    int i;
    
    for(i=0; i<len; i++) {
        if(rw==0) {
            ads_sw_read_byte(mode, &data[i]);
        }
        else {
            ads_sw_write_byte(mode, data[i]);
        }
    }
}


static void ads_read_reg(U8 reg, U8 *data)
{
    U8 tmp[3];
    
    ads_spi_deinit();
    
    //1001_<8-bit address>_0000_0000    RD_REG 
    tmp[0] = 0x09;
    tmp[1] = reg;
    tmp[2] = 0;
    
    ads_set_cs(0);
    ads_sw_readwrite(1, adsHandle.spiMode, tmp, 3);
    ads_set_cs(1);
    
    ads_set_cs(0);
    ads_sw_readwrite(0, adsHandle.spiMode, tmp, 3);
    ads_set_cs(1);
    
    *data = tmp[0];
}
static void ads_write_reg(U8 reg, U8 data)
{
    U8 tmp[3];
    
    ads_spi_deinit();
    
    //1010_<8-bit address>_<8-bit data> WR_REG
    tmp[0] = 0x0a;
    tmp[1] = reg;;
    tmp[2] = data;
    
    ads_set_cs(0);
    ads_sw_readwrite(1, adsHandle.spiMode, tmp, 3);
    ads_set_cs(1);
}


static void ads_sw_read_data(U8 *data, int cnt)
{
    int i;
    
    ads_spi_deinit();
    
    ads_set_cs(0);
    ads_sw_readwrite(0, adsHandle.spiMode, data, cnt);
    ads_set_cs(1);
}

static void ads_hw_read_data(U16 *data, int cnt)
{
    U16 tmp[4]={0};
    
    ads_spi_init();
    
    ads_set_cs(0);
    dal_spi_readwrite(adsHandle.hspi, tmp, data, cnt, 0);
    ads_set_cs(1);
}


/*
To enter PD mode:
   1. Write 069h to address 011h to unlock the PD_CNTL register.
   2. Set the PDWN bit in the PD_CNTL register. The device enters PD mode on the CS rising edge.
   In PD mode, all analog blocks within the device are powered down. All register contents are 
   retained and the interface remains active.
To exit PD mode:
   1. Reset the PDWN bit in the PD_CNTL register.
   2. The RVS pin goes high, indicating that the device has processed the command and has started coming out
   of PD mode. However, the host controller must wait for the tPWRUP time to elapse before initiating a new conversion.
*/
static void ads_powerdown(U8 on)
{
    U8 tmp=0;
    
    if(on) {    //power down
        ads_write_reg(0x11, 0x69);
        ads_write_reg(PWR_CTL, 0x01);
    }
    else {      //power up
        ads_write_reg(PWR_CTL, 0x00);
        dal_delay_ms(1);
    }
    
    ads_read_reg(PWR_CTL, &tmp); 
}


static inline void my_rw_bytes(U8 *wdata, U8 *rdata, int len)
{
    int i;
    
    for(i=0; i<len; i++) {
        /* wait data register emplty */
        while((SPI_STAT(SPI5)&SPI_FLAG_TBE)==0);
        
        /* send byte through the port */
        SPI_DATA(SPI5) = wdata[i];
        
        /* wait to receive a byte */
        while((SPI_STAT(SPI5)&SPI_FLAG_RBNE)==0);
        //while((SPI_STAT(SPI5)&SPI_STAT_TRANS));
        
        rdata[i] = SPI_DATA(SPI5);
    }
}
static inline void my_read_data(U8 *data, int len)
{
    U8 wtmp[3]={0};
    
    ads_spi_init();
    
    ads_set_cs(0);
    my_rw_bytes(wtmp, data, len);
    ads_set_cs(1);
}


static void inline ads_read(S16 *data)
{
    U16 tmp;
    int cnt=(adsHandle.spiWidth==8)?2:1;
    
#ifdef GPIO_SPI
    ads_sw_read_data((U8*)&tmp, 2);
    if(data) *data = SWAP16(tmp);
#else
    ads_hw_read_data(&tmp, cnt);
    if(data) *data = tmp;
#endif
        
}


//////////////////////////////////////////////////////////////////
static void ads_callback(U16 *rx, U32 cnt)
{
    int i,j=0;

    if(rx && adsHandle.cfg.callback) {
        adsHandle.cfg.callback(rx, cnt);
    }
}
static void ads_config(void)
{
    U8 val=0x0c,tmp=0;
    
    //ads_write_reg(DAT_CTL, 0x07); //0x04:0x0000, 0x05:0xffff, 0x06:0x5555  0x07:0x3333
    //ads_write_reg(DAT_CTL, 0x06);
    //ads_read_reg(DAT_CTL, &tmp);

#if (SPI_MODE>0)
    if(SPI_MODE) {
        ads_write_reg(SDI_CTL, SPI_MODE);
        adsHandle.spiMode = SPI_MODE;
        ads_read_reg(SDI_CTL, &tmp);
    }
#endif
    
#ifdef SPI_QUAD
    ads_write_reg(SDO_CTL, val);
    spiQuad = 1;
    
    ads_read_reg(SDO_CTL, &tmp);
#endif
}


int ads9120_init(ads9120_cfg_t *cfg)
{
    int r=0;
    
    if(!cfg) {
        return -1;
    }
    adsHandle.inited = 0;
    adsHandle.cfg = *cfg;
    adsHandle.spiHard=0;
    adsHandle.spiMode=0;
    adsHandle.spiQuad=0;
    adsHandle.spiWidth=16;
    
    ads_gpio_init();
    ads_hw_reset();
    ads_config();
    ads_other_init();
    
    ads_spi_init();
    adsHandle.inited = 1;

    //ads9120_test();
    
    return 0;
}


static int ads_en_flag=0;
int ads9120_enable(U8 on)
{
    if(ads_en_flag==on) {
        return -1;
    }
    
    ads9120_power(on);
    dal_pwm_enable(on);
    ads_en_flag = on;
    
    return 0;
}


int ads9120_is_enabled(void)
{
    return ads_en_flag;
}


int ads9120_read(S16 *data, int cnt)
{
    int i;
    
    for(i=0; i<cnt; i++) {
        ads_read(data+i);
    }
    
    return 0;
}


int ads9120_conv(F32 *f, U16 *u, U32 cnt)
{
    U32 i;
    
    if(!f || !u || !cnt) {
        return -1;
    }
    
    for(i=0; i<cnt; i++) {
        f[i] = ((S16)u[i])*LSB;
    }
    
    return 0;
}



int ads9120_power(U8 on)
{
    handle_t hpwr=adsHandle.hpin[PIN_PWR];
    
    if(adsHandle.inited) {
        dal_gpio_set_hl(hpwr, on);
    }
        
    return 0;
}



int ads9120_test(void)
{
    U8 tmp=0,parity;
    S16 data=0;
    static F32 voltate;
    
    //ads_config();
    while(1) {
    
        tmp = data = 0;
        
#if 1
        ads_write_reg(DAT_CTL, 0x06);
        dal_delay_ms(1);
        
        ads_read_reg(DAT_CTL, &tmp);
        dal_delay_ms(1);
        
        //ads_hw_write_reg(DAT_CTL, 0x07);
        //ads_hw_read_reg(DAT_CTL, &tmp);
        
        //ads_write_reg(DAT_CTL, 0x07);
        //dal_delay_ms(1);
        //ads_read_reg(DAT_CTL, &tmp);
        
        //ds9120_read(&data, &parity);
#else
        
        ads9120_read(&data, 1);
        voltate = LSB*data;
        LOGD("%f\n", voltate);
        
        //ads_read_reg(DAT_CTL, &tmp);
        //ads_read_data(0, &data, &parity);
#endif
        
        tmp = 0;
    }
    
    
}



