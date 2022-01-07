#include "drv/i2c.h"
#include "drv/delay.h"
#include "lock.h"
#include "myCfg.h"


#if defined(__CC_ARM)
#pragma diag_suppress 1296
#endif


#define NMAX    5

#ifndef GPIOF
#define GPIOF 0
#endif


typedef struct {
    GPIO_TypeDef        *grp;
    U32                 pin;
    U32                 alt;
}pin_info_t;

typedef struct {
    pin_info_t          scl[NMAX];
    pin_info_t          sda[NMAX];
}i2c_pin_info_t;

typedef struct {
    pin_info_t          scl;
    pin_info_t          sda;
}i2c_info_t;
typedef struct {
    int                 port;
    U8                  useDMA;
    i2c_info_t          info;
    i2c_callback        callback;
    U8                  finish_flag;
    I2C_HandleTypeDef   hi2c;
    handle_t            lock;
}i2c_handle_t;


static i2c_pin_info_t pin_info[I2C_MAX]= {
    {//I2C_1
        {{GPIOB, GPIO_PIN_6, GPIO_AF4_I2C1}, {GPIOB, GPIO_PIN_8, GPIO_AF4_I2C1}, {NULL, 0, 0}},
        {{GPIOB, GPIO_PIN_7, GPIO_AF4_I2C1}, {GPIOB, GPIO_PIN_9, GPIO_AF4_I2C1}, {NULL, 0, 0}}
    },
    
    {//I2C_2
        {{GPIOB, GPIO_PIN_10, GPIO_AF4_I2C2}, {GPIOF, GPIO_PIN_1, GPIO_AF4_I2C2}, {NULL, 0, 0}},
        {{GPIOB, GPIO_PIN_3,  GPIO_AF9_I2C2}, {GPIOB, GPIO_PIN_9, GPIO_AF9_I2C2}, {GPIOB, GPIO_PIN_11, GPIO_AF4_I2C2}, {GPIOF, GPIO_PIN_0, GPIO_AF4_I2C2}, {NULL, 0, 0}}
    },
    
    {//I2C_3
        {{GPIOA, GPIO_PIN_8, GPIO_AF4_I2C3}, {NULL, 0, 0}},
        {{GPIOB, GPIO_PIN_8, GPIO_AF4_I2C3}, {GPIOC, GPIO_PIN_9, GPIO_AF4_I2C3}, {NULL, 0, 0}},
    }
};



I2C_TypeDef *i2cDef[I2C_MAX]={I2C1, I2C2, I2C3};
static i2c_handle_t *allHandle[I2C_MAX];
void HAL_I2C_MspInit(I2C_HandleTypeDef* i2cHandle)
{
    GPIO_InitTypeDef init={0};
    i2c_handle_t *h=NULL;
  
    init.Mode = GPIO_MODE_AF_OD;
    init.Pull = GPIO_PULLUP;
    init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    switch((U32)i2cHandle->Instance) {
        case (U32)I2C1:
        {
            h = allHandle[I2C_1];
            __HAL_RCC_I2C1_CLK_ENABLE();
            __HAL_RCC_GPIOB_CLK_ENABLE();
        }
        break;
        
        case (U32)I2C2:
        {
            h = allHandle[I2C_2];
            __HAL_RCC_I2C2_CLK_ENABLE();
#ifdef STM32F412Zx
            __HAL_RCC_GPIOF_CLK_ENABLE();
#else
            __HAL_RCC_GPIOB_CLK_ENABLE();
#endif  
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
    
    
    if(h->info.scl.grp==h->info.sda.grp && h->info.scl.alt==h->info.sda.alt) {
        init.Pin = h->info.scl.pin | h->info.sda.pin;
        init.Alternate = h->info.scl.alt;
        HAL_GPIO_Init(h->info.scl.grp, &init);
    }
    else {
        init.Pin = h->info.scl.pin;
        init.Alternate = h->info.scl.alt;
        HAL_GPIO_Init(h->info.scl.grp, &init);
        
        init.Pin = h->info.sda.pin;
        init.Alternate = h->info.sda.alt;
        HAL_GPIO_Init(h->info.sda.grp, &init);
    }
    
    //__I2C2_FORCE_RESET();
  	//__I2C2_RELEASE_RESET();
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
    
    if(h->info.scl.grp==h->info.sda.grp) {
        HAL_GPIO_DeInit(h->info.scl.grp, h->info.scl.pin | h->info.sda.pin);
    }
    else {
        HAL_GPIO_DeInit(h->info.scl.grp, h->info.scl.pin);
        HAL_GPIO_DeInit(h->info.sda.grp, h->info.sda.pin);
    }
}

static int get_port(i2c_info_t *info, i2c_pin_t *pin)
{
    int i,j,r=-1;
    int p1=-1,p2=-1;
    i2c_pin_info_t *inf=pin_info;
    
    for(i=0; i<I2C_MAX; i++) {
        for(j=0; j<NMAX; j++) {
            if(inf[i].scl[j].grp==pin->scl.grp && inf[i].scl[j].pin==pin->scl.pin ) {
                p1 = i;
                info->scl.alt = inf[i].scl[j].alt;
                break;
            }
        }
        
        for(j=0; j<NMAX; j++) {
            if(inf[i].sda[j].grp==pin->sda.grp && inf[i].sda[j].pin==pin->sda.pin ) {
                p2 = i;
                info->sda.alt = inf[i].sda[j].alt;
                break;
            }
        }
        
        if(p1!=-1 && p2!=-1 && p1==p2) {
            r = p1;
            break;
        }
    }
    
    return r;
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
    
    h->info.scl.grp  = cfg->pin.scl.grp;
    h->info.scl.pin  = cfg->pin.scl.pin;
    h->info.sda.grp  = cfg->pin.sda.grp;
    h->info.sda.pin  = cfg->pin.sda.pin;
    
    h->port = get_port(&h->info, &cfg->pin);
    h->hi2c.Instance = i2cDef[h->port];
    h->hi2c.Init = init;
    h->useDMA = cfg->useDMA;
    h->callback = cfg->callback;
    allHandle[h->port] = h;
    
    HAL_I2C_DeInit(&h->hi2c);
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
    
    lock_dynamic_free(&(*ih)->lock);
    HAL_I2C_DeInit(&(*ih)->hi2c);
    free(*ih);
    
    return 0;
}


static void i2c_reset(i2c_handle_t *h)
{
    HAL_I2C_DeInit(&h->hi2c);
    HAL_I2C_Init(&h->hi2c);
}


int i2c_read(handle_t h, U16 addr, U8 *data, U32 len, U8 bStop, U32 timeout)
{
    int r;
    i2c_handle_t *ih=(i2c_handle_t*)h;
    
    if(!h) {
        return -1;
    }
    
    lock_dynamic_hold(ih->lock);
    r = HAL_I2C_Master_Receive2(&ih->hi2c, (addr<<1), data, len, timeout, bStop);
    if(r!=HAL_OK) {
        i2c_reset(ih);
    }
    lock_dynamic_release(ih->lock);
    
    return r;
}


int i2c_write(handle_t h, U16 addr, U8 *data, U32 len, U8 bStop, U32 timeout)
{
    int r;
    i2c_handle_t *ih=(i2c_handle_t*)h;
    
    if(!h) {
        return -1;
    }
    
    lock_dynamic_hold(ih->lock);
    r = HAL_I2C_Master_Transmit2(&ih->hi2c, (addr<<1), data, len, timeout, bStop);
    if(r!=HAL_OK) {
        i2c_reset(ih);
    }
    lock_dynamic_release(ih->lock);
    
    return r;
}


#define _P0_SHIFT_VAL_MEMADD_SIZE_8BIT   7    /* Value for memory address shift */
#define _P0_BIT_SEL_MEMADD_SIZE_8BIT     0x0E /* Value for check P0 addressing bit in 8bit memory address mode */
#define _P0_SHIFT_VAL_MEMADD_SIZE_16BIT  15   /* Value for memory address shift */
#define _P0_BIT_SEL_MEMADD_SIZE_16BIT    0x02 /* Value for check P0 addressing bit in 16bit memory address mode */

#define CALC_DEVADD_8BIT(dev_add , mem_add)   ((dev_add) | (uint8_t)(((mem_add) >> _P0_SHIFT_VAL_MEMADD_SIZE_8BIT) & _P0_BIT_SEL_MEMADD_SIZE_8BIT)) /* Calculating new address */
#define CALC_DEVADD_16BIT(dev_add , mem_add)  ((dev_add) | (uint8_t)(((mem_add) >> _P0_SHIFT_VAL_MEMADD_SIZE_16BIT) & _P0_BIT_SEL_MEMADD_SIZE_16BIT)) /* Calculating new address */

int i2c_at_read(handle_t h, U16 devAddr, U16 memAddr, U16 memAddrSize, U8 *data, U32 len, U32 timeout)
{
    int r;
    U16 deviceAddr=(devAddr<<1);
    i2c_handle_t *ih=(i2c_handle_t*)h;
    
    if(!h) {
        return -1;
    }
    
    lock_dynamic_hold(ih->lock);
    deviceAddr = (memAddrSize==I2C_MEMADD_SIZE_8BIT)?CALC_DEVADD_8BIT(deviceAddr,memAddr):CALC_DEVADD_16BIT(deviceAddr,memAddr);
    r = HAL_I2C_Mem_Read(&ih->hi2c, deviceAddr, memAddr, memAddrSize, data, len, timeout);
    if(r!=HAL_OK) {
        i2c_reset(ih);
    }
    lock_dynamic_release(ih->lock);
    
    return r;
}


int i2c_at_write(handle_t h, U16 devAddr, U16 memAddr, U16 memAddrSize, U8 *data, U32 len, U32 timeout)
{
    int r;
    U16 deviceAddr=(devAddr<<1);
    i2c_handle_t *ih=(i2c_handle_t*)h;
    
    if(!h) {
        return -1;
    }
    
    lock_dynamic_hold(ih->lock);
    deviceAddr = (memAddrSize==I2C_MEMADD_SIZE_8BIT)?CALC_DEVADD_8BIT(deviceAddr,memAddr):CALC_DEVADD_16BIT(deviceAddr,memAddr);
    r = HAL_I2C_Mem_Write(&ih->hi2c, deviceAddr, memAddr, memAddrSize, data, len, timeout);
    if(r!=HAL_OK) {
        i2c_reset(ih);
    }
    lock_dynamic_release(ih->lock);
    
    return r;
}


int i2c_mem_read(handle_t h, U16 devAddr, U16 memAddr, U16 memAddrSize, U8 *data, U32 len, U32 timeout)
{
    int r;
    U16 deviceAddr=(devAddr<<1);
    i2c_handle_t *ih=(i2c_handle_t*)h;
    
    if(!h) {
        return -1;
    }
    
    lock_dynamic_hold(ih->lock);
    r = HAL_I2C_Mem_Read(&ih->hi2c, deviceAddr, memAddr, memAddrSize, data, len, timeout);
    if(r!=HAL_OK) {
        i2c_reset(ih);
    }
    lock_dynamic_release(ih->lock);
    
    return r;
}


int i2c_mem_write(handle_t h, U16 devAddr, U16 memAddr, U16 memAddrSize, U8 *data, U32 len, U32 timeout)
{
    int r;
    U16 deviceAddr=(devAddr<<1);
    i2c_handle_t *ih=(i2c_handle_t*)h;
    
    if(!h) {
        return -1;
    }
    
    lock_dynamic_hold(ih->lock);
    r = HAL_I2C_Mem_Write(&ih->hi2c, deviceAddr, memAddr, memAddrSize, data, len, timeout);
    if(r!=HAL_OK) {
        i2c_reset(ih);
    }
    lock_dynamic_release(ih->lock);
    
    return r;
}

