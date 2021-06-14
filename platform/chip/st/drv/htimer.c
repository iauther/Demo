#include "drv/htimer.h"


#if 0
typedef struct {
    TIM_HandleTypeDef   htim;
    DMA_HandleTypeDef   hdma;
    htimer_callback_t   callback;
    
    
    
}htimer_handle_t;



DMA_HandleTypeDef htimerHdma[HTIMER_MAX];
htimer_callback_t htimerCallback[HTIMER_MAX];
TIM_HandleTypeDef htimer[HTIMER_MAX];
TIM_TypeDef *htimerDef[HTIMER_MAX]={TIM2, TIM3, TIM4, TIM5, TIM6, TIM7};
static U8 get_htimer(TIM_HandleTypeDef *htim)
{
    U8 i=0;

    for(i=0; i<HTIMER_MAX; i++) {
        if(htim->Instance == htimerDef[i]) {
            return i;
        }
    }
    return HTIMER_MAX;
} 

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
    GPIO_InitTypeDef init = {0};

    switch((U32)htim->Instance) {

        case (U32)TIM2:
        __HAL_RCC_TIM2_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();

        init.Pin = GPIO_PIN_11;
        init.Mode = GPIO_MODE_INPUT;
        init.Pull = GPIO_NOPULL;
        //init.Pull = GPIO_PULLDOWN;
        HAL_GPIO_Init(GPIOB, &init);

        //__HAL_AFIO_REMAP_TIM2_PARTIAL_2();
        
        HAL_NVIC_SetPriority(TIM2_IRQn, 4, 0);
        HAL_NVIC_EnableIRQ(TIM2_IRQn);
        
        break;

        case (U32)TIM3:
        __HAL_RCC_TIM3_CLK_ENABLE();
        break;

        case (U32)TIM4:
        __HAL_RCC_TIM4_CLK_ENABLE();
        break;

        case (U32)TIM5:
        __HAL_RCC_TIM5_CLK_ENABLE();
        break;

        case (U32)TIM6:
        __HAL_RCC_TIM6_CLK_ENABLE();
        break;

        case (U32)TIM7:
        __HAL_RCC_TIM7_CLK_ENABLE();
        break;
    }
}


void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef *htim)
{
    switch((U32)htim->Instance) {

        case (U32)TIM2:
        __HAL_RCC_TIM2_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_11);
        HAL_NVIC_DisableIRQ(TIM2_IRQn);
        break;

        case (U32)TIM3:
        __HAL_RCC_TIM3_CLK_DISABLE();
        break;

        case (U32)TIM4:
        __HAL_RCC_TIM4_CLK_DISABLE();
        break;

        case (U32)TIM5:
        __HAL_RCC_TIM5_CLK_DISABLE();
        break;

        case (U32)TIM6:
        __HAL_RCC_TIM6_CLK_DISABLE();
        break;

        case (U32)TIM7:
        __HAL_RCC_TIM7_CLK_DISABLE();
        break;
    }
} 


void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    U8 tim=get_htimer(htim);
    if(htimerCallback[tim]) {
        htimerCallback[tim](htim);
    }
}


int htimer_init(HTIMER tmr, TIM_Base_InitTypeDef *init, htimer_callback_t fn)
{
    TIM_ClockConfigTypeDef sClockSourceConfig;

    htimer[tmr].Instance = htimerDef[tmr];
    htimer[tmr].Init = *init;
    
    if (HAL_TIM_Base_Init(&htimer[tmr]) != HAL_OK) {
        return -1;
    }

    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htimer[tmr], &sClockSourceConfig) != HAL_OK) {
        return -1;
    }
    htimerCallback[tmr] = fn;

    return 0;
}


int htimer_deinit(HTIMER tmr)
{
    return HAL_TIM_Base_DeInit(&htimer[tmr]);
}


static int htime_dma_init(HTIMER tmr)
{
    htimerHdma[tmr].Instance = DMA_CHANNEL_0;
    htimerHdma[tmr].Init.Direction = DMA_PERIPH_TO_MEMORY;
    htimerHdma[tmr].Init.PeriphInc = DMA_PINC_DISABLE;
    htimerHdma[tmr].Init.MemInc = DMA_MINC_ENABLE;
    htimerHdma[tmr].Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    htimerHdma[tmr].Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    htimerHdma[tmr].Init.Mode = DMA_NORMAL;
    htimerHdma[tmr].Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&htimerHdma[tmr]) != HAL_OK) {
        return -1;
    }
    __HAL_LINKDMA(&htimer[tmr], hdma[TIM_DMA_ID_CC4], htimerHdma[tmr]);
    __HAL_LINKDMA(&htimer[tmr], hdma[TIM_DMA_ID_UPDATE], htimerHdma[tmr]);

    return 0;
}


static int htime_dma_deinit(HTIMER tmr)
{
    HAL_DMA_DeInit(htimer[tmr].hdma[TIM_DMA_ID_CC4]);
    HAL_DMA_DeInit(htimer[tmr].hdma[TIM_DMA_ID_UPDATE]);
    return 0;
}


int htimer_master_confg(HTIMER tmr, TIM_MasterConfigTypeDef *cfg)
{
    return HAL_TIMEx_MasterConfigSynchronization(&htimer[tmr], cfg);
}


int htimer_ic_config(HTIMER tmr, U32 channel, TIM_IC_InitTypeDef *cfg)
{
    int r;
    
    r = HAL_TIM_IC_Init(&htimer[tmr]);
    if (r==HAL_OK) {
        r = HAL_TIM_IC_ConfigChannel(&htimer[tmr], cfg, channel);
    }

    return r;
}


int htimer_start(HTIMER tmr)
{
    //return HAL_TIM_Base_Start(&htimer[tim]);
    return HAL_TIM_Base_Start_IT(&htimer[tmr]);
}


int htimer_stop(HTIMER tmr)
{
    //return HAL_TIM_Base_Stop(&htimer[tim]);
    return HAL_TIM_Base_Stop_IT(&htimer[tmr]);;
}

#endif


TIM_HandleTypeDef htim6;
static htimer_callback_t htim_callback=NULL;
void htimer_irq_handler()
{
    HAL_TIM_IRQHandler(&htim6);
    if(htim_callback) {
        htim_callback(0);
    }
}



void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* htim_base)
{
    if(htim_base->Instance==TIM6){
        __HAL_RCC_TIM6_CLK_ENABLE();
        
        /* TIM6 interrupt Init */
        HAL_NVIC_SetPriority(TIM6_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(TIM6_IRQn);
    }
}


void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* htim_base)
{
    if(htim_base->Instance==TIM6){
        __HAL_RCC_TIM6_CLK_DISABLE();
        HAL_NVIC_DisableIRQ(TIM6_IRQn);
    }
}

int htimer_start2(htimer_callback_t cb, int ms, int repeat)
{
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    
    htim6.Instance = TIM6;
    htim6.Init.Prescaler = 5;
    htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim6.Init.Period = 65535;
    htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_Base_Init(&htim6) != HAL_OK) {
        return -1;
    }
    
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_ENABLE;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK) {
        return -1;
    }
    
    return 0;
}


int htimer_stop2(void)
{
    if (HAL_TIM_Base_DeInit(&htim6) != HAL_OK) {
        return -1;
    }
    
    return 0;
}















