#if (MQTT_LIB==1)

#include <stdio.h>
#include <string.h>
#include "hal_at_impl.h"
#include "os_net_interface.h"
#include "cmsis_os2.h"
#include "dal_uart.h"
#include "dal_gpio.h"
#include "dal_delay.h"
#include "task.h"
#include "lock.h"
#include "mem.h"
#include "log.h"
#include "cfg.h"

#define BAUDRATE       (115200*4)
#define USE_ECXX
#define TEMP_BUF_LEN    100


#ifdef BOARD_V134
#define POWER_PORT  GPIOB
#define POWER_PIN   GPIO_PIN_12
#elif defined BOARD_V136
#define POWER_PORT  GPIOB
#define POWER_PIN   GPIO_PIN_1
#endif

extern aiot_os_al_t g_aiot_os_api;
extern aiot_net_al_t g_aiot_net_at_api;

/*AT module*/
#ifdef USE_ECXX
extern at_device_t ecxxx_at_cmd;
extern at_device_t ecxxx_at_cmd_ssl;
at_device_t *device = &ecxxx_at_cmd;
#else
extern at_device_t air724_at_cmd;
at_device_t *device = &air724_at_cmd;
#endif

/*linkkit process tast id*/
osThreadId_t link_process_id;
#define RING_BUFFER_SIZE            (AIOT_AT_DATA_RB_SIZE_DEFAULT)


typedef struct {
    handle_t hurt;
    handle_t hpwr;
    handle_t hdtr;
    
    U8       buf[TEMP_BUF_LEN];
    buf_t    tmp;
    
    handle_t lck;
    int      inited;
}at_handle_t;

typedef struct {
    uint8_t  data[RING_BUFFER_SIZE];
    uint16_t tail;
    uint16_t head;
} uart_ring_buffer_t;
static uart_ring_buffer_t   uart_dma_rx_buf = {0};

static at_handle_t atHandle={
    .hurt = NULL,
    .hpwr = NULL,
    .hdtr = NULL,
    .tmp  = {NULL,0,0},
    .lck  = NULL,
    .inited = 0,
};
static int32_t at_uart_send(const uint8_t *data, uint16_t len, uint32_t timeout);
static int at_recv_callback(handle_t h, void *addr, U32 evt, void *data, int len);



static void set_temp_buf(U8 flag)
{
    if(flag) {
        atHandle.tmp.buf = atHandle.buf;
        atHandle.tmp.blen = sizeof(atHandle.buf);
        atHandle.tmp.dlen = 0;
        
        atHandle.tmp.buf[0] = 0;
    }
    else {
        atHandle.tmp.buf = NULL;
        atHandle.tmp.blen = 0;
        atHandle.tmp.dlen = 0;
    }
}
static void fill_temp_buf(void *data, int len)
{
    buf_t *pb=&atHandle.tmp;
    
    if(pb->buf && (pb->blen>0) && ((pb->dlen+len)<(pb->blen-1))) {
        memcpy(pb->buf+pb->dlen, data, len);
        pb->dlen += len;
        pb->buf[pb->dlen] = 0;
    }
}
static int send_cmd(char *cmd, char *resp, U32 ms)
{
    int r=-2;
    char tmp[100];
    U32 time_ms=ms;
    buf_t *pb=&atHandle.tmp;
    
    set_temp_buf(1);
    sprintf(tmp, "%s\r\n", cmd);
    at_uart_send((U8*)tmp, strlen(tmp), 0);
    while(time_ms--) {
        if(pb->dlen) {
            if(strstr((char*)pb->buf, "ERROR")) {
                LOGD("___ %s failed, cost time %d\n", cmd, ms-time_ms);
                r = -1; break;
            }
            
            if(strstr((char*)pb->buf, resp)) {
                LOGD("___ %s success, cost time %d\n", cmd, ms-time_ms);
                r = 0; break;
            }
        }
        
        dal_delay_ms(1);
    }
    
    if(r==-2) {
        LOGD("___ %s timeout, cost time %d\n", cmd, ms);
    }
    
    return r;
}

static int is_powered(void)
{
    char tmp[20];
    int  r,powered=0;
    
    r = send_cmd("AT\r\n", "OK", 50);
    if(r==0) {
        powered = 1;
    }
    set_temp_buf(0);
    
    return powered;
}
static int set_power(int on)
{
    int pwred;
    
    if(!atHandle.hpwr) {
        //LOGE("___ pwr pin not inited!\n");
        return -1;
    }
    
    pwred = is_powered();
    LOGD("___ at module is %s, now switch to %s\n", pwred?"on":"off", on?"on":"off");
    
    if(on==pwred) return 1;
    
    if(on) {
        dal_gpio_set_hl(atHandle.hpwr, 0);
        dal_delay_ms(600);
        dal_gpio_set_hl(atHandle.hpwr, 1);
    }
    else {
        dal_gpio_set_hl(atHandle.hpwr, 0);
        dal_delay_ms(700);
        dal_gpio_set_hl(atHandle.hpwr, 1);
    }
    dal_delay_ms(2000);
    
    return 0;
}

////////////////////////////////////////////////////////

static int32_t at_uart_send(const uint8_t *data, uint16_t len, uint32_t timeout)
{
    int r;
    
    //printf(">>>>>%d\n", len);
    //printf("[%d]>> %s", len,(char*)data);
    //printf("at_uart_send: %d\n", len);
    lock_on(atHandle.lck);
    r = dal_uart_write(atHandle.hurt, (U8*)data, len);
    lock_off(atHandle.lck);
    //dal_delay_us(50);
    
    return (r==0)?len:0;
}
static int at_recv_callback(handle_t h, void *addr, U32 evt, void *data, int len)
{
    //printf("$$:%s\n", (char*)data);
    //printf("at_uart_recv: %d\n", len);
    
    fill_temp_buf(data, len);
    aiot_at_hal_recv_handle(data, len);
    
    return 0;
}


static void set_baudrate(int br)
{
    int  r;
    char tmp[60];
    
    sprintf(tmp, "AT+IPR=%d\r\n", br);        //过大，则发较长数据会导致超时无回应，过小则耗时太长
    r = send_cmd(tmp, "OK",  300);
    if(r==0) {
        dal_uart_set_baudrate(atHandle.hurt, br);
    }
}

int hal_at_init(void)
{
    dal_uart_cfg_t uc;
    dal_gpio_cfg_t gc;
    
    if(atHandle.inited) {
        return 0;
    }
    
    if(!atHandle.hurt) {
        uc.baudrate = 115200;
        uc.mode = MODE_IT;
        uc.port = UART_1;
        uc.msb  = 0;
        uc.callback = at_recv_callback;
        uc.rx.blen =  16*KB;
        uc.rx.buf = eMalloc(uc.rx.blen);
        uc.rx.dlen = 0;
        
        atHandle.hurt = dal_uart_init(&uc);
    }
    
    if(!atHandle.hpwr) {
        gc.gpio.grp = POWER_PORT;
        gc.gpio.pin = POWER_PIN;
        gc.mode = MODE_OUT_PP;
        gc.pull = PULL_NONE;
        gc.hl = 1;
        atHandle.hpwr = dal_gpio_init(&gc);
    }
    
    if(!atHandle.lck) {
        atHandle.lck = lock_init();
    }

#if 0
    if(!atHandle.hdtr) {
        gc.gpio.grp = GPIOA;
        gc.gpio.pin = GPIO_PIN_7;
        gc.mode = MODE_OUT_OD;
        gc.pull = PULL_NONE;
        gc.hl = 0;
        atHandle.hdtr = dal_gpio_init(&gc);
    }
#endif

    /*设置设备系统接口及网络接口*/
    aiot_install_os_api(&g_aiot_os_api);
    aiot_install_net_api(&g_aiot_net_at_api);

    /*at_module_init*/
    int r = aiot_at_init();
    if (r < 0) {
        LOGE("aiot_at_init failed\r\n");
        return -1;
    }
        
    /*设置发送接口*/
    aiot_at_setopt(AIOT_ATOPT_UART_TX_FUNC, at_uart_send);
    /*设置模组*/
    aiot_at_setopt(AIOT_ATOPT_DEVICE, device);

    return 0;
}


int hal_at_boot(void)
{
    int r;
    
    set_baudrate(BAUDRATE);

    /*初始化模组及获取到IP网络*/
    r = aiot_at_bootstrap();
    if (r<0) {
        LOGE("aiot_at_bootstrap failed\r\n");
        return -1;
    }
    
    
    atHandle.inited = 1;
    
    return 0;
}


#include "rtc.h"
#include "datetime.h"
int hal_at_ntp(void)
{
    int r;
    U64 ts,ts2;
    datetime_t dt,dt2;
    buf_t *pb=&atHandle.tmp;
    
    LOGD("___ at_hal_ntp\n");
    
    r = set_power(1);
    if(r==0) {
        dal_delay_ms(12000);
    }
    
    send_cmd("ATE0\r\n", "OK", 300);
    
    send_cmd("AT+QICSGP=1,1,\"CMNET\",\"\",\"\",1\r\n", "OK", 5000);
    
    send_cmd("AT+QIDEACT=1\r\n", "OK", 5000);
    
    send_cmd("AT+QIACT=1\r\n", "OK", 5000);
    
    r = send_cmd("AT+QNTP=1,\"ntp.aliyun.com\",123\r\n", "+QNTP", 5000);
    if(r==0) {
        U8 x,cc=0,zz=0;
        char *ntp=strstr((char*)pb->buf, "+QNTP:");
        r = sscanf(ntp, "+QNTP: %hhu,\"%hu/%hhu/%hhu,%hhu:%hhu:%hhu%c%hhu\"", &x, &dt.date.year, &dt.date.mon, &dt.date.day, &dt.time.hour, &dt.time.min, &dt.time.sec, &cc, &zz);
        if(r==9) {
            int plus;
            tm_to_ts(&dt, &ts);
            plus = zz*15*60*1000;
            if(cc=='-') plus *= -1;
            ts2 = ts+plus;
            ts_to_tm(ts2, &dt2);
            
            rtc2_set_time(&dt2);
        }
    }
    set_temp_buf(0);
    
    return 0;
}

int hal_at_power(int on)
{
    return set_power(on);
}


int hal_at_reset(void)
{
    set_power(0);
    
    set_baudrate(115200);
    set_power(1);
    set_baudrate(BAUDRATE);
    
    return 0;
}
#endif



