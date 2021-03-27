#include "drv/spim.h"
#include "drv/gpio.h"
#include "drv/delay.h"
#include "log.h"
#include "bio/bio.h"
#include "tca6424a.h"
#include "bio/ads129x.h"
#include "cfg.h"


#ifdef ADS129X_ENABLE

#define START_DELAY             200         //ms

#define TIMEOUT                 5   //ms
#define REGS_LENGTH             sizeof(ads129x_regs_t)
    
#define CONV16(d)               ((U32)(((d)[0]<<8) | (d)[1]))
#define CONV24(d)               ((U32)(((d)[0]<<16) | ((d)[1]<<8) | (d)[2]))

#define VREF                    (2.4)  //2.5, 4
#define ADCMAX                  (0xffffff)
//#define LSB                     ((VREF*2)/ADCMAX)
#define LSB                      (VREF/0x7fffff)
#define MVALUEOF(gain)           ((U32)((gain)/(LSB*1000)))       //20971.5

#define ADC2VOL(adc, gain)       (LSB*(adc)/(gain))



U8 timer_started=0;
static U16 datarate=250;
extern ads129x_regs_t adsRegs;
static handle_t ads129x_spi_handle;
static bio_callback_t ads129x_callback={0};
static void data_ready_callback(void *data);
static int ads129x_read_ecg(ecg_data_t *ecg);
static void __set_cs(U8 hl);
static int __spi_write(U8 *data, U16 len);
static int __send_cmd(U8 cmd);
int __rdatac_en(U8 on);

//spi buffer lead map
static U8 ads129x_lead_map[8] = {
    LEAD_RLD,
    LEAD_I,  
    LEAD_II, 
    LEAD_III,
    LEAD_V1, 
    LEAD_V2, 
    LEAD_V3, 
    LEAD_V4, 
};  

static bio_setting_t ads129x_sett={
    DC,
    
    {//wct_set_t
        {1, 1, 1, 0, 0, 0, 0, 0},       //
    },
    
    {//rld_set_t
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       //in
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},       //out
    },
};


U16 DATA_RATE[7]={
    16000, 8000, 4000, 2000, 1000, 500, 250,
};


//1tclk is 0.488us
static void __delay_tclk(int t)
{
    int us=(t>>1)+1;
    delay_us(us);
}
static int __gpio_init(void)
{
    int r=0;
    gpio_callback_t gc;
    
    gpio_deinit(SPI_CS_PIN);
    gpio_deinit(ADS129X_START_PIN);
    gpio_deinit(ADS129X_RESET_PIN);
    gpio_deinit(ADS129X_DRDY_PIN);
    
    r = gpio_init(SPI_CS_PIN, HAL_GPIO_DIRECTION_OUTPUT, HAL_GPIO_DATA_HIGH);
    r |= gpio_init(ADS129X_START_PIN, HAL_GPIO_DIRECTION_OUTPUT, HAL_GPIO_DATA_HIGH);
    r |= gpio_init(ADS129X_RESET_PIN, HAL_GPIO_DIRECTION_OUTPUT, HAL_GPIO_DATA_HIGH);
    r |= gpio_init(ADS129X_DRDY_PIN, HAL_GPIO_DIRECTION_INPUT, HAL_GPIO_DATA_HIGH);
    
    gc.mode = HAL_EINT_EDGE_FALLING;
    gc.callback = data_ready_callback;
    gc.user_data = NULL;
    gpio_int_init(ADS129X_DRDY_PIN, &gc);
    //gpio_pull_up(ADS129X_PIN_DRDY);
    
    return r;
}

//CPOL: 空闲时CLK电平状态,    0:低电平,     1:高电平
//CPHA: 数据采样在第几个边沿  0:第一个边沿, 1:第2个边沿
//ADS129X   CPOL:0   CPHA:1
static int __spi_init(void)
{
    int r;
    spim_cfg_t cfg;
    hal_spi_master_port_t port;
    
    cfg.pin.cs   = HAL_GPIO_MAX;//here we control cs by user
    cfg.pin.clk  = SPI_CLK_PIN;
    cfg.pin.miso = SPI_MISO_PIN;
    cfg.pin.mosi = SPI_MOSI_PIN;
    
    cfg.para.freq = ADS129X_FREQ;
    cfg.para.lsb = HAL_SPI_MASTER_MSB_FIRST;
    cfg.para.cpha = HAL_SPI_MASTER_CLOCK_PHASE1;
    cfg.para.cpol = HAL_SPI_MASTER_CLOCK_POLARITY0;
    cfg.para.sport = HAL_SPI_MASTER_SLAVE_0;      //???
    cfg.para.endian = HAL_SPI_MASTER_LITTLE_ENDIAN;
    cfg.para.cs_polarity = HAL_SPI_MASTER_CHIP_SELECT_LOW;
    cfg.para.delay_tick = HAL_SPI_MASTER_NO_GET_TICK_MODE;
    cfg.para.sample_edge = HAL_SPI_MASTER_SAMPLE_POSITIVE;
    
    cfg.useDMA = 0;
    ads129x_spi_handle = spim_init(&cfg);
    LOGI("ads129x spim_init %s\r\n", (ads129x_spi_handle)?"ok":"failed");
    
    return ads129x_spi_handle?0:-1;
}
static void __set_cs(U8 hl)
{
    if(hl) {
        __delay_tclk(10);
    }
    
    gpio_set_hl(SPI_CS_PIN, (hl>0)?HAL_GPIO_DATA_HIGH:HAL_GPIO_DATA_LOW);
    
}
static void __set_start(U8 hl)
{
    if(hl) {
        gpio_pull_up(ADS129X_START_PIN);
        gpio_set_hl(ADS129X_START_PIN, HAL_GPIO_DATA_HIGH);
    }
    else {
        gpio_pull_down(ADS129X_START_PIN);
        gpio_set_hl(ADS129X_START_PIN, HAL_GPIO_DATA_LOW);
    }
}
static void __hw_reset(void)
{
    gpio_set_hl(ADS129X_RESET_PIN, HAL_GPIO_DATA_LOW);
    delay_ms(5);
    gpio_set_hl(ADS129X_RESET_PIN, HAL_GPIO_DATA_HIGH);
    delay_ms(5);
}

static void __hw_init(void)
{
    __gpio_init();
    __spi_init();
}
//////////////////////////////////////////////////////////////

static int __spi_read(U8 *data, U16 len)
{
	return spim_read(ads129x_spi_handle, data, len);
}
static int __spi_write(U8 *data, U16 len)
{
    return spim_write(ads129x_spi_handle, data, len);
}

static int __send_cmd(U8 cmd)
{
    int r,clks=0;
    
    if(cmd!=CMD_SDATAC) {
        __send_cmd(CMD_SDATAC);
    }
    
    __set_cs(0);
    r = __spi_write(&cmd, 1);
    __set_cs(1);
    
    switch(cmd) {
        case CMD_WAKEUP:
        case CMD_START:
        case CMD_RDATAC:
        case CMD_SDATAC:
        clks = 4;
        break;
        
        case CMD_RESET:
        clks = 18;
        break;
        
        default:
        return -1;
    }
    
    __delay_tclk(clks);
    
    return r;
}

int __rdatac_en(U8 on)
{
    return __send_cmd((on>0)?CMD_RDATAC:CMD_SDATAC);
}
int __cmd_start(U8 on)
{
    return __send_cmd((on>0)?CMD_START:CMD_STOP);
}


static void __rdatac_start(U8 on)
{
    __set_start(on);
    __cmd_start(on);
    
    __rdatac_en(on);
    delay_ms(START_DELAY);
    
}
/////////////////////////////////////////////////////////////////////////
int __read_regs(U8 addr, void *data, U8 n)
{
    int i,r;
    U8 cmd=CMD_RREG|addr;
    
    __rdatac_en(0);
    
    __set_cs(0);
    __spi_write(&cmd, 1);
    
    cmd = n-1;
    __spi_write(&cmd, 1);
    
    r = __spi_read((U8*)data, n);
    __set_cs(1);
    
    return r;
}

int __write_regs(U8 addr, void *data, U8 n)
{
    int i,r;
    U8 cmd=CMD_WREG|addr;
    
    __rdatac_en(0);
    
    __set_cs(0);
    __spi_write(&cmd, 1);
    
    cmd = n-1;
    __spi_write(&cmd, 1);
    
    r = __spi_write((U8*)data, n);
    __set_cs(1);
    
    return r;
}


void print_regs(U8 start)
{
    U8 *ori=(U8*)&adsRegs;
    U8 regs[REGS_LENGTH];
    __read_regs(0, regs, REGS_LENGTH);
    
    for(int i=0; i<REGS_LENGTH; i++) {
        printf("reg[%d]: 0x%02x, 0x%02x\r\n", i, ori[i], regs[i]);
    }
    
    if(start) {
        __rdatac_start(1);
    }
}
static int __load_paras(void)
{
    __write_regs(0, (U8*)&adsRegs, REGS_LENGTH);
    //print_regs(0);
    
    return 0;
}


static loff_data_t g_loff;
//#define ADCTO(x)            ((x&0x800000)?(0x7fffff-(x&0x7fffff)):(0x800000+x))
//#define ADCTO(x)           (ADCMAX-((x&0x800000)?(0x7fffff-(x&0x7fffff)):(0x800000+x)))
#define ADCTO(x)            (((x)&0x800000)?(0xff000000|(x)):(x))
static inline U32 get_adc(U8 *buf, U8 ch)
{
    U32 adc;
    
    if(datarate<32000) {
        adc = CONV24(buf+3+ch*3);
    }
    else {
        adc = CONV16(buf+3+ch*2);
    }
    
    return adc;
}

static U8 get_gain(U8 ch)
{
    U8  g,gainV[8]={6,1,2,3,4,8,12,6};
    #define GAIN(n) adsRegs.channel[n].gain
    
    g = gainV[GAIN(ch)];
    
    return g;
}
static inline F32 get_vol(U8 ch, U32 adc)
{
    F32 vol;
    U8  g;
    
    g = get_gain(ch);
    if(adc&0x800000) {
        vol = -ADC2VOL(ADCMAX-adc, g);
    }
    else {
        vol = ADC2VOL(adc, g);
    }
    
    return vol;
}


typedef struct {
    U8  p[8];
    U8  n[8];
}lead_t;
static lead_t et_map={
    {ET_RLD, ET_LA, ET_LL, ET_LL, ET_V1,  ET_V2,  ET_V3,  ET_V4},
    {ET_RLD, ET_RA, ET_RA, ET_LA, ET_WCT, ET_WCT, ET_WCT, ET_WCT},
};
static void fill_loff(U8 p, U8 n, loff_data_t *loff)
{
    U8 i,idx;
    
    for(i=0; i<8; i++) {
        idx = et_map.p[i]; 
        loff->etrode[idx] = (p&(1<<i))?1:0;
        
        idx = et_map.n[i]; 
        loff->etrode[idx] = (n&(1<<i))?1:0;
    }
}

static int __read_frame(ecg_data_t *ecg, loff_data_t *loff)
{
    U8 lead;
    int r,i,l,flag;
    U8 buf[30];
    U8  loff_p,loff_n;
    U8  g,gainV[8]={6,1,2,3,4,8,12,6};
    U32 adc;
    U8  line[LINE_MAX];
    
    l = (datarate<32000)?27:19;
    
    __set_cs(0);
    r = __spi_read(buf, l);
    __set_cs(1);
    
    if(r!=0 || (buf[0]&0xf0)!=0xc0) {
        LOGE("______ r: %d, 0x%02x, 0x%02x, 0x%02x\r\n", r, buf[0], buf[1], buf[2]);
        return -1;
    }
    
    loff_p = ((buf[0]&0x0f)<<4) | (buf[1]&0xf0);               //need modify
    loff_n = ((buf[1]&0x0f)<<4) | (buf[2]&0xf0);
    
    if(loff) {
        fill_loff(loff_p, loff_n, loff);
    }
    
    //printf("______loff_p: 0x%02x, loff_n: 0x%02x\r\n", loff_p, loff_n);
    for(i=0; i<8; i++) {
        
        adc = get_adc(buf, i);
        //printf("______[%d] _____0x%06x\r\n", i, adc);
        
        lead = ads129x_lead_map[i];
        if(ecg) {
            ecg->adc[lead] = ADCTO(adc);
        }
    }
    //printf("\r\n");
    
    return 0;
}
///////////////////////////////////////////////////////////////////////////////////

static void data_ready_callback(void *user_data)
{
    int r;
    
    gpio_int_en(ADS129X_DRDY_PIN, 0);
    if(ads129x_callback.ecgCallback) {
        ads129x_callback.ecgCallback(ads129x_callback.user_data);
    }
    gpio_int_en(ADS129X_DRDY_PIN, 1);
}


//////////////////////////////////////////////////////////////
int ads129x_probe(void)
{
    int r;
    U8 chipid=0;
    
    __hw_init();
    r = __read_regs(REG_ID, &chipid, 1);
    LOGI("_____r: %d  chipid: 0x%02x________\r\n", r, chipid);
    if(r || (chipid!=CHID_ADS1298 && chipid!=CHID_ADS1298R)) {
        return -1;
    }
    
    return 0;
}

static int dev_reset(void)
{
    //Issue reset pulse, wait for 18 tclk  (1tclk is 0.48us, 18tclk is about 10us)
    __hw_reset();
    
    //send SDATAC command in 
    //SDATAC send in __write_reg, so skip this step
    
    //write some registers
    __load_paras();
    
    //set START=1 and send SDATAC command
    __rdatac_start(1);
    
    return 0;
}


static int ads129x_init(void)
{
    //set PDWN=1 RESET=1
    __hw_init();
    
    //delay tPOR(pow(2,18), is about 126 ms) time to wait for power on reset
    //loop check, make sure VCAP1==1.1V
    delay_ms(200);
    
    dev_reset();
    
    return 0;
}


static int ads129x_get_info(bio_info_t *info)
{
    U8  g,gainV[8]={6,1,2,3,4,8,12,6};
    
    if(!info) {
        return -1;
    }
    info->dev = ADS129X;
    info->mvValue = MVALUEOF(6);        //这里统一设置为固定的6dB增益，可能需要修改
    
    return 0;
}


static int ads129x_get_setting(bio_setting_t *sett)
{
    if(!sett) {
        return -1;
    }
    *sett = ads129x_sett;
    
    return 0;
}



static int ads129x_set_current(U8 current)
{
    if(current!=DC || current!=AC) {
        return -1;
    }
    ads129x_sett.current = current;
    
    return 0;
}
      

static int ads129x_read_ecg(ecg_data_t *ecg)
{
    int r;
    
    if(!ecg) {
        return -1;
    }
    
    r = __read_frame(ecg, &g_loff);
    
    return r;
}


static int ads129x_read_loff(loff_data_t *loff)
{
    if(!loff) {
        return -1;
    }
    
    return 0;
}


static int ads129x_get_rld(rld_set_t *rld)
{
    if(!rld) {
        return -1;
    }
    *rld = ads129x_sett.rld;
    
    return 0;
}


static int ads129x_set_rld(rld_set_t *rld)
{
    if(!rld) {
        return -1;
    }
    ads129x_sett.rld = *rld;
    
    return 0;
}


static int ads129x_get_wct(wct_set_t *wct)
{
    if(!wct) {
        return -1;
    }
    *wct = ads129x_sett.wct;
    
    return 0;
}


static int ads129x_set_wct(wct_set_t *wct)
{
    if(!wct) {
        return -1;
    }
    ads129x_sett.wct = *wct;
    
    return 0;
}


static int set_dr(U16 dr)
{
    U8 n,tmp;
    
    if(dr<250 || dr>32000 || (dr%250!=0)) {
        return -1;
    }
    
    for(n=0; n<7; n++) {
        if(DATA_RATE[n]==dr) {
            break;
        }
    }
    datarate = dr;
    
    __read_regs(REG_CONFIG1, &tmp, 1);
    tmp &= 0xf8;
    tmp |= n;
    __write_regs(REG_CONFIG1, &tmp, 1);
    
    return __rdatac_en(1);
}



int ads129x_reset(void)
{
    //__send_cmd(CMD_RESET);
    dev_reset();
    
    return 0;
}



static int ads129x_start(void)
{
    gpio_int_en(ADS129X_DRDY_PIN, 1);
    return  0;
}


static int ads129x_stop(void)
{
    gpio_int_en(ADS129X_DRDY_PIN, 0);
    return  0;
}


static int ads129x_set_callback(bio_callback_t *cb)
{
    if(!cb) {
        return -1;
    }
    
    ads129x_callback = *cb;
    return 0;//set_dr(cb->freq);
}


static int ads129x_get_stat(bio_stat_t *stat)
{
    if(!stat) {
        return -1;
    }
    
    return 0;
}


/////////////electrode switch functions start//////////////
enum {
    FUN_ECG=0,
    FUN_SHORT,
    FUN_MEAS,
    FUN_MVDD,
    FUN_TEMP,
    FUN_TEST_SIG,
    FUN_RLD_DRVP,
    FUN_RLD_DRVN,
}FUN;
static void select_ch_fun(U8 ch, U8 fun)
{
    U8 reg;
    channel_t tmp;
    
    reg = REG_CH1SET+ch;
    __read_regs(reg, &tmp, 1);
    tmp.mux = fun;
    __write_regs(reg, &tmp, 1);
}
static void select_ch_gain(U8 ch, U8 gain)
{
    U8 reg;
    channel_t tmp;
    
    reg = REG_CH1SET+ch;
    __read_regs(reg, &tmp, 1);
    tmp.gain = gain;
    __write_regs(reg, &tmp, 1);
}

static void select_rld_sens(U8 l, U8 on)
{
    U8 i,tmp,reg,ch=l/8;
    config3_t cfg3;
    
    __read_regs(reg, &cfg3, 1);
    cfg3.rld_loff_sens = on?1:0;
    
    reg = (l%2==0)?REG_RLD_SENSP:REG_RLD_SENSN;
    __read_regs(reg, &tmp, 1);
    for(i=0; i<8; i++) {
        if(ch&(1<<i)) {
            if(on>0) {
                tmp |= (1<<i);
            }
            else {
                tmp &= ~(1<<i);
            }
        }
    }
    __write_regs(reg, &tmp, 1);
}

static void select_rld_drv(U8 l, U8 on)
{
    U8 ch,fun,pn;
    
    ch = l/8;
    pn = (l%2==0)?FUN_RLD_DRVP:FUN_RLD_DRVN;
    fun = on?FUN_ECG:pn;
    select_ch_fun(ch, fun);
}

static void rld_power(U8 on)
{
    config3_t tmp;
    
    __read_regs(REG_CONFIG3, &tmp, 1);
    tmp.pd_rld = (on>0)?1:0;
    __write_regs(REG_CONFIG3, &tmp, 1);
}
static void messure_rld(U8 line_out, U8 line_in, U8 on)
{
    U8 ch,fun;
    config3_t tmp;
    
    //确保rldout与rldin在外部相连
    
    __read_regs(REG_CONFIG3, &tmp, 1);
    tmp.pd_rld = 1;
    tmp.rld_meas = (on>0)?1:0;
    tmp.rldref_int = 1;
    __write_regs(REG_CONFIG3, &tmp, 1);
    
    ch = line_out/8;
    fun = on?FUN_ECG:FUN_MEAS;
    select_ch_fun(ch, fun);
    
    ch = line_in/8;
    select_ch_fun(ch, fun);
}

static void set_loff_sens(U8 line, U8 on)
{
    U8 i,tmp;

    
    //__write_reg(REG_LOFF_SENSP, &tmp);
    //__write_reg(REG_LOFF_SENSN, &tmp);
}

static void select_loff_current(U8 fun)
{
    U8 i,tmp;
    loff_t loff;

    tmp = 0xff;
    __write_regs(REG_LOFF_SENSP, &tmp, 1);
    __write_regs(REG_LOFF_SENSN, &tmp, 1);
    
    __read_regs(REG_LOFF, &loff, 1);
    if(fun==LOFF_CURRENT_DC) {
        loff.flead_off = 3;
    }
    else if(fun==LOFF_CURRENT_AC){
        loff.flead_off = 1;
    }
    else {
        loff.flead_off = 2;
    }
    __write_regs(REG_LOFF, &loff, 1);
}

static U8 rld_loff_stat(void)
{
    config3_t tmp;
    
    __read_regs(REG_CONFIG3, &tmp, 1);
    return tmp.rld_stat;
}
static int rld_is_loff(void)
{
    U8 loff;
    config3_t tmp;
    
    //rld_buf_en(0);
    loff = rld_loff_stat();
    //rld_buf_en(1);
    __rdatac_en(1);
    
    return loff;
}

typedef struct {
    U8  th;             //0~7   95%  92.5%  90%  87.5%  85%  80%  75% 70%
    U8  mode;           //0:    current mode   1: pull up/down mode
    U8  current;        //0~3   6nA ~24nA
}loff_para_t;
static void set_loff_para(loff_para_t *p)
{
    loff_t tmp;
    
    __read_regs(REG_LOFF, &tmp, 1);
    tmp.comp_th = p->th;
    tmp.vlead_off_en = p->mode;
    tmp.ilead_off = p->current;
    __write_regs(REG_LOFF, &tmp, 1);
}

static int ads129x_send_cmd(bio_cmd_t *cmd)
{
    if(!cmd) {
        return -1;
    }
    
    switch(cmd->id) {
        
        case CMD_DEV_RESET:
        ads129x_reset();
        break;
        
        case CMD_REG_PRINT:
        print_regs(1);
        break;
        
        default:
        return -1;
    }
    
    return 0;
}
/////////////electrode switch functions end//////////////

bio_handle_t ads129x_handle={
    ads129x_init,
    ads129x_reset,
    ads129x_get_stat,
    ads129x_send_cmd,
    ads129x_start,
    ads129x_stop,
    ads129x_get_info,
    ads129x_set_current,
    ads129x_get_setting,
    ads129x_set_callback,

    ads129x_read_ecg,
    ads129x_read_loff,
    ads129x_get_rld,
    ads129x_set_rld,
    ads129x_get_wct,
    ads129x_set_wct,
};
#endif

