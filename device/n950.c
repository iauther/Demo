#include "drv/uart.h"
#include "drv/delay.h"
#include "drv/pwm.h"
#include "data.h"
#include "n950.h"
#include "log.h"
#include "msg.h"
#include "myCfg.h"

//KNF泵  最大负压: 2mbar=2*100pa=0.2kpa


#define PWM_FREQ            100
#define N950_BAUDRATE       57600

handle_t uartHandle=NULL;
handle_t pwmHandle=NULL;
static U16 rx_length=0;
static U16 n950_speed=N950_SPEED_MAX;
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

#define N950_BUFLEN     30
static U8 n950_tmp_buf[N950_BUFLEN];
static U8 n950_rx_buf[N950_BUFLEN];
static U8 n950_tx_buf[N950_BUFLEN];
static int uart_send_cmd(U8 cmd, U32 speed)
{
    int r;
    U8  tmp[30];
    
    if(cmd==CMD_SET_SPEED) {
        sprintf((char*)tmp, "%s%d\r", cmd_str[cmd], speed);
    }
    else {
        sprintf((char*)tmp, "%s\r", cmd_str[cmd]);
    }
    
    rx_length = 0;
    r = uart_write(uartHandle, (U8*)tmp, strlen((char*)tmp));
    if(r==0) {
        if(cmd==CMD_SET_START) {
            delay_ms(30);
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
                    .pwm_pin  = GPIO_PUMP_PWM_PIN,            //set speed
                    .type = TYPE_OC,
                    .mode = MODE_PP,
                },
                
                {
                    .type = TYPE_NO,
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

static int pwm_set_speed(U32 speed)
{
    int r;
    F32 dr;
    
    //dr = 2.1/5.0;
    dr = DRATIO_OF(speed);
    r = pwm_set(pwmHandle, GPIO_PUMP_PWM_PIN, PWM_FREQ, dr);
    
    return r;
}


static int pwm_send_cmd(U8 cmd, U32 speed)
{
    switch(cmd) {
        case CMD_SET_START:
        case CMD_SET_SPEED:
        pwm_set_speed(speed);
        break;
        
        case CMD_SET_STOP:
        pwm_set_speed(0);
        break;
        
        default:
        break;
    }
    
    return 0;
}

static int send_cmd(U8 cmd, U32 speed)
{
#ifdef PUMP_USE_UART
    return uart_send_cmd(cmd, speed);
#else
    return pwm_send_cmd(cmd, speed);
#endif
}

static int uart_rx_wait(int ms)
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
    if(len<=N950_BUFLEN) {
        memcpy(n950_tmp_buf, data, len);
    }
}
int n950_init(void)
{
    uart_cfg_t uc;

    uc.mode = MODE_DMA;
    uc.port = PUMP_UART_PORT;
    uc.baudrate = N950_BAUDRATE;
    uc.para.rx  = rx_callback;
    uc.para.buf = n950_rx_buf;
    uc.para.blen = sizeof(n950_rx_buf);
    uc.para.dlen = 0;
    uartHandle = uart_init(&uc);
    
    pwmHandle = pwm_init(&cfg);
    
    //n950_test();
    
    return 0;
}


int n950_deinit(void)
{
    uart_deinit(&uartHandle);
    pwm_deinit(&pwmHandle);
    
    return 0;
}

int n950_start(void)
{
    return send_cmd(CMD_PUMP_SPEED, n950_speed);
}


int n950_stop(void)
{
    return send_cmd(CMD_PUMP_SPEED, 0);
}



int n950_set_speed(U32 speed)
{   
    if(((speed>0) && (speed<N950_SPEED_MIN)) || speed>N950_SPEED_MAX) {
        return -2;
    }
    
    return send_cmd(CMD_PUMP_SPEED, speed);
}


int n950_send_cmd(U8 cmd, U32 speed)
{
    U8 c=cmd;
    U32 sp=speed;
    
    if(((speed>0) && (speed<N950_SPEED_MIN)) || speed>N950_SPEED_MAX) {
        return 1;
    }
    
    if(cmd==CMD_PUMP_START || cmd==CMD_PUMP_STOP) {
        c = CMD_PUMP_SPEED;
        sp = (cmd==CMD_PUMP_START)?n950_speed:0;
    }
    
    return send_cmd(c, sp);
}



int n950_get(n950_stat_t *st)
{
    int r;
    
    r = uart_send_cmd(CMD_GET_STAT, 0);
    uart_rx_wait(20);
    //rx_buf[0~5]       min speed;       r/min
    //rx_buf[6~11]      operate current; mA
    //rx_buf[12~19]     temperature of controller; 
    //rx_buf[20~24]     fault status;
    //rx_buf[25]        0: command not complete  1: command complete  ?: command unclear
    //rx_buf[26]        0x0d: end sympol
#if 1
    n950_tmp_buf[5]=n950_tmp_buf[11]=n950_tmp_buf[19]=n950_tmp_buf[24]=0;
    st->speed = atoi((char*)n950_tmp_buf);
    st->current = atof((char*)(n950_tmp_buf+6));
    st->temp = atof((char*)(n950_tmp_buf+12));
    st->fault = atoi((char*)(n950_tmp_buf+20));
    st->cmdAck = (n950_tmp_buf[25]=='?')?2:n950_tmp_buf[25];
#endif
    
    return r;
}


void n950_test(void)
{
    int r, cnt=0;
    U8 tmp[50];
    
    //set_speed(100);
    while(1) {
        sprintf((char*)tmp, "______test cnt: %d\r\n", cnt++);
        r = uart_write(uartHandle, tmp, strlen((char*)tmp));
        //LOGD("______test cnt: %d\r\n", cnt++);
        delay_ms(100);
    }
}


