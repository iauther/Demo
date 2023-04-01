#ifndef __ADAU197x_Hx__
#define __ADAU197x_Hx__

#include "types.h"

enum {
    ADAU197X_ID_1=0,
    ADAU197X_ID_2,
    
    ADAU197X_ID_MAX
};



// ADAU1978¼Ä´æÆ÷Ãû³Æ¶¨Òå
#define M_POWER_REG				(0x00)
#define PLL_CONTROL_REG			(0x01)
#define BLOCK_POWER_SAI_REG		(0x04)
#define SAI_CTRL0_REG			(0x05)
#define SAI_CTRL1_REG			(0x06)
#define SAI_CMAP12_REG			(0x07)
#define SAI_CMAP34_REG			(0x08)
#define SAI_OVERTEMP_REG		(0x09)
#define POSTADC_GAIN1_REG		(0x0A)
#define POSTADC_GAIN2_REG		(0x0B)
#define POSTADC_GAIN3_REG		(0x0C)
#define POSTADC_GAIN4_REG		(0x0D)
#define MISC_CONTROL_REG		(0x0E)
#define ASDC_CLIP_REG			(0x19)
#define DC_HPF_CAL_REG			(0x1A)


int adau197x_init(void);
int adau197x_config(void);
int adau197x_get_status(U8 *status);

#endif



