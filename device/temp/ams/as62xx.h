#ifndef __AS62XX_Hx__
#define __AS62XX_Hx__

#include "types.h"


/* AS62XX_ registers */
#define AS62XX_REG_TVAL		        0x00
#define AS62XX_REG_CONFIG	        0x01
#define AS62XX_REG_TLOW		        0x02
#define AS62XX_REG_THIGH	        0x03


#define AS62XX_CONFIG_AL_MASK	    BIT(5)
#define AS62XX_CONFIG_AL_SHIFT	    5
#define AS62XX_CONFIG_CR_MASK	    GENMASK(7, 6)
#define AS62XX_CONFIG_CR_SHIFT	    6
#define AS62XX_CONFIG_SM_MASK       BIT(8)
#define AS62XX_CONFIG_SM_SHIFT      8
#define AS62XX_CONFIG_IM_MASK	    BIT(9)
#define AS62XX_CONFIG_IM_SHIFT	    9
#define AS62XX_CONFIG_POL_MASK	    BIT(10)
#define AS62XX_CONFIG_POL_SHIFT	    10
#define AS62XX_CONFIG_CF_MASK	    GENMASK(12, 11)
#define AS62XX_CONFIG_CF_SHIFT	    11

/* AS62XX_ init configuration values */
#define AS62XX_CONFIG_INIT_IM       0x0
#define AS62XX_CONFIG_INIT_POL	    0x0
#define AS62XX_CONFIG_INIT_CF       0x2

#define AS62XX_16BIT_DEFAULT_TLOW	0x2580



// Configuration constants
#define AS62XX_DEFAULT_CONFIG       0x40A0
#define AS62XX_DEFAULT_ADDRESS      0x48
#define AS62XX_T_ERR                0.1F
#define AS62XX_SINGLE_SHOT          0x8000

typedef enum {
  AS62XX_CONV_RATE025 = 0x0000,
  AS62XX_CONV_RATE1   = 0x0040,
  AS62XX_CONV_RATE4   = 0x0080,
  AS62XX_CONV_RATE8   = 0x00C0,
} as62xx_conv_rate_t;

typedef enum {
  AS62XX_STATE_SLEEP  = 0x0100,
  AS62XX_STATE_ACTIVE = 0x0000,
} as62xx_state_t;

typedef enum {
  AS62XX_CONSEC_FAULTS1 = 0x0000,
  AS62XX_CONSEC_FAULTS2 = 0x1000,
  AS62XX_CONSEC_FAULTS3 = 0x2000,
  AS62XX_CONSEC_FAULTS4 = 0x3000,
} as62xx_consec_faults_t;

typedef enum {
  AS62XX_ALERT_INTERRUPT  = 0x0200,
  AS62XX_ALERT_COMPARATOR = 0x0000,
} as62xx_alert_mode_t;

typedef enum {
  AS62XX_ALERT_ACTIVE_LOW  = 0x0000,
  AS62XX_ALERT_ACTIVE_HIGH = 0x0400,
} as62xx_alert_polarity_t;

typedef struct {
    as62xx_conv_rate_t cr;
    as62xx_state_t state;
    as62xx_alert_mode_t alert_mode;
    as62xx_alert_polarity_t alert_polarity;
    as62xx_consec_faults_t cf;
    U8 singleShot;
}as62xx_config_t;
/////////////////////////////////////////////


int as62xx_init(void);
int as62xx_test(void);
int as62xx_get(F32 *temp);



#endif

