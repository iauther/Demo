#include "drv/i2c.h"
#include "drv/delay.h"
#include "lock.h"

#if defined(__CC_ARM)
#pragma diag_suppress 1296
#endif


#define NMAX    5


typedef struct {
    int                 port;
    U8                  useDMA;
    i2c_pin_t           pin;
    i2c_callback        callback;
    U8                  finish_flag;
    I2C_HandleTypeDef   hi2c;
    handle_t            lock;
}i2c_handle_t;


typedef struct {
    pin_t           scl[NMAX];
    pin_t           sda[NMAX];
}i2c_pin_info_t;


static i2c_pin_info_t pin_info[I2C_MAX]= {
    {//I2C_1
        {{GPIOB, GPIO_PIN_6}, {GPIOB, GPIO_PIN_8}, {NULL, 0}},
        {{GPIOB, GPIO_PIN_7}, {GPIOB, GPIO_PIN_9}, {NULL, 0}},
    },
    
    {//I2C_2
        {{GPIOB, GPIO_PIN_10}, {GPIOF, GPIO_PIN_1}, {NULL, 0}},
        {{GPIOB, GPIO_PIN_3}, {GPIOB, GPIO_PIN_9}, {GPIOB, GPIO_PIN_11}, {GPIOF, GPIO_PIN_0}, {NULL, 0}},
    },
    
    {//I2C_3
        {{GPIOA, GPIO_PIN_8}, {NULL, 0}},
        {{GPIOB, GPIO_PIN_8}, {GPIOB, GPIO_PIN_9}, {GPIOC, GPIO_PIN_9}, {NULL, 0}},
    }
};





I2C_TypeDef *i2cDef[I2C_MAX]={I2C1, I2C2, I2C3};
static i2c_handle_t *allHandle[I2C_MAX];
void HAL_I2C_MspInit(I2C_HandleTypeDef* i2cHandle)
{
    GPIO_InitTypeDef init={0};
    i2c_handle_t *h=NULL;
    
    switch((U32)i2cHandle->Instance) {
        case (U32)I2C1:
        {
            h = allHandle[I2C_1];
            __HAL_RCC_I2C1_CLK_ENABLE();
            __HAL_RCC_GPIOB_CLK_ENABLE();
            init.Alternate = GPIO_AF4_I2C1;
        }
        break;
        
        case (U32)I2C2:
        {
            h = allHandle[I2C_2];
            __HAL_RCC_I2C2_CLK_ENABLE();
            __HAL_RCC_GPIOF_CLK_ENABLE();
            init.Alternate = GPIO_AF4_I2C2;
        }
        break;
        
        case (U32)I2C3:
        {
            h = allHandle[I2C_3];
            __HAL_RCC_I2C3_CLK_ENABLE();
            __HAL_RCC_GPIOA_CLK_ENABLE();
            __HAL_RCC_GPIOC_CLK_ENABLE();
            init.Alternate = GPIO_AF4_I2C3;
        }
        break;
        
        default:
        return;
    }
    
    init.Mode = GPIO_MODE_AF_OD;
    init.Pull = GPIO_NOPULL;//GPIO_PULLUP;
    init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    if(h->pin.scl.grp==h->pin.sda.grp) {
        init.Pin = h->pin.scl.pin | h->pin.sda.pin;
        HAL_GPIO_Init(h->pin.scl.grp, &init);
    }
    else {
        init.Pin = h->pin.scl.pin;
        HAL_GPIO_Init(h->pin.scl.grp, &init);
        
        init.Pin = h->pin.sda.pin;
        HAL_GPIO_Init(h->pin.sda.grp, &init);
    }
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef* i2cHandle)
{
    i2c_handle_t *h=NULL;
    
    switch((U32)i2cHandle->Instance) {
        case (U32)I2C1:
        {
            h = allHandle[I2C_1];
            __HAL_RCC_I2C1_CLK_DISABLE();
        }
        break;
        
        case (U32)I2C2:
        {
            h = allHandle[I2C_2];
            __HAL_RCC_I2C2_CLK_DISABLE();
  
        }
        break;
        
        case (U32)I2C3:
        {
            h = allHandle[I2C_3];
            __HAL_RCC_I2C3_CLK_DISABLE();
        }
        break;
        
        default:
        return;
    }
    
    if(h->pin.scl.grp==h->pin.sda.grp) {
        HAL_GPIO_DeInit(h->pin.scl.grp, h->pin.scl.pin | h->pin.sda.pin);
    }
    else {
        HAL_GPIO_DeInit(h->pin.scl.grp, h->pin.scl.pin);
        HAL_GPIO_DeInit(h->pin.sda.grp, h->pin.sda.pin);
    }
}

static int get_port(i2c_pin_t *pin)
{
    int i,j;
    int p1=-1,p2=-1;
    i2c_pin_info_t *info;
    
    for(i=0; i<I2C_MAX; i++) {
        info = &pin_info[i];
        
        for(j=0; j<NMAX; j++) {
            if(info->scl[j].grp==pin->scl.grp && info->scl[j].pin==pin->scl.pin ) {
                p1 = i;
                break;
            }
        }
        
        for(j=0; j<NMAX; j++) {
            if(info->sda[j].grp==pin->sda.grp && info->sda[j].pin==pin->sda.pin ) {
                p2 = i;
                break;
            }
        }
    }
    
    if(p1!=-1 && p2!=-1 && p1==p2) {
        return p1;
    }
    
    return -1;
}    

handle_t i2c_init(i2c_cfg_t *cfg)
{
    i2c_handle_t *h;
    I2C_InitTypeDef init;
    HAL_StatusTypeDef st;
    
    if(!cfg) {
        return NULL;
    }
    h = calloc(1, sizeof(i2c_handle_t));
    if(!h) {
        return NULL;
    }
    
    init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    init.ClockSpeed = cfg->freq;
    init.DutyCycle = I2C_DUTYCYCLE_2;
    init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    init.OwnAddress1 = 0;
    init.OwnAddress2 = 0;
    
    h->port = get_port(&cfg->pin);
    h->pin  = cfg->pin;
    h->hi2c.Instance = i2cDef[h->port];
    h->hi2c.Init = init;
    h->useDMA = cfg->useDMA;
    h->callback = cfg->callback;
    allHandle[h->port] = h;
    
    st = HAL_I2C_Init(&h->hi2c);
    if(st!=HAL_OK) {
        free(h);
        h = NULL;
    }
    
    if(h) {
        h->lock = lock_dynamic_new();
    }
    
    return h;
}


int i2c_deinit(handle_t *h)
{
    i2c_handle_t **ih=(i2c_handle_t**)h;
    
    if(!ih || !(*ih)) {
        return -1;
    }
    
    HAL_I2C_DeInit(&(*ih)->hi2c);
    lock_dynamic_free(&(*ih)->lock);
    free(*ih);
    
    return 0;
}


int i2c_read(handle_t h, U16 addr, U8 *data, U32 len)
{
    int r;
    i2c_handle_t *ih=(i2c_handle_t*)h;
    
    if(!h) {
        return -1;
    }
    
    lock_dynamic_hold(ih->lock);
    r = HAL_I2C_Master_Receive(&ih->hi2c, (addr<<1), data, len, HAL_MAX_DELAY);
    lock_dynamic_release(ih->lock);
    return r;
}


int i2c_write(handle_t h, U16 addr, U8 *data, U32 len)
{
    int r;
    i2c_handle_t *ih=(i2c_handle_t*)h;
    
    if(!h) {
        return -1;
    }
    
    lock_dynamic_hold(ih->lock);
    r = HAL_I2C_Master_Transmit(&ih->hi2c, (addr<<1), data, len, HAL_MAX_DELAY);
    lock_dynamic_release(ih->lock);
    return r;
}


int i2c_mem_read(handle_t h, U16 devAddr, U16 memAddr, U16 memAddrSize, U8 *data, U32 len)
{
    i2c_handle_t *ih=(i2c_handle_t*)h;
    
    if(!h) {
        return -1;
    }
    
    return HAL_I2C_Mem_Read(&ih->hi2c, (devAddr<<1), memAddr, memAddrSize, data, len, HAL_MAX_DELAY);
}


int i2c_mem_write(handle_t h, U16 devAddr, U16 memAddr, U16 memAddrSize, U8 *data, U32 len)
{
    i2c_handle_t *ih=(i2c_handle_t*)h;
    
    if(!h) {
        return -1;
    }
    
    return HAL_I2C_Mem_Write(&ih->hi2c, (devAddr<<1), memAddr, memAddrSize, data, len, HAL_MAX_DELAY);
}


int i2c_mem_is_ready(handle_t h, U16 devAddr)
{
    HAL_StatusTypeDef st;
    i2c_handle_t *ih=(i2c_handle_t*)h;
    
    if(!h) {
        return -1;
    }
    
    st = HAL_I2C_IsDeviceReady(&ih->hi2c, (devAddr<<1),  3, HAL_MAX_DELAY);
    
    return (st==HAL_OK)?1:0;
}

