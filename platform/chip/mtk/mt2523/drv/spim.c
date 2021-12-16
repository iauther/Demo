#include "drv/spim.h"
#include "log.h"
#include "drv/gpio.h"
#include "drv/delay.h"


typedef struct {
    hal_spi_master_port_t           port;
    U8                              useDMA;
    U8                              rflag;
    U8                              wflag;
}spim_handle_t;


#define SPIM_PIN_GRP        2
typedef struct {
    hal_gpio_pin_t      pin;
    U8                  fun;
}pinmux_t;
typedef struct {
    pinmux_t                cs[SPIM_PIN_GRP];
    pinmux_t                clk[SPIM_PIN_GRP];
    pinmux_t                miso[SPIM_PIN_GRP];
    pinmux_t                mosi[SPIM_PIN_GRP];
}spim_pinmuxs_t;

typedef struct {
    pinmux_t                cs;
    pinmux_t                clk;
    pinmux_t                miso; 
    pinmux_t                mosi;
}spim_pinmux_t;

spim_pinmuxs_t spim_pinmuxs[HAL_SPI_MASTER_MAX]={
    {
        {
            {HAL_GPIO_0, HAL_GPIO_0_MA_SPI0_A_CS},
            {HAL_GPIO_4, HAL_GPIO_4_MA_SPI0_B_CS},
        },
        
        {
            {HAL_GPIO_1, HAL_GPIO_1_MA_SPI0_A_SCK},
            {HAL_GPIO_5, HAL_GPIO_5_MA_SPI0_B_SCK},
        },
        
        {
            {HAL_GPIO_3, HAL_GPIO_3_MA_SPI0_A_MISO},
            {HAL_GPIO_7, HAL_GPIO_7_MA_SPI0_B_MISO},
        },
        
        {
            {HAL_GPIO_2, HAL_GPIO_2_MA_SPI0_A_MOSI},
            {HAL_GPIO_6, HAL_GPIO_6_MA_SPI0_B_MOSI},
        },
    },
    
    {
        {
            {HAL_GPIO_0, HAL_GPIO_0_MA_SPI1_A_CS},
            {HAL_GPIO_11, HAL_GPIO_11_MA_SPI1_B_CS},
        },
           
        {
            {HAL_GPIO_1, HAL_GPIO_1_MA_SPI1_A_SCK},
            {HAL_GPIO_12, HAL_GPIO_12_MA_SPI1_B_SCK},
        },
        
        {
            {HAL_GPIO_3, HAL_GPIO_3_MA_SPI1_A_MISO},
            {HAL_GPIO_14, HAL_GPIO_14_MA_SPI1_B_MISO},
        },
        
        {
            {HAL_GPIO_2, HAL_GPIO_2_MA_SPI1_A_MOSI},
            {HAL_GPIO_13, HAL_GPIO_13_MA_SPI1_B_MOSI},
        },
    },
    
    {
        {
            {HAL_GPIO_25, HAL_GPIO_25_MA_SPI2_A_CS},
            {HAL_GPIO_39, HAL_GPIO_39_MA_SPI2_B_CS},
        },
               
        {
            {HAL_GPIO_26, HAL_GPIO_26_MA_SPI2_A_SCK},
            {HAL_GPIO_40, HAL_GPIO_40_MA_SPI2_B_SCK},
        },
        
        {
            {HAL_GPIO_28, HAL_GPIO_28_MA_SPI2_A_MISO},
            {HAL_GPIO_42, HAL_GPIO_42_MA_SPI2_B_MISO},
        },
        
        {
            {HAL_GPIO_27, HAL_GPIO_27_MA_SPI2_A_MOSI},
            {HAL_GPIO_41, HAL_GPIO_41_MA_SPI2_B_MOSI},
        },
    },
    
    {
        {
            {HAL_GPIO_25, HAL_GPIO_25_MA_SPI3_A_CS},
            {HAL_GPIO_39, HAL_GPIO_32_MA_SPI3_B_CS},
        },
       
        {
            {HAL_GPIO_26, HAL_GPIO_26_MA_SPI3_A_SCK},
            {HAL_GPIO_40, HAL_GPIO_33_MA_SPI3_B_SCK},
        },
        
        {
            {HAL_GPIO_28, HAL_GPIO_28_MA_SPI3_A_MISO},
            {HAL_GPIO_42, HAL_GPIO_35_MA_SPI3_B_MISO},
        },
        
        {
            {HAL_GPIO_27, HAL_GPIO_27_MA_SPI3_A_MOSI},
            {HAL_GPIO_41, HAL_GPIO_34_MA_SPI3_B_MOSI},
        },
    },
};



static int get_pin_info(spim_pin_t *pin, spim_pinmux_t *mux, spim_info_t *info)
{
    int find=0,grp=-1;
    hal_spi_master_port_t i;
    hal_spi_master_macro_select_t j;
    
    for(i=HAL_SPI_MASTER_0; i<HAL_SPI_MASTER_MAX; i++) {
        find = 0;
        for(j=HAL_SPI_MASTER_MACRO_GROUP_A; j<SPIM_PIN_GRP; j++) {
            if(spim_pinmuxs[i].cs[j].pin==pin->cs) {
                mux->cs = spim_pinmuxs[i].cs[j];
            }
        }
        
        for(j=HAL_SPI_MASTER_MACRO_GROUP_A; j<SPIM_PIN_GRP; j++) {
            
            if(spim_pinmuxs[i].clk[j].pin==pin->clk) {
                mux->clk = spim_pinmuxs[i].clk[j];
                find++;
            }
        }
        
        for(j=HAL_SPI_MASTER_MACRO_GROUP_A; j<SPIM_PIN_GRP; j++) {
            if(spim_pinmuxs[i].mosi[j].pin==pin->mosi) {
                mux->mosi = spim_pinmuxs[i].mosi[j];
                find++;
                if(grp!=-1 && grp!=j) {
                    LOGE("spi group conflict\r\n");
                    return -1;
                }
                grp = j;
            }
        }
            
        for(j=HAL_SPI_MASTER_MACRO_GROUP_A; j<SPIM_PIN_GRP; j++) {
            if(spim_pinmuxs[i].miso[j].pin==pin->miso) {
                mux->miso = spim_pinmuxs[i].miso[j];
                find++;
                
                if(grp!=-1 && grp!=j) {
                    LOGE("spi group conflict!!\r\n");
                    return -1;
                }
                grp = j;
            }
        }
            
        if(find>1) {
            if(info) {
                info->port = i;
                info->group = (hal_spi_master_macro_select_t)grp;
            }
            LOGD("spi port: %d, group: %d\r\n", i, grp);
            return 0;
        }
    }
    LOGD("_____spim not found\r\n");
    
    return -1;
}
static int set_pinmux(spim_pin_t *pins, spim_info_t *info)
{
    int r;
    spim_info_t inf;
    spim_pinmux_t mux;
    hal_pinmux_status_t st;
    
    r = get_pin_info(pins, &mux, &inf);
    if(r) {
        LOGE("get_pin_info failed\r\n");
        return -1;
    }
    
    hal_gpio_init(mux.cs.pin);
    st = hal_pinmux_set_function(mux.cs.pin, mux.cs.fun);
    LOGD("set cs pinmux %d, pin: %d, fun: %d\r\n", st, mux.cs.pin, mux.cs.fun);
    
    hal_gpio_init(mux.clk.pin);
    st = hal_pinmux_set_function(mux.clk.pin, mux.clk.fun);
    LOGD("set clk pinmux %d, pin: %d, fun: %d\r\n", st, mux.clk.pin, mux.clk.fun);
    
    hal_gpio_init(mux.miso.pin);
    st = hal_pinmux_set_function(mux.miso.pin, mux.miso.fun);
    LOGD("set miso pinmux %d, pin: %d, fun: %d\r\n", st, mux.miso.pin, mux.miso.fun);
    
    hal_gpio_init(mux.mosi.pin);
    st = hal_pinmux_set_function(mux.mosi.pin, mux.mosi.fun);
    LOGD("set mosi pinmux %d, pin: %d, fun: %d\r\n", st, mux.mosi.pin, mux.mosi.fun);
    
    if(info) *info = inf;
    
    return 0;
}



static void spim_callback(hal_spi_master_callback_event_t event, void *user_data)
{
    spim_handle_t *h=(spim_handle_t*)user_data;
    
    if(h && event==HAL_SPI_MASTER_EVENT_SEND_FINISHED)     h->wflag=1;
    if(h && event==HAL_SPI_MASTER_EVENT_RECEIVE_FINISHED)  h->rflag=1;
}

handle_t spim_init(spim_cfg_t *cfg)
{
    int r;
    spim_info_t inf;
    spim_handle_t *h;
    hal_spi_master_status_t st;
    hal_spi_master_config_t spi_config;
    hal_spi_master_advanced_config_t advanced_config;
    hal_spi_master_send_and_receive_config_t send_and_receive_config;
    
    h = (spim_handle_t*)calloc(1, sizeof(spim_handle_t));
    if(!h) {
        LOGE("spim_handle_t malloc failed\r\n");
        return NULL;
    }
    
    r = set_pinmux(&cfg->pin, &inf);
    if(r!=0) {
        LOGE("set_pinmux failed\r\n");
        free(h);
        return NULL;
    }
    
    //hal_spi_master_set_macro_selection(inf.port, inf.group);
    
    /* Step2: Initializes SPI master with normal mode */
    spi_config.bit_order       = cfg->para.lsb;
    spi_config.slave_port      = cfg->para.sport;
    spi_config.clock_frequency = cfg->para.freq;
    spi_config.phase           = cfg->para.cpha;
    spi_config.polarity        = cfg->para.cpol;
    
    LOGI("spim port: %d, lsb: %d, freq: %d, cpha: %d, cpol: %d\r\n", inf.port, spi_config.bit_order, spi_config.clock_frequency, spi_config.phase, spi_config.polarity);
    
    hal_spi_master_deinit(inf.port);
    st = hal_spi_master_init(inf.port, &spi_config);
    if (HAL_SPI_MASTER_STATUS_OK != st) {
        LOGE("hal_spi_master_init failed %d\r\n", st);
        goto failed;
    }
    h->port = inf.port;
    h->useDMA = cfg->useDMA;
    
    hal_spi_master_register_callback(inf.port, spim_callback, h);
    
#if 0
    advanced_config.byte_order    = cfg->para.endian;
    advanced_config.chip_polarity = cfg->para.cs_polarity;
    advanced_config.get_tick      = cfg->para.delay_tick;
    advanced_config.sample_select = cfg->para.sample_edge;
    st = hal_spi_master_set_advanced_config(inf.port, &advanced_config);
    if (HAL_SPI_MASTER_STATUS_OK != st) {
        LOGE("hal_spi_master_set_advanced_config failed %d\r\n", st);
        goto failed;
    }
#endif
    
    return h;
    
failed:
    hal_spi_master_deinit(inf.port);
    free(h);
    
    return NULL;
}


int spim_deinit(handle_t *h)
{
    hal_spi_master_status_t st;
    spim_handle_t *sh;
    
    if(!h) {
        return -1;
    }
    
    sh = *((spim_handle_t**)h);
    if(!sh) {
        return -1;
    }
    
    st = hal_spi_master_deinit(sh->port);
    free(sh);
    
    return (st==HAL_SPI_MASTER_STATUS_OK)?0:-1;
}


int spim_get_port(spim_pin_t *pin, hal_spi_master_port_t *port)
{
    int r;
    spim_info_t inf;
    spim_pinmux_t mux;
    hal_pinmux_status_t st;
    
    r = get_pin_info(pin, &mux, &inf);
    if(r==0 && port) {
        *port = inf.port;
    }
    
    return r;
}



#define SPI_MAXLEN  HAL_SPI_MAXIMUM_POLLING_TRANSACTION_SIZE
typedef hal_spi_master_status_t (*spim_fn_t)(hal_spi_master_port_t master_port,
        hal_spi_master_send_and_receive_config_t *spi_send_and_receive_config);


static int dma_delay(spim_handle_t *h, U8 rw)
{
    U8 *flag=(rw==0)?&h->rflag:&h->wflag;
    
    while((*flag)==0) os_delay_ms(1);
    
    return 0;
}


int spim_read(handle_t h, U8 *data, U32 len)
{
    int r,i,m,n;
    spim_fn_t spim_fn;
    U8 tmp[SPI_MAXLEN]={0};
    spim_handle_t *sh=(spim_handle_t*)h;
    hal_spi_master_send_and_receive_config_t cfg;
    
    if(!h || !data || !len) {
        return -1;
    }
    
    spim_fn = (sh->useDMA)?hal_spi_master_send_and_receive_dma:hal_spi_master_send_and_receive_polling;
    
    m = len/SPI_MAXLEN;   n = len%SPI_MAXLEN;
    for(i=0; i<m; i++) {
        cfg.send_data = tmp+i*SPI_MAXLEN;
        cfg.send_length = SPI_MAXLEN;
        cfg.receive_buffer = data+i*SPI_MAXLEN;
        cfg.receive_length = SPI_MAXLEN;
        r = spim_fn(sh->port, &cfg);
        if(sh->useDMA) dma_delay(sh, 0);
    }
    
    if(n>0) {
        cfg.send_data = tmp+len-n;
        cfg.send_length = n;
        cfg.receive_buffer = data+len-n;
        cfg.receive_length = n;
        r = spim_fn(sh->port, &cfg);
        if(sh->useDMA) dma_delay(sh, 0);
    }
    
    return r;
}


int spim_write(handle_t h, U8 *data, U32 len)
{
    int r,i,m,n;
    spim_fn_t spim_fn;
    U8 tmp[SPI_MAXLEN]={0};
    spim_handle_t *sh=(spim_handle_t*)h;
    hal_spi_master_send_and_receive_config_t cfg;
    
    if(!h || !data || !len) {
        return -1;
    }
    
    spim_fn = (sh->useDMA)?hal_spi_master_send_and_receive_dma:hal_spi_master_send_and_receive_polling;
    
    m = len/SPI_MAXLEN;   n = len%SPI_MAXLEN;
    for(i=0; i<m; i++) {
        cfg.send_data = data+i*SPI_MAXLEN;
        cfg.send_length = SPI_MAXLEN;
        cfg.receive_buffer = tmp+i*SPI_MAXLEN;
        cfg.receive_length = SPI_MAXLEN;
        r = spim_fn(sh->port, &cfg);
        if(sh->useDMA) dma_delay(sh, 1);
    }
    
    if(n>0) {
        cfg.send_data = data+len-n;
        cfg.send_length = n;
        cfg.receive_buffer = tmp+len-n;
        cfg.receive_length = n;
        r = spim_fn(sh->port, &cfg);
        if(sh->useDMA) dma_delay(sh, 1);
    }
    
    return r;
}



