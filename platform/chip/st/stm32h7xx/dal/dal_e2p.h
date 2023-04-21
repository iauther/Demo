#ifndef __DAL_E2P_Hx__
#define __DAL_E2P_Hx__

#include "types.h"

int dal_e2p_init(void);
int dal_e2p_read_byte(U32 addr, U8* data);
int dal_e2p_write_byte(U32 addr, U8* data);
int dal_e2p_read(U32 addr, U8 *data, U32 len);
int dal_e2p_write(U32 addr, U8 *data, U32 len);
void dal_e2p_test(void);

#endif

