#include <stdio.h>
#include <string.h>
#include "cmsis_os2.h"
#include "aiot_at_api.h"
#include "os_net_al.h"
#include "dal_uart.h"
#include "dal_gpio.h"
#include "dal_delay.h"
#include "log.h"
#include "hw_at_impl.h"


extern aiot_os_al_t g_aiot_os_api;
extern aiot_net_al_t g_aiot_net_at_api;

/*AT module*/
extern at_device_t ecxxx_at_cmd;
extern at_device_t ecxxx_at_cmd_ssl;
at_device_t *device = &ecxxx_at_cmd;

/*linkkit process tast id*/
osThreadId_t link_process_id;
#define RING_BUFFER_SIZE            (AIOT_AT_DATA_RB_SIZE_DEFAULT)

typedef struct {
    handle_t hurt;
    handle_t hpwr;
    handle_t hdtr;
    
    int      inited;
}at_handle_t;

typedef struct {
    uint8_t  data[RING_BUFFER_SIZE];
    uint16_t tail;
    uint16_t head;
} uart_ring_buffer_t;
static uart_ring_buffer_t   uart_dma_rx_buf = {0};
static int at_uart_recv_len=0;
static at_handle_t atHandle={NULL};
static int32_t at_uart_send(uint8_t *p_data, uint16_t len, uint32_t timeout);

static int is_powered(void)
{
    char tmp[20];
    
    sprintf(tmp, "AT+QPOWD=?\r\n");
    
    at_uart_send((U8*)tmp, strlen(tmp), 0);
    at_uart_recv_len = 0;
    
    dal_delay_ms(5);
    if(at_uart_recv_len) {
        return 1;
    }
    
    return 0;
}
static int at_power(int on)
{
    int pwred;
    
    if(!atHandle.hpwr) {
        //LOGE("___ pwr pin not inited!\n");
        return -1;
    }
    
    pwred = is_powered();
    if(on==pwred) {
        LOGD("___ at module is powered %s\n", on?"on":"off");
        return 0;
    }
    
    if(on) {
        dal_gpio_set_hl(atHandle.hpwr, 0);
        dal_delay_ms(500);
        dal_gpio_set_hl(atHandle.hpwr, 1);
    }
    else {
        dal_gpio_set_hl(atHandle.hpwr, 0);
        dal_delay_ms(650);
        dal_gpio_set_hl(atHandle.hpwr, 1);
    }
    
    return 0;
}

static int32_t at_uart_send(uint8_t *p_data, uint16_t len, uint32_t timeout)
{
    int r;
    
    //LOGD("##:%s", (char*)p_data);
    r = dal_uart_write(atHandle.hurt, p_data, len);
    return (r==0)?len:0;
}


static int at_recv_callback(handle_t h, void *addr, U32 evt, void *data, int len)
{
    //LOGD("$$:%s", (char*)data);
    at_uart_recv_len = len;
    aiot_at_hal_recv_handle((uint8_t*)data, len);
    return 0;
}


int at_hal_init(void)
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
        uc.para.callback = at_recv_callback;
        
        atHandle.hurt = dal_uart_init(&uc);
    }
    
#if 1
    if(!atHandle.hpwr) {
        gc.gpio.grp = GPIOB;
        gc.gpio.pin = GPIO_PIN_12;
        gc.mode = MODE_OUT_PP;
        gc.pull = PULL_NONE;
        gc.hl = 1;
        atHandle.hpwr = dal_gpio_init(&gc);
   }
    
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
    
    at_power(1);
    
    /*初始化模组及获取到IP网络*/
    LOGD("___ aiot_at_bootstrap 000\n");
    r = aiot_at_bootstrap();
    LOGD("___ aiot_at_bootstrap 111\n");
    if (r<0) {
        LOGE("aiot_at_bootstrap failed\r\n");
        return -1;
    }
    
    atHandle.inited = 1;
    
    return 0;
}


int at_hal_power(int on)
{
    return at_power(on);
}


int at_hal_reset(void)
{
    at_power(0);
    dal_delay_ms(50);
    at_power(1);
    
    return 0;
}
