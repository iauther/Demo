#ifndef __BUZZER_Hx__
#define __BUZZER_Hx__

#include "types.h"

typedef struct {
    U16     ring_time;
    U16     quiet_time;
    S32     total_time;
}buzzer_cfg_t;




int buzzer_init(void);
int buzzer_deinit(void);
int buzzer_start(buzzer_cfg_t *cfg);
int buzzer_stop(void);

int buzzer_test(void);

#endif



