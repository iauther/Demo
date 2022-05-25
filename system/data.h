#ifndef __DATA_Hx__
#define __DATA_Hx__

#ifdef BOARD_PUMP_F412RET6
#include "data_pump.h"
#elif defined TEST_MISTRIG_BOX
#include "data_testbox.h"
#else
error no data defined!
#endif


#endif

