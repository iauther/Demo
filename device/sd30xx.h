#ifndef __SD30XX_Hx__
#define __SD30XX_Hx__

#include "types.h"
#include "cfg.h"


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
    IRQ_NONE,
}SD_IRQ;


typedef struct {
    u8  year;
    u8  month;
    u8  day;
    u8  week;
    u8  hour;
    u8  minute;
    u8  second;
}sd_time_t;


typedef struct {
    SD_CLK  clk;
    u8      im;         //im=1:周期性中断
    u32     val;
}sd_countdown_t;


typedef struct {
    SD_IRQ  irq;        //开机中断标志
    s8      first;
}sd_info_t;


#ifdef CALL_IN_SVC
    #define SVC(x) __svc(x)
#else
    #define SVC(x)
#endif


int sd30xx_init(void) SVC(1);
int sd30xx_deinit(void) SVC(2);
int sd30xx_write_time(sd_time_t *tm) SVC(3);
int sd30xx_read_time(sd_time_t *tm) SVC(4);
int sd30xx_set_countdown(sd_countdown_t *cd) SVC(5);
int sd30xx_set_alarm(u8 en, sd_time_t *tm) SVC(6);
int sd30xx_set_freq(SD_FREQ freq) SVC(7);
int sd30xx_set_irq(SD_IRQ irq, u8 on) SVC(8);
int sd30xx_clr_irq(void) SVC(9);
int sd30xx_read_id(U8 *buf, U8 len) SVC(10);
int sd30xx_set_charge(U8 on) SVC(11);
int sd30xx_get_volt(F32 *volt) SVC(12);
int sd30xx_get_info(sd_info_t *info) SVC(13);
int sd30xx_read(U8 reg, U8 *buf, U8 len) SVC(14);

void sd30xx_test(void);
#endif
