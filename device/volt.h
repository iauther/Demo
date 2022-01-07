#ifndef __VOLT_H__
#define __VOLT_H__

#include "types.h"


int tcp232_init(void);
    
int tcp232_write(U8 *data, U32 len);


#endif

