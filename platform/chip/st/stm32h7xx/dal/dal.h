#ifndef __DAL_Hx__
#define __DAL_Hx__


#include "dal_adc.h"
#include "dal_delay.h"
#include "dal_e2p.h"
#include "dal_flash.h"
#include "dal_gpio.h"
#include "dal_timer.h"
#include "dal_i2c.h"
#include "dal_si2c.h"
#include "dal_jump.h"
#include "dal_pwm.h"
#include "dal_spi.h"
#include "dal_uart.h"
#include "dal_wdg.h"
#include "dal_sdram.h"
#include "dal_eth.h"
//#include "dal_dma.h"
//#include "dal_si2c.h"

int dal_init(void);

U32 dal_get_freq(void);


#endif

