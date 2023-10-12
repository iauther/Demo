#ifndef __DAL_Hx__
#define __DAL_Hx__


#include "types.h"


typedef struct {
    U32 flashSize;
    U32 flashStart;
    U32 flashEnd;
    
    U32 sramSize;
    U32 sramStart;
    U32 sramEnd;
    
	U32 uniqueID[3];
}mcu_info_t;



int dal_init(void);
void dal_set_priority(void);
int dal_set_vector(void);
void dal_reboot(void);
U32 dal_get_freq(void);
int dal_get_info(mcu_info_t *info);
U32 dal_get_chipid(void);

#endif

