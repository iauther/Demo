#include "drv/gpio.h"

#if defined(__CC_ARM)
#pragma diag_suppress 1296
#endif


#define INDEX(gpio)     ((int)log2(gpio))
#define GRP_INDEX(grp)  ((U8)(((U32)(grp)-GPIOA_BASE)/0x400))
#define DATA_OF(gpio)   (&gpioData[INDEX(gpio)])
typedef struct {
    gpio_pin_t          pin;
    GPIO_InitTypeDef    init;
    gpio_callback       callback;
    U8                  enable;
}gpio_data_t;

static gpio_data_t gpioData[6][16]={0};
IRQn_Type gpioIRQn[4]={EXTI0_IRQn,EXTI1_IRQn,EXTI2_IRQn,EXTI3_IRQn};
static U32 modeMap[GPIO_MODE_MAX]={GPIO_MODE_INPUT,GPIO_MODE_OUTPUT_PP,GPIO_MODE_IT_RISING,GPIO_MODE_IT_FALLING,
                                   GPIO_MODE_IT_RISING_FALLING,GPIO_MODE_EVT_RISING,GPIO_MODE_EVT_FALLING,GPIO_MODE_EVT_RISING_FALLING};
                                            

static uint32_t gpioPin[16]={GPIO_PIN_0,GPIO_PIN_1,GPIO_PIN_2,GPIO_PIN_3,GPIO_PIN_4,GPIO_PIN_5,GPIO_PIN_6,GPIO_PIN_7,
                             GPIO_PIN_8,GPIO_PIN_9,GPIO_PIN_10,GPIO_PIN_11,GPIO_PIN_12,GPIO_PIN_13,GPIO_PIN_14,GPIO_PIN_15};

static void my_gpio_callback(uint16_t pin)
{
    int x=log2(pin);
//    if(gpioData[x].callback) {
//        gpioData[x].callback(&pin);
//    }
}
static void my_gpio_handler(GPIO_TypeDef *grp, U16 pin)
{
  if (__HAL_GPIO_EXTI_GET_IT(pin) != 0x00u) {
        __HAL_GPIO_EXTI_CLEAR_IT(pin);
        my_gpio_callback(pin);
  }
    
    
}


static void gpio_irq_handle(GPIO_TypeDef *grp)
{
    int i;
    for(i=0;i<16;i++) {
        my_gpio_handler(grp, gpioPin[i]);
    }
}

void EXTI0_IRQHandler(void)
{
    gpio_irq_handle(GPIOA);
}
void EXTI1_IRQHandler(void)
{
    gpio_irq_handle(GPIOB);
}
void EXTI2_IRQHandler(void)
{
    gpio_irq_handle(GPIOC);
}
void EXTI3_IRQHandler(void)
{
    gpio_irq_handle(GPIOD);
}
void EXTI4_IRQHandler(void)
{
    gpio_irq_handle(GPIOE);
}
void EXTI5_IRQHandler(void)
{
    gpio_irq_handle(GPIOF);
}
                                   



int gpio_init(gpio_cfg_t *cfg)
{
    int r;
    gpio_data_t *pGpio;
    GPIO_InitTypeDef init={0};
    
    if(!cfg) {
        return -1;
    }
    
    r = gpio_clk_en(&cfg->pin, 1);
    if(r) {
        return -1;
    }

    init.Pin = cfg->pin.pin;
    init.Mode = modeMap[cfg->mode];
    init.Pull = GPIO_NOPULL;
    init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_DeInit(cfg->pin.grp, cfg->pin.pin);
    HAL_GPIO_Init(cfg->pin.grp, &init);
/*    
    pGpio = DATA_OF(pin->pin);
    pGpio->callback = NULL;
    pGpio->enable = 0;
    pGpio->init = init;
    pGpio->pin  = *pin;
*/
    
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
    
//    DATA_OF(pin->pin)->init = init;
    
    return 0;
}


int gpio_get_dir(gpio_pin_t *pin, U8 *hl)
{
    if(!pin || !hl) {
        return -1;
    }
    
//    *hl = (DATA_OF(pin->pin)->init.Mode==GPIO_MODE_INPUT)?INPUT:OUTPUT;
    
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
    
//  init = DATA_OF(pin->pin)->init;
    
    init.Pin = pin->pin;
    init.Pull = type;
    init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_DeInit(pin->grp, pin->pin);
    HAL_GPIO_Init(pin->grp, &init);
//    DATA_OF(pin->pin)->init = init;
    
    return 0;
}



int gpio_set_callback(gpio_pin_t *pin, gpio_callback cb)
{
    int index;
    
    if(!pin) {
        return -1;
    }
    
//    DATA_OF(pin->pin)->callback = cb;
    
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

