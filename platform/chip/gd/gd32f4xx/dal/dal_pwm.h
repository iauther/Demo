#ifndef __DAL_PWM_Hx__
#define __DAL_PWM_Hx__

#include "types.h"

int dal_pwm_init(U32 freq);
int dal_pwm_deinit(void);
int dal_pwm_deinit(void);
int dal_pwm_start(void);
int dal_pwm_stop(void);

#endif

