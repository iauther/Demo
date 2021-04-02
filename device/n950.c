#include "drv/uart.h"
#include "drv/delay.h"
#include "drv/pwm.h"
#include "n950.h"
#include "log.h"
#include "msg.h"

//KNF泵

//#define N950_USE_UART
#define PWM_FREQ            100
#define N950_PORT           UART_1
#define N950_BAUDRATE       57600

handle_t uartHandle=NULL;
handle_t pwmHandle=NULL;
static U16 rx_length=0;
static U16 n950_speed=0;
char *cmd_str[CMD_MAX] = {
    "dB",
    "dE",
    "dS",
    "pP",
    "gP",
    "gS",
    "gSI",
    "gSh",
    "iV",
};
U8 tx_buf[30],rx_buf[30];
static int uart_send_cmd(U8 cmd, U32 speed)
{
    int r;
    
    if(cmd==CMD_SET_SPEED) {
        sprintf((char*)tx_buf, "%s%d\r", cmd_str[cmd], speed);
    }
    else {
        sprintf((char*)tx_buf, "%s\r", cmd_str[cmd]);
    }
    
    rx_length = 0;
    r = uart_write(uartHandle, (U8*)tx_buf, strlen((char*)tx_buf));
    if(r==0) {
        if(cmd==CMD_SET_START) {
            delay_ms(30);
        }
        else if(cmd==CMD_SET_SPEED){
            n950_speed = speed;
        }
    }
    
    return r;
}

//////////////////////////////////////////////////////////////
//PIN1， 黑线， 输出：电源，5-0.2V~5+0.2V  170mA    最大5.5V

//PIN2， 灰线， 输XX：地
//PIN3， 白线， 输入：速度信号输入，0.1~5V  pwm freq 50~150Hz，低于0.1V泵将关闭

//PIN4， 蓝线， 输入：地
//PIN5， 绿线， 输入：开关控制，ON: <=0.9V   OFF：>=4.2V

//PIN6， 紫线， 输出：错误信号，错误0：<=0.6V  错误1：>=4.5V

//PIN7， 橙色， 输XX：地
//PIN8， 黄色， 输出：速度信号输出，pwm, 50HZ

//PIN9， 棕色：开关地
//PIN10，红色：开关电   24V   最大500mA

#define VOLTAGE_OF(speed)   (((speed)-N950_SPEED_MIN)*(N950_VOLTAGE_MAX-N950_VOLTAGE_MIN)/(N950_SPEED_MAX-N950_SPEED_MIN)+N950_VOLTAGE_MIN)
#define DRATIO_OF(speed)    ((speed)?(VOLTAGE_OF(speed)/5000.0F):0)

static void pwm_ic_callback_fn(handle_t h, U8 pwm_pin, U32 freq, F32 dr);
static pwm_cfg_t cfg={
        {
            {//pwm[]
                {
                    .pwm_pin  = PWM_PIN_A0,            //set speed
                    .type = TYPE_OC,
                    .mode = MODE_OD,
                },
                
                {
                    .pwm_pin  = PWM_PIN_A1,            //get speed
                    .type = TYPE_NO,
                    .mode = MODE_OD,
                },
                
                {
                    .type = TYPE_NO,
                },
                
                {
                    .type = TYPE_NO,
                },
            },
        
            .freq  = PWM_FREQ, .dr = 0,
        },
        
        .useDMA = 0,
        .callback = pwm_ic_callback_fn,
};


static void pwm_ic_callback_fn(handle_t h, U8 pwm_pin, U32 freq, F32 dr)
{
    
}

static int set_speed(U32 speed)
{
    int r;
    F32 dr;
    if(speed>N950_SPEED_MAX || (speed>0 && speed<N950_SPEED_MIN)) {
        return -1;
    }
    
    //dr = 2.1/5.0;
    dr = DRATIO_OF(speed);
    r = pwm_set(pwmHandle, PWM_PIN_A0, PWM_FREQ, dr);
    n950_speed = speed;
    
    return r;
}


static int pwm_send_cmd(U8 cmd, U32 speed)
{
    switch(cmd) {
        case CMD_SET_START:
        set_speed(n950_speed);
        break;
        
        case CMD_SET_STOP:
        set_speed(0);
        break;
       
        case CMD_SET_SPEED:
        set_speed(speed);
        break;
        
        default:
        break;
    }
    
    return 0;
}

static int send_cmd(U8 cmd, U32 speed)
{
#ifdef N950_USE_UART
    return uart_send_cmd(cmd, speed);
#else
    return pwm_send_cmd(cmd, speed);
#endif
}

static int uart_wait(int ms)
{
    int n=1;
    while(!rx_length) {
        if(ms>0 && n++<ms) {
            break;
        }
        
        delay_ms(1);
    }
    return (n==ms)?-1:n;
}
static void rx_callback(U8 *data, U16 len)
{
    rx_length = len;
}
int n950_init(void)
{
    uart_cfg_t uc;

    uc.mode = MODE_DMA;
    uc.port = N950_PORT;
    uc.baudrate = N950_BAUDRATE;
    uc.para.rx  = rx_callback;
    uc.para.buf = rx_buf;
    uc.para.blen = sizeof(rx_buf);
    uc.para.dlen = 0;
    uartHandle = uart_init(&uc);
    
    pwmHandle = pwm_init(&cfg);
    //n950_start();
    
    return 0;
}



int n950_send_cmd(U8 cmd, U32 speed)
{
    return send_cmd(cmd, speed);
}



int n950_get(n950_stat_t *st)
{
    int r;
    
    r = uart_send_cmd(CMD_GET_STAT, 0);
    uart_wait(20);
    //rx_buf[0~5]       min speed;       r/min
    //rx_buf[6~11]      operate current; mA
    //rx_buf[12~19]     temperature of controller; 
    //rx_buf[20~24]     fault status;
    //rx_buf[25]        0: command not complete  1: command complete  ?: command unclear
    //rx_buf[26]        0x0d: end sympol
#if 1
    rx_buf[5]=rx_buf[11]=rx_buf[19]=rx_buf[24]=0;
    st->speed = atoi((char*)rx_buf);
    st->current = atof((char*)(rx_buf+6));
    st->temp = atof((char*)(rx_buf+12));
    st->fault = atoi((char*)(rx_buf+20));
    st->cmdAck = (rx_buf[25]=='?')?2:rx_buf[25];
#endif
    
    return r;
}


void n950_test(void)
{
    //U8 tmp[50];
    
    //set_speed(100);
    while(1) {
        //sprintf((char*)tmp, "______test cnt: %d\r\n", cnt++);
        //r = uart_write(uartHandle, tmp, strlen((char*)tmp));
        //LOGD("______test cnt: %d\r\n", cnt++);
        //delay_ms(500);
    }
}


