#include "drv/gpio.h"

#if defined(__CC_ARM)
#pragma diag_suppress 1296
#endif



#define INDEXOF(grp) ((U8)(((U32)(grp)-GPIOA_BASE)/0x400))
#define DATAOF(pin)  (&gpioData[(int)log2(pin)])
typedef struct {
    gpio_pin_t          pin;
    GPIO_InitTypeDef    init;
    gpio_callback       callback;
    U8                  enable;
}gpio_data_t;

static gpio_data_t gpioData[16]={0};
IRQn_Type gpioIRQn[4]={EXTI0_IRQn,EXTI1_IRQn,EXTI2_IRQn,EXTI3_IRQn};
static U32 modeMap[GPIO_MODE_MAX]={GPIO_MODE_INPUT,GPIO_MODE_OUTPUT_PP,GPIO_MODE_IT_RISING,GPIO_MODE_IT_FALLING,
                                   GPIO_MODE_IT_RISING_FALLING,GPIO_MODE_EVT_RISING,GPIO_MODE_EVT_FALLING,GPIO_MODE_EVT_RISING_FALLING};
                                            


void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
    int x=log2(pin);
    if(gpioData[x].callback) {
        gpioData[x].callback(&pin);
    }
}




int gpio_init(gpio_pin_t *pin, U8 mode)
{
    int r;
    gpio_data_t *pGpio;
    GPIO_InitTypeDef init={0};
    
    if(!pin) {
        return -1;
    }
    
    r = gpio_clk_en(pin, 1);
    if(r) {
        return -1;
    }

    init.Pin = pin->pin;
    init.Mode = modeMap[mode];
    init.Pull = GPIO_PULLUP;
    init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_DeInit(pin->grp, pin->pin);
    HAL_GPIO_Init(pin->grp, &init);
    
    pGpio = DATAOF(pin->pin);
    pGpio->callback = NULL;
    pGpio->enable = 0;
    pGpio->init = init;
    pGpio->pin  = *pin;
    
    return 0;
}


int gpio_deinit(gpio_pin_t *pin)
{
    if(!pin) {
        return -1;
    }
    
    HAL_GPIO_DeInit(pin->grp, pin->pin);
    return 0;
}



int gpio_set_dir(gpio_pin_t *pin, U8 dir)
{
    GPIO_InitTypeDef init;
    
    if(!pin) {
        return -1;
    }
    
    init.Pin = pin->pin;
    init.Mode = (dir==OUTPUT)?GPIO_MODE_OUTPUT_PP:GPIO_MODE_INPUT;
    init.Pull = GPIO_NOPULL;
    init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_DeInit(pin->grp, pin->pin);
    HAL_GPIO_Init(pin->grp, &init);
    
    DATAOF(pin->pin)->init = init;
    
    return 0;
}


int gpio_get_dir(gpio_pin_t *pin, U8 *hl)
{
    if(!pin || !hl) {
        return -1;
    }
    
    *hl = (DATAOF(pin->pin)->init.Mode==GPIO_MODE_INPUT)?INPUT:OUTPUT;
    
    return 0;
}


int gpio_set_hl(gpio_pin_t *pin, U8 hl)
{
    if(!pin) {
        return -1;
    }
    
    HAL_GPIO_WritePin(pin->grp, pin->pin, (GPIO_PinState)hl);
    return 0;
}


U8 gpio_get_hl(gpio_pin_t *pin)
{
    return HAL_GPIO_ReadPin(pin->grp, pin->pin);
}


int gpio_pull(gpio_pin_t *pin, U8 type)
{
    GPIO_InitTypeDef init;
    
    if(!pin) {
        return -1;
    }
    
    init = DATAOF(pin->pin)->init;
    
    init.Pin = pin->pin;
    init.Pull = type;
    init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_DeInit(pin->grp, pin->pin);
    HAL_GPIO_Init(pin->grp, &init);
    DATAOF(pin->pin)->init = init;
    
    return 0;
}



int gpio_set_callback(gpio_pin_t *pin, gpio_callback cb)
{   
    if(!pin) {
        return -1;
    }
    
    DATAOF(pin->pin)->callback = cb;
    
    //NVIC_SetPriorityGrouping(NVIC_PriorityGroup_4);
    //HAL_EXTI_RegisterCallback();
    //HAL_GPIO_EXTI_IRQHandler();
    return 0;
}


int gpio_irq_en(gpio_pin_t *pin, U8 on)
{
    IRQn_Type IRQn;
    
    if(!pin) {
        return -1;
    }
    
    IRQn = gpioIRQn[pin->pin];   
    HAL_NVIC_SetPriority(IRQn, 0, 0);
    if(on>0) {
        HAL_NVIC_EnableIRQ(IRQn);
    }
    else {
        HAL_NVIC_DisableIRQ(IRQn);
    }
    
    return 0;
}

int gpio_clk_en(gpio_pin_t *pin, U8 on)
{
    if(!pin) {
        return -1;
    }
    
    switch((U32)pin->grp) {
        case (U32)GPIOA:
        {
            if(on) __HAL_RCC_GPIOA_CLK_ENABLE();
            else   __HAL_RCC_GPIOA_CLK_DISABLE();
        }
        break;
        
        case (U32)GPIOB:
        {
            if(on) __HAL_RCC_GPIOB_CLK_ENABLE();
            else   __HAL_RCC_GPIOB_CLK_DISABLE();
        }
        break;
        
        case (U32)GPIOC:
        {
            if(on) __HAL_RCC_GPIOC_CLK_ENABLE();
            else   __HAL_RCC_GPIOC_CLK_DISABLE();
        }
        break;
        
        case (U32)GPIOD:
        {
            if(on) __HAL_RCC_GPIOD_CLK_ENABLE();
            else   __HAL_RCC_GPIOD_CLK_DISABLE();
        }
        break;

#ifdef STM32F412Zx      
        case (U32)GPIOE:
        {
            if(on) __HAL_RCC_GPIOE_CLK_ENABLE();
            else   __HAL_RCC_GPIOE_CLK_DISABLE();
        }
        break;
        
        case (U32)GPIOF:
        {
            if(on) __HAL_RCC_GPIOF_CLK_ENABLE();
            else   __HAL_RCC_GPIOF_CLK_DISABLE();
        }
        break;
        
        case (U32)GPIOG:
        {
            if(on) __HAL_RCC_GPIOG_CLK_ENABLE();
            else   __HAL_RCC_GPIOG_CLK_DISABLE();
        }
        break;
        
        case (U32)GPIOH:
        {
            if(on) __HAL_RCC_GPIOH_CLK_ENABLE();
            else   __HAL_RCC_GPIOH_CLK_DISABLE();
        }
        break;
#endif
        
        default:
        return -1;
    }
    
    return 0;
}

