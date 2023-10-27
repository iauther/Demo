#ifndef __SD30XX_Hx__
#define __SD30XX_Hx__

#include "types.h"



typedef enum {
    F_0Hz=0,
    F32KHz,
    F4096Hz,
    F1024Hz,
    F64Hz,
    F32Hz,
    F16Hz,
    F8Hz,
    F4Hz,
    F2Hz,
    F1Hz,
    F1_2Hz,
    F1_4Hz,
    F1_8Hz,
    F1_16Hz,
    F_1s
}SD_FREQ;

typedef enum {
    S_4096Hz=0,
    S_1024Hz,
    S_1s,
    S_1min
}SD_CLK;

typedef enum {
    F_12H=0,
    F_24H,
}SD_FMT;

typedef enum {
    IRQ_FREQ=0,
    IRQ_ALARM,
    IRQ_COUNTDOWN,
}SD_IRQ;


typedef struct
{
    u8  year;
    u8  month;
    u8  day;
    u8  week;
    u8  hour;
    u8  minute;
    u8  second;
}sd_time_t;


typedef struct
{
    SD_CLK  clk;
    u8      im; //im=1:周期性中断
    u32     val;
}sd_countdown_t;


int sd30xx_init(void);
int sd30xx_write_time(sd_time_t *tm);
int sd30xx_read_time(sd_time_t *tm);
int sd30xx_set_countdown(sd_countdown_t *cd);
int sd30xx_set_alarm(u8 en, sd_time_t *tm);
int sd30xx_set_freq(SD_FREQ freq);
int sd30xx_set_irq(SD_IRQ irq, u8 on);
int sd30xx_clr_irq(void);

int sd30xx_read_id(U8 *buf, U8 len);
int sd30xx_set_charge(U8 on);
int sd30xx_get_volt(F32 *volt);
int sd30xx_first(void);

int sd30xx_read(U8 reg, U8 *buf, U8 len);

#endif
