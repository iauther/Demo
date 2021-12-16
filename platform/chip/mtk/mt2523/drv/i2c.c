#include "drv/i2c.h"
#include "drv/si2c.h"
#include "drv/delay.h"
#include "log.h"


typedef struct {
    hal_i2c_port_t      port;
    U8                  useDMA;
    i2c_callback        callback;
    U8                  finish_flag;
}i2c_handle_t;



#define I2C_PIN_GRP     3

typedef struct {
    pin_t       pin;
    U8          fun;
}pinmux_t;

typedef struct {
    pinmux_t    scl;
    pinmux_t    sda;
}i2c_pinmux_t;

typedef struct {
    pinmux_t    scl[I2C_PIN_GRP];
    pinmux_t    sda[I2C_PIN_GRP];
}i2cm_pin_t;


i2cm_pin_t i2c_pin[HAL_I2C_MASTER_MAX]={
    {
        {
            {{0, HAL_GPIO_30 }, HAL_GPIO_30_SCL0},
            {{0, HAL_GPIO_36 }, HAL_GPIO_36_SCL0},
            {{0, HAL_GPIO_MAX}, 0},
        },
        
        {
            {{0, HAL_GPIO_31 }, HAL_GPIO_31_SDA0},
            {{0, HAL_GPIO_37 }, HAL_GPIO_37_SDA0},
            {{0, HAL_GPIO_MAX}, 0},
        },
        
    },
    
    {
        {
            {{0, HAL_GPIO_36}, HAL_GPIO_36_SCL1},
            {{0, HAL_GPIO_38}, HAL_GPIO_38_SCL1},
            {{0, HAL_GPIO_39}, HAL_GPIO_39_SCL1},
        },
        
        {
            {{0, HAL_GPIO_37}, HAL_GPIO_37_SDA1},
            {{0, HAL_GPIO_41}, HAL_GPIO_41_SDA1},
            {{0, HAL_GPIO_43}, HAL_GPIO_43_SDA1},
        },
    },
    
    {
        {
            {{0, HAL_GPIO_8 }, HAL_GPIO_8_SCL2},
            {{0, HAL_GPIO_19}, HAL_GPIO_19_SCL2},
            {{0, HAL_GPIO_30}, HAL_GPIO_30_SCL2},
        },
        
        {
            {{0, HAL_GPIO_9 }, HAL_GPIO_9_SDA2},
            {{0, HAL_GPIO_20}, HAL_GPIO_20_SDA2},
            {{0, HAL_GPIO_31}, HAL_GPIO_31_SDA2},
        },
    },
};


static int get_pinmux(i2c_pin_t *pin, i2c_pinmux_t *mux, hal_i2c_port_t *port)
{
    hal_i2c_port_t i;
    int j,find;
    U8 scl_fun,sda_fun;
    hal_i2c_port_t p1,p2;
    
    for(i=HAL_I2C_MASTER_0; i<HAL_I2C_MASTER_MAX; i++) {
        find = 0;
        for(j=0; j<I2C_PIN_GRP; j++) {
            if(i2c_pin[i].scl[j].pin.pin==HAL_GPIO_MAX){
                break;
            }
            
            if(i2c_pin[i].scl[j].pin.pin==pin->scl.pin) {
                p1 = i;
                scl_fun = i2c_pin[i].scl[j].fun;
                find |= 1;
                break;
            }
        }
        
        for(j=0; j<I2C_PIN_GRP; j++) {
            if(i2c_pin[i].sda[j].pin.pin==HAL_GPIO_MAX){
                break;
            }
            
            if(i2c_pin[i].sda[j].pin.pin==pin->sda.pin) {
                p2 = i;
                sda_fun = i2c_pin[i].sda[j].fun;
                find |= 2;
                break;
            }
        }
        
        if(find==3) {
            break;
        }
    }
    
    if(find==3 && p1==p2) {
        if(mux) {
            mux->scl.pin = pin->scl;
            mux->scl.fun = scl_fun;
            
            mux->sda.pin = pin->sda;
            mux->sda.fun = sda_fun;
            
            //LOGI("___ scl.pin: %d, scl.fun: %d\r\n", mux->scl.pin.pin, mux->scl.fun);
            //LOGI("___ sda.pin: %d, sda.fun: %d\r\n", mux->sda.pin.pin, mux->sda.fun);
        }
        if(port) *port = p1;
        
        return 0;
    }
    
    return -1;
}

static int set_pinmux(i2c_pinmux_t *mux)
{
    hal_gpio_init((hal_gpio_pin_t)mux->scl.pin.pin);
    hal_gpio_init((hal_gpio_pin_t)mux->sda.pin.pin);
    
    hal_pinmux_set_function((hal_gpio_pin_t)mux->scl.pin.pin, mux->scl.fun);
    hal_pinmux_set_function((hal_gpio_pin_t)mux->sda.pin.pin, mux->sda.fun);
    
    return 0;
}

static void i2c_master_callback(uint8_t slave_address, hal_i2c_callback_event_t event, void *user_data)
{
    i2c_handle_t *h=(i2c_handle_t*)user_data;
    
    if(h && event==HAL_I2C_EVENT_SUCCESS) {
        h->finish_flag = 1;
        if(h->callback) h->callback(NULL);
    }
}
////////////////////////////////////////////////////////////////////////////


handle_t i2c_init(i2c_cfg_t *cfg)
{
    int r;
    hal_i2c_port_t port;
    i2c_pinmux_t mux;
    hal_i2c_status_t st;
    hal_i2c_config_t cf;
    i2c_handle_t *h=(i2c_handle_t*)calloc(1, sizeof(i2c_handle_t));
 
    if(!h) {
        return NULL;
    }
    
    r = get_pinmux(&cfg->pin, &mux, &port);
    if(r<0) {
        LOGE("get_pinmux failed\r\n");
        free(h);
        return NULL;
    }
    
    r = set_pinmux(&mux);
    if(r<0) {
        LOGE("set_pinmux failed\r\n");
        goto failed;
    }
    
    cf.frequency = (hal_i2c_frequency_t)cfg->freq;
    
    hal_i2c_master_deinit(port);
    st = hal_i2c_master_init(port, &cf);
    if(st!=HAL_I2C_STATUS_OK) {
        LOGE("hal_i2c_master_init failed, %d\r\n", st);
        goto failed;
    }
    LOGI("hal_i2c_master_init ok!\r\n");
    
    hal_i2c_master_register_callback(port, i2c_master_callback, h);
    h->port = port;
    h->useDMA = cfg->useDMA;
    h->callback = cfg->callback;
    
    return h;
    
failed:
    hal_i2c_master_deinit(port);
    free(h);
    return NULL;
}


int i2c_deinit(handle_t *h)
{
    i2c_handle_t *ih;
    
    if(!h) {
        return -1;
    }
    
    ih = *((i2c_handle_t**)h);
    if(!ih) {
        return -1;
    }
    
    hal_i2c_master_deinit(ih->port);
    free(ih);
    return 0;
}



#define MCU_PKT_BYTE_MAX        8
#define DMA_PKT_CNT_MAX         0xff
#define DMA_PKT_BYTE_MAX        0x0f
#define DMA_PKT_LEN_MAX (DMA_PKT_CNT_MAX*DMA_PKT_BYTE_MAX)
static hal_i2c_status_t i2c_dma_read(i2c_handle_t *h, uint8_t addr, uint8_t *data, uint32_t len)
{
    int i,m,n,remain=0;
    hal_i2c_status_t st;
    hal_i2c_receive_config_t rc;
    int packet_len,bytes_in_one_packet;
    
    if(len>DMA_PKT_LEN_MAX) {
        return HAL_I2C_STATUS_INVALID_PORT_NUMBER;
    }
    
    if(len<=DMA_PKT_BYTE_MAX) {
        rc.receive_packet_length = 1;
        rc.receive_bytes_in_one_packet = len;
    }
    else {
        rc.receive_packet_length = len/DMA_PKT_BYTE_MAX;
        rc.receive_bytes_in_one_packet = DMA_PKT_BYTE_MAX;
        remain = len%DMA_PKT_BYTE_MAX;
    }
    
    rc.slave_address = addr;
    rc.receive_buffer = data;
    
    h->finish_flag = 0;
    st = hal_i2c_master_receive_dma_ex(h->port, &rc);
    while(h->finish_flag==0) os_delay_ms(1);
    
    if(remain>0) {
        st |= i2c_dma_read(h, addr, data+len-remain, remain);
    }
    
    return st;
    
}
static hal_i2c_status_t i2c_dma_write(i2c_handle_t *h, uint8_t addr, const uint8_t *data, uint32_t len)
{
    int i,m,n,remain=0;
    hal_i2c_status_t st;
    hal_i2c_send_config_t sc;
    
    if(len>DMA_PKT_LEN_MAX) {
        return HAL_I2C_STATUS_INVALID_PORT_NUMBER;
    }
    
    if(len<=DMA_PKT_BYTE_MAX) {
        sc.send_packet_length = 1;
        sc.send_bytes_in_one_packet = len;
    }
    else {
        sc.send_packet_length = len/DMA_PKT_BYTE_MAX;
        sc.send_bytes_in_one_packet = DMA_PKT_BYTE_MAX;
        remain = len%DMA_PKT_BYTE_MAX;
    }
    
    sc.slave_address = addr;
    sc.send_data = data;
    
    h->finish_flag = 0;
    st = hal_i2c_master_send_dma_ex(h->port, &sc);
    while(h->finish_flag==0) os_delay_ms(1);
    
    if(remain>0) {
        st |= i2c_dma_write(h, addr, data+len-remain, remain);
    }
    
    return st;
}
static hal_i2c_status_t i2c_mcu_read(i2c_handle_t *h, uint8_t addr, uint8_t *data, uint32_t len)
{
    return hal_i2c_master_receive_polling(h->port, addr, data, len);
}
static hal_i2c_status_t i2c_mcu_write(i2c_handle_t *h, uint8_t addr, const uint8_t *data, uint32_t len)
{
    return hal_i2c_master_send_polling(h->port, addr, data, len);
}


typedef hal_i2c_status_t (*i2c_read_ptr)(i2c_handle_t *h, uint8_t slave_address, uint8_t *buffer, uint32_t size);
typedef hal_i2c_status_t (*i2c_write_ptr)(i2c_handle_t *h, uint8_t slave_address, const uint8_t *buffer, uint32_t size);

int i2c_read(handle_t h, U16 addr, U8 *data, U32 len, U8 bStop)
{
    int r,i,m,n;
    i2c_handle_t *ih=(i2c_handle_t*)h;
    hal_i2c_status_t st=HAL_I2C_STATUS_OK;
    
    if(!ih || !data || !len) {
        return -1;
    }
    
    int lenMax=(ih->useDMA>0)?DMA_PKT_LEN_MAX:MCU_PKT_BYTE_MAX;
    i2c_read_ptr i2c_r=(ih->useDMA>0)?i2c_dma_read:i2c_mcu_read;
    
    m=len/lenMax; n=len%lenMax;
    for(i=0; i<m; i++) {
        st |= i2c_r(ih, (U8)addr, data+i*lenMax, lenMax);
    }
    if(n>0) {
        st |= i2c_r(ih, (U8)addr, data+m*lenMax, n);
    }

    return (int)st;//(st==HAL_I2C_STATUS_OK)?0:-1;
}

int i2c_write(handle_t h, U16 addr, U8 *data, U32 len, U8 bStop)
{
    int r,i,m,n;
    i2c_handle_t *ih=(i2c_handle_t*)h;
    hal_i2c_status_t st=HAL_I2C_STATUS_OK;
    
    if(!ih || !data || !len) {
        return -1;
    }
    
    int lenMax=(ih->useDMA>0)?DMA_PKT_LEN_MAX:MCU_PKT_BYTE_MAX;
    i2c_write_ptr i2c_w=(ih->useDMA>0)?i2c_dma_write:i2c_mcu_write;
    
    m=len/lenMax; n=len%lenMax;
    for(i=0; i<m; i++) {
        st |= i2c_w(ih, (U8)addr, data+i*lenMax, lenMax);
    }
    if(n>0) {
        st |= i2c_w(ih, (U8)addr, data+m*lenMax, n);
    }

    return (int)st;//(st==HAL_I2C_STATUS_OK)?0:-1;
}



int i2c_set_callback(handle_t h, i2c_callback callback)
{
    i2c_handle_t *ih=(i2c_handle_t*)h;
    
    if(!ih) {
        return -1;
    }
    
    ih->callback = callback;
    return 0;
}

#define SWAP16(x)   (((U16)x)<<8)|(((U16)x)>>8)
int i2c_mem_read(handle_t h, U16 devAddr, U16 memAddr, U16 memAddrSize, U8 *data, U32 len)
{
    int r; 
    U16 addr=SWAP16(memAddr);
    hal_i2c_send_to_receive_config_t cfg;
    i2c_handle_t *ih=(i2c_handle_t*)h;
    
    if(!h) {
        return -1;
    }
    
    cfg.receive_buffer = data;
    cfg.receive_length = len;
    cfg.send_data = (U8*)&addr;
    cfg.send_length = memAddrSize;
    cfg.slave_address = devAddr;
    r = hal_i2c_master_send_to_receive_polling(ih->port, &cfg);
    
    return r;
}




