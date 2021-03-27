#ifndef __N950_Hx__
#define __N950_Hx__

#include "types.h"

/*
    baudrate:       57600
    data bits:      8
    parity:         none
    stop bits:      1
    flow control:   none
*/
#define N950_SPEED_MIN           500         //r/min
#define N950_SPEED_MAX           1500        //r/min

#define N950_VOLTAGE_MIN         100         //mV
#define N950_VOLTAGE_MAX         5000        //mV

enum {
    CMD_SET_START=0,      //delay 25ms or more
    CMD_SET_STOP,
    CMD_SET_SPEED,
    
    CMD_GET_STAT,
    CMD_GET_FAULT,
    CMD_GET_SPEED,
    CMD_GET_SPEED_MIN,
    CMD_GET_SPEED_MAX,
    CMD_GET_VERSION,
    
    CMD_MAX
};

typedef void (*rx_func_t)(U8 *data, U16 data_len);

#pragma pack (1)

typedef struct {
    U16 speed;
    F32 current;
    F32 temp;
    U8  cmdAck;    //0: command not complete  1: command complete  0x3f: command unclear
    U16 fault;  
}n950_stat_t;

#pragma pack ()

int n950_init(void);
int n950_send_cmd(U8 cmd, U32 speed);
int n950_get(n950_stat_t *st);
void n950_test(void);

#endif



