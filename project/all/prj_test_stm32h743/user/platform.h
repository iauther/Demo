#ifndef __PLATFORM_Hx__
#define __PLATFORM_Hx__

#ifndef _WIN32
#include "stm32h7xx.h"
#endif

#ifdef OS_KERNEL
#include "cmsis_os2.h"
#endif

#define CMSIS_device_header "platform.h"

#endif
