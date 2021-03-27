#ifndef __AT24CXX_Hx__
#define _AT24CXX_Hx__

#include "types.h"
#include "cfg.h"

#define AT24C01		127
#define AT24C02		255
#define AT24C04		511
#define AT24C08		1023
#define AT24C16		2047
#define AT24C32		4095
#define AT24C64	    8191
#define AT24C128	16383
#define AT24C256	32767  

#define EE_TYPE AT24C02

#define AT_ADDR		0xa0

#define AT_CHECK_ADDR	0X00
#define AT_CHECK_VALUE	0X52


#define A0_PIN_BIT 1 /* Address pin bit in device address */
#define A1_PIN_BIT 2 /* Address pin bit in device address */
#define A2_PIN_BIT 3 /* Address pin bit in device address */
#ifdef USE_AT24C01
	#define DEV_ADDRESS      (0xA0 | (AT24CXX_A2_PIN << A2_PIN_BIT) | (AT24CXX_A1_PIN << A1_PIN_BIT) | (AT24CXX_A0_PIN << A0_PIN_BIT)) /* Memory IC Address */
	#define MEMADD_SIZE      I2C_MEMADD_SIZE_8BIT  /* Memory address size */
	#define MEM_PAGE_SIZE    8U                    /* Size of memory page */
	#define MEM_NUM_OF_PAGE  16U                   /* Number of memory page */
	#define MEM_STWC         10U                   /* Self Timed Write Cycle */
	#define MEM_MAX_SIZE     ((MEM_PAGE_SIZE) * (MEM_NUM_OF_PAGE)) /* Maximum memory size */
#elif defined(USE_AT24C02)
	#define DEV_ADDRESS      (0xA0 | (AT24CXX_A2_PIN << A2_PIN_BIT) | (AT24CXX_A1_PIN << A1_PIN_BIT) | (AT24CXX_A0_PIN << A0_PIN_BIT)) /* Memory IC Address */
	#define MEMADD_SIZE      I2C_MEMADD_SIZE_8BIT /* Memory address size */
	#define MEM_PAGE_SIZE    8U                   /* Size of memory page */
	#define MEM_NUM_OF_PAGE  32U                  /* Number of memory page */
	#define MEM_STWC         10U                  /* Self Timed Write Cycle */
	#define MEM_MAX_SIZE     ((MEM_PAGE_SIZE) * (MEM_NUM_OF_PAGE)) /* Maximum memory size */
#elif defined(USE_AT24C04)
	#define DEV_ADDRESS      (0xA0 | (AT24CXX_A2_PIN << A2_PIN_BIT) | (AT24CXX_A1_PIN << A1_PIN_BIT)) /* Memory IC Address */
	#define MEMADD_SIZE      I2C_MEMADD_SIZE_8BIT /* Memory address size */
	#define MEM_PAGE_SIZE    16U                  /* Size of memory page */
	#define MEM_NUM_OF_PAGE  32U                  /* Number of memory page */
	#define MEM_STWC         10U                  /* Self Timed Write Cycle */
	#define MEM_MAX_SIZE     ((MEM_PAGE_SIZE) * (MEM_NUM_OF_PAGE)) /* Maximum memory size */
#elif defined(USE_AT24C08)
	#define DEV_ADDRESS      (0xA0 | (AT24CXX_A2_PIN << A2_PIN_BIT)) /* Memory IC Address */
	#define MEMADD_SIZE      I2C_MEMADD_SIZE_8BIT /* Memory address size */
	#define MEM_PAGE_SIZE    16U                  /* Size of memory page */
	#define MEM_NUM_OF_PAGE  64U                  /* Number of memory page */
	#define MEM_STWC         10U                  /* Self Timed Write Cycle */
	#define MEM_MAX_SIZE     ((MEM_PAGE_SIZE) * (MEM_NUM_OF_PAGE)) /* Maximum memory size */
#elif defined(USE_AT24C16)
	#define DEV_ADDRESS      0xA0 /* Memory IC Address */
	#define MEMADD_SIZE      I2C_MEMADD_SIZE_8BIT /* Memory address size */
	#define MEM_PAGE_SIZE    16U                  /* Size of memory page */
	#define MEM_NUM_OF_PAGE  128U                 /* Number of memory page */
	#define MEM_STWC         10U                  /* Self Timed Write Cycle */
	#define MEM_MAX_SIZE     ((MEM_PAGE_SIZE) * (MEM_NUM_OF_PAGE)) /* Maximum memory size */
#elif defined(USE_AT24C32)
	#define DEV_ADDRESS      (0xA0 | (AT24CXX_A2_PIN << A2_PIN_BIT) | (AT24CXX_A1_PIN << A1_PIN_BIT) | (AT24CXX_A0_PIN << A0_PIN_BIT)) /* Memory IC Address */
	#define MEMADD_SIZE      I2C_MEMADD_SIZE_16BIT /* Memory address size */
	#define MEM_PAGE_SIZE    32U                   /* Size of memory page */
	#define MEM_NUM_OF_PAGE  128U                  /* Number of memory page */
	#define MEM_STWC         10U                   /* Self Timed Write Cycle */
	#define MEM_MAX_SIZE     ((MEM_PAGE_SIZE) * (MEM_NUM_OF_PAGE)) /* Maximum memory size */
#elif defined(USE_AT24C64)
	#define DEV_ADDRESS      (0xA0 | (AT24CXX_A2_PIN << A2_PIN_BIT) | (AT24CXX_A1_PIN << A1_PIN_BIT) | (AT24CXX_A0_PIN << A0_PIN_BIT)) /* Memory IC Address */
	#define MEMADD_SIZE      I2C_MEMADD_SIZE_16BIT /* Memory address size */
	#define MEM_PAGE_SIZE    32U                   /* Size of memory page */
	#define MEM_NUM_OF_PAGE  256U                  /* Number of memory page */
	#define MEM_STWC         10U                   /* Self Timed Write Cycle */
	#define MEM_MAX_SIZE     ((MEM_PAGE_SIZE) * (MEM_NUM_OF_PAGE)) /* Maximum memory size */
#elif defined(USE_AT24C128)
	#define DEV_ADDRESS      (0xA0 | (AT24CXX_A1_PIN << A1_PIN_BIT) | (AT24CXX_A0_PIN << A0_PIN_BIT)) /* Memory IC Address */
	#define MEMADD_SIZE      I2C_MEMADD_SIZE_16BIT /* Memory address size */
	#define MEM_PAGE_SIZE    64U                   /* Size of memory page */
	#define MEM_NUM_OF_PAGE  256U                  /* Number of memory page */
	#define MEM_STWC         5U                    /* Self Timed Write Cycle */
	#define MEM_MAX_SIZE     ((MEM_PAGE_SIZE) * (MEM_NUM_OF_PAGE)) /* Maximum memory size */
#elif defined(USE_AT24C256)
	#define DEV_ADDRESS      (0xA0 | (AT24CXX_A1_PIN << A1_PIN_BIT) | (AT24CXX_A0_PIN << A0_PIN_BIT)) /* Memory IC Address */
	#define MEMADD_SIZE      I2C_MEMADD_SIZE_16BIT /* Memory address size */
	#define MEM_PAGE_SIZE    64U                   /* Size of memory page */
	#define MEM_NUM_OF_PAGE  512U                  /* Number of memory page */
	#define MEM_STWC         5U                    /* Self Timed Write Cycle */
	#define MEM_MAX_SIZE     ((MEM_PAGE_SIZE) * (MEM_NUM_OF_PAGE)) /* Maximum memory size */
#elif defined(USE_AT24C512)
	#define DEV_ADDRESS      (0xA0 | (AT24CXX_A1_PIN << A1_PIN_BIT) | (AT24CXX_A0_PIN << A0_PIN_BIT)) /* Memory IC Address */
	#define MEMADD_SIZE      I2C_MEMADD_SIZE_16BIT /* Memory address size */
	#define MEM_PAGE_SIZE    128UL                  /* Size of memory page */
	#define MEM_NUM_OF_PAGE  512UL                  /* Number of memory page */
	#define MEM_STWC         5U                    /* Self Timed Write Cycle */
	#define MEM_MAX_SIZE     ((MEM_PAGE_SIZE) * (MEM_NUM_OF_PAGE)) /* Maximum memory size */
#elif defined(USE_AT24C1024)
	#define DEV_ADDRESS      (0xA0 | (AT24CXX_A2_PIN << A2_PIN_BIT) | (AT24CXX_A1_PIN << A1_PIN_BIT)) /* Memory IC Address */
	#define MEMADD_SIZE      I2C_MEMADD_SIZE_16BIT /* Memory address size */
	#define MEM_PAGE_SIZE    256UL                  /* Size of memory page */
	#define MEM_NUM_OF_PAGE  512UL                  /* Number of memory page */
	#define MEM_STWC         5U                    /* Self Timed Write Cycle */
	#define MEM_MAX_SIZE     ((MEM_PAGE_SIZE) * (MEM_NUM_OF_PAGE)) /* Maximum memory size */
#else
	#error [MEM ERROR01]Memory is not selected Or not supported.
#endif


int at24cxx_init(void);
int at24cxx_read(U16 addr, U8 *data, U16 len);
int at24cxx_write(U16 addr, U8 *data, U16 len);

#endif



