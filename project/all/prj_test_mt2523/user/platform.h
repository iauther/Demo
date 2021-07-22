#ifndef __PLATFORM_Hx__
#define __PLATFORM_Hx__

#ifndef _WIN32

#if (defined STM32F412ZX) || (defined STM32F412RX)
#include "stm32f4xx.h"
#elif defined MT2523
#include "mt2523.h"
#else
#include "hal.h"
//error "please define a platform"
#endif

#endif

#endif
