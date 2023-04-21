#include "dal_gpio.h"

#if defined(__CC_ARM)
#pragma diag_suppress 1296
#endif


#define INDEX(gpio)     ((int)log2(gpio))
#define GRP_INDEX(grp)  ((U8)(((U32)(grp)-GPIOA_BASE)/0x400))
#define DATA_OF(gpio)   (&gpioData[INDEX(gpio)])
typedef struct {
    dal_gpio_pin_t          pin;
    GPIO_InitTypeDef    init;
    gpio_callback       callback;
    U8                  enable;
}gpio_data_t;

static gpio_data_t gpioData[16]={0};
IRQn_Type gpioIRQn[4]={EXTI0_IRQn,EXTI1_IRQn,EXTI2_IRQn,EXTI3_IRQn};
static U32 modeMap[GPIO_MODE_MAX]={GPIO_MODE_INPUT,GPIO_MODE_OUTPUT_PP,GPIO_MODE_IT_RISING,GPIO_MODE_IT_FALLING,
                                   GPIO_MODE_IT_RISING_FALLING,GPIO_MODE_EVT_RISING,GPIO_MODE_EVT_FALLING,GPIO_MODE_EVT_RISING_FALLING};
                                            

static uint32_t gpioPin[16]={GPIO_PIN_0,GPIO_PIN_1,GPIO_PIN_2,GPIO_PIN_3,GPIO_PIN_4,GPIO_PIN_5,GPIO_PIN_6,GPIO_PIN_7,
                             GPIO_PIN_8,GPIO_PIN_9,GPIO_PIN_10,GPIO_PIN_11,GPIO_PIN_12,GPIO_PIN_13,GPIO_PIN_14,GPIO_PIN_15};
static void gpio_irq_handle(void)
{
    int i;
    for(i=0;i<16;i++) {
        HAL_GPIO_EXTI_IRQHandler(gpioPin[i]);
    }
}

void EXTI0_IRQHandler(void)
{
    gpio_irq_handle();
}
void EXTI1_IRQHandler(void)
{
    gpio_irq_handle();
}
void EXTI2_IRQHandler(void)
{
    gpio_irq_handle();
}
void EXTI3_IRQHandler(void)
{
    gpio_irq_handle();
}
void EXTI4_IRQHandler(void)
{
    gpio_irq_handle();
}
void EXTI5_IRQHandler(void)
{
    gpio_irq_handle();
}
                                   
                                   
                                   
void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
    int x=log2(pin);
    if(gpioData[x].callback) {
        gpioData[x].callback(&pin);
    }
}




int dal_gpio_init(dal_gpio_pin_t *pin, U8 mode)
{
    int r;
    gpio_data_t *pGpio;
    GPIO_InitTypeDef init={0};
    
    if(!pin) {
        return -1;
    }
    
    r = dal_gpio_clk_en(pin, 1);
    if(r) {
        return -1;
    }

    init.Pin = pin->pin;
    init.Mode = modeMap[mode];
    init.Pull = GPIO_NOPULL;
    init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_DeInit(pin->grp, pin->pin);
    HAL_GPIO_Init(pin->grp, &init);
    
    pGpio = DATA_OF(pin->pin);
    pGpio->callback = NULL;
    pGpio->enable = 0;
    pGpio->init = init;
    pGpio->pin  = *pin;
    
    return 0;
}


int dal_gpio_deinit(dal_gpio_pin_t *pin)
{
    if(!pin) {
        return -1;
    }
    
    HAL_GPIO_DeInit(pin->grp, pin->pin);
    return 0;
}


int dal_gpio_set_dir(dal_gpio_pin_t *pin, U8 dir)
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
    
    DATA_OF(pin->pin)->init = init;
    
    return 0;
}


int dal_gpio_get_dir(dal_gpio_pin_t *pin, U8 *hl)
{
    if(!pin || !hl) {
        return -1;
    }
    
    *hl = (DATA_OF(pin->pin)->init.Mode==GPIO_MODE_INPUT)?INPUT:OUTPUT;
    
    return 0;
}


int dal_gpio_set_hl(dal_gpio_pin_t *pin, U8 hl)
{
    if(!pin) {
        return -1;
    }
    
    HAL_GPIO_WritePin(pin->grp, pin->pin, (GPIO_PinState)hl);
    return 0;
}


int dal_gpio_get_hl(dal_gpio_pin_t *pin)
{
    return HAL_GPIO_ReadPin(pin->grp, pin->pin);
}


int dal_gpio_pull(dal_gpio_pin_t *pin, U8 type)
{
    GPIO_InitTypeDef init;
    
    if(!pin) {
        return -1;
    }
    
    init = DATA_OF(pin->pin)->init;
    
    init.Pin = pin->pin;
    init.Pull = type;
    init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_DeInit(pin->grp, pin->pin);
    HAL_GPIO_Init(pin->grp, &init);
    DATA_OF(pin->pin)->init = init;
    
    return 0;
}



int dal_gpio_set_callback(dal_gpio_pin_t *pin, gpio_callback cb)
{
    int index;
    
    if(!pin) {
        return -1;
    }
    
    DATA_OF(pin->pin)->callback = cb;
    
    //NVIC_SetPriorityGrouping(NVIC_PriorityGroup_4);
    //HAL_EXTI_RegisterCallback();
    //HAL_GPIO_EXTI_IRQHandler();
    return 0;
}


int dal_gpio_irq_en(dal_gpio_pin_t *pin, U8 on)
{
    IRQn_Type IRQn;
    
    if(!pin) {
        return -1;
    }
    
    IRQn = gpioIRQn[pin->pin];   
    HAL_NVIC_SetPriority(IRQn, 8, 0);
    if(on>0) {
        HAL_NVIC_EnableIRQ(IRQn);
    }
    else {
        HAL_NVIC_DisableIRQ(IRQn);
    }
    
    return 0;
}

int dal_gpio_clk_en(dal_gpio_pin_t *pin, U8 on)
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
        
        case (U32)GPIOI:
        if(on) __HAL_RCC_GPIOI_CLK_ENABLE();
        else   __HAL_RCC_GPIOI_CLK_DISABLE();
        break;
        
        case (U32)GPIOJ:
        if(on) __HAL_RCC_GPIOJ_CLK_ENABLE();
        //else   __HAL_RCC_GPIOJ_CLK_DISABLE();
        break;
        
        case (U32)GPIOK:
        if(on) __HAL_RCC_GPIOK_CLK_ENABLE();
        //else   __HAL_RCC_GPIOK_CLK_DISABLE();
#endif
        
        default:
        return -1;
    }
    
    return 0;
}

