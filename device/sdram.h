#ifndef __SDRAM_Hx__
#define __SDRAM_Hx__

#include "types.h"

#define SDRAM_SIZE          (64*1024*1024)
#define BANK6_SDRAM_ADDR    ((U32)(0xD0000000)) // SDRAM开始地址

//SDRAM配置参数
#define SDRAM_MODEREG_BURST_LENGTH_1             ((U16)0x0000)
#define SDRAM_MODEREG_BURST_LENGTH_2             ((U16)0x0001)
#define SDRAM_MODEREG_BURST_LENGTH_4             ((U16)0x0002)
#define SDRAM_MODEREG_BURST_LENGTH_8             ((U16)0x0004)
#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL      ((U16)0x0000)
#define SDRAM_MODEREG_BURST_TYPE_INTERLEAVED     ((U16)0x0008)
#define SDRAM_MODEREG_CAS_LATENCY_2              ((U16)0x0020)
#define SDRAM_MODEREG_CAS_LATENCY_3              ((U16)0x0030)
#define SDRAM_MODEREG_OPERATING_MODE_STANDARD    ((U16)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_PROGRAMMED ((U16)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE     ((U16)0x0200)


int sdram_init(void);
int sdram_read(U8 *buf, U32 offset, U32 len);
int sdram_write(U8 *buf, U32 offset, U32 len);

#endif

