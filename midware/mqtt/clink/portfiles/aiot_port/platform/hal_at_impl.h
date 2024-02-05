#if (MQTT_LIB==1)

#ifndef __HAL_AT_IMPL_Hx__
#define __HAL_AT_IMPL_Hx__

#include "aiot_at_api.h"

int hal_at_init(void);
int hal_at_boot(void);
int hal_at_ntp(void);
int hal_at_power(int on);
int hal_at_reset(void);

#endif

#endif
