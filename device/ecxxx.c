#include "ecxxx.h"
#include "dal_uart.h"
#include "dal_gpio.h"
#include "dal_delay.h"
#include "rtc.h"
#include "date.h"
#include "log.h"


#define RX_LEN      (1024*2)
#define TX_LEN      (1024*2)

#define EC_TOPIC    "/h471xrlbuE9/gasLeap/user/XXX"


typedef struct {
    ecxxx_cfg_t cfg;
    handle_t    hurt;
    
    handle_t    hpwr;
    handle_t    hdtr;
        
    U8          txBuf[TX_LEN+4];
    int         txLen;
    
    U8          rxBuf[RX_LEN+4];
    int         rxLen;
    
    CONTEXT_ID  contID;
    CONNECT_ID  connID;
}ecxxx_handle_t;

#define ASC2INT(x) (((x)>='A')?(0x41+(x)-'A'):(0x30+(x)-'0'))
#define INT2ASC(x) (((x)<10)?('0'+(x)):('A'+((x)-9)))

ecxxx_handle_t* exxxHandle=NULL;
static int ec_itoa(U8 *src, int srclen, U8 *dst, int dstlen)
{
    int i,j=0;
    
    for(i=0; i<srclen; i++) {
        dst[j++] = INT2ASC(src[i]>>4);
        dst[j++] = INT2ASC(src[i]&0x0f);
    }
    dst[j++] = '\r';
    dst[j++] = '\n';
    
    return j;
}
    
static int ec_atoi(U8 *data, int len, U8 *xdata, int xlen)
{
    int i,j=0;
    U8 a,b;
    
    if(xlen<len/2) {
        return -1;
    }
    
    for(i=0; i<len; i+=2) {
        xdata[j++] = (ASC2INT(data[i])<<4) | ASC2INT(data[i+1]);
    }
    
    return j;
}

static int ec_recv_callback(handle_t h, void *addr, U32 evt, void *data, int len)
{
    int xlen;
    U8 *pdata=(U8*)data;
    ecxxx_handle_t* eh=exxxHandle;
    
    if(eh) {
        if(eh->rxLen+len<=RX_LEN) {
            memcpy(eh->rxBuf+eh->rxLen, pdata, len);
            eh->rxLen += len;
            eh->rxBuf[eh->rxLen] = 0;
        }
        
        LOGD("$$:%s\n", eh->rxBuf);
        
        if(eh->cfg.callback) {
            eh->cfg.callback(eh->rxBuf, eh->rxLen);
        }
    }
    
    return 0;
}


#include <stdarg.h>
static int ec_find(void *buf, ...)
{
    int r=0;
	va_list args;
    const char *arg;
    
	va_start(args, buf);//
    while(1) {
        arg = va_arg(args, const char *);
        if (arg == NULL)
            break;
        
        if(strstr((char*)buf, arg)) {
            r = 1;
            break;
        }
    }
	va_end(args);
    
    return r;
}


static int ec_search(char *buf, char *dst)
{
    int r=0,i;
    char* str;
    const char *delim="/";
    
	str = strstr(dst, delim);
    if(str) {
        char *tmp=(char*)malloc(strlen(dst)+1);
        if(!tmp) {
            return 0;
        }
        
        strcpy(tmp, dst);
        str = strtok(tmp, delim);
        while (str) {
            if(strstr(buf, str)) {
                r = 1;
                break;
            }
            str = strtok(NULL, delim);
        }
        
        free(tmp);
    }
    else {
        if(strstr(buf, dst)) {
            r = 1;
        }
    }
    
    return r;
}


/*
   返回0： 成功
   返回-1：成功
   返回-2：成功
   返回-3：成功
*/
static int rx_wait(ecxxx_handle_t *h, int ms, char *ok, char *err)
{
    int r=0;
    U32 x,time=dal_get_tick_ms();
    U32 wait_ms = (ms==-1)?0xffffffff:ms;
    
    h->rxLen = 0;
    if(wait_ms>0) {
        while(1) {
            if(wait_ms--==0) {
                r = -3;  break;    //timeout
                
            }
            
            if(h->rxLen>0) {
                if(ec_search((char*)h->rxBuf, ok)) {
                    break;   //ok
                }
                
                if(ec_search((char*)h->rxBuf, err)) {
                    r = -2; break;  //error
                }
            }
            
            dal_delay_ms(1);
        }
    }
    x = dal_get_tick_ms()-time;
   // LOGD("wait time: %d ms\n", x);
    
    return r;
}
static int ec_write(void *data, int len, int timeout_ms, char *ok, char *err)
{
    int r;
    ecxxx_handle_t* eh=exxxHandle;
    
    dal_uart_write(eh->hurt, (U8*)data, len);
    r = rx_wait(eh, timeout_ms, ok, err);
    return r;
}




int ecxxx_init(ecxxx_cfg_t *cfg)
{
    ecxxx_handle_t *h=NULL;
    dal_uart_cfg_t uc;
    dal_gpio_cfg_t gc;
    
    if(!cfg) {
        return -1;
    }
    
    h = malloc(sizeof(ecxxx_handle_t));
    if(!h) {
        return -1;
    }
    
    h->cfg = *cfg;
    
    uc.baudrate = h->cfg.baudrate;
    uc.mode = MODE_IT;
    uc.port = h->cfg.port;
    uc.msb  = 0;
    uc.para.callback = ec_recv_callback;
    
    h->hurt = dal_uart_init(&uc);
    
    gc.gpio.grp = GPIOB;
    gc.gpio.pin = GPIO_PIN_12;
    gc.mode = MODE_OUT_OD;
    gc.pull = PULL_NONE;
    gc.hl = 1;
    h->hpwr = dal_gpio_init(&gc); //配置
    
/*    
    gc.gpio.grp = GPIOA;
    gc.gpio.pin = GPIO_PIN_7;
    gc.mode = MODE_OUT_PP;
    gc.pull = PULL_NONE;
    gc.hl = 0;
    h->hdtr = dal_gpio_init(&gc);
*/

    h->contID = CONTEXT_ID_1;
    h->connID = CONNECT_ID_0;

    exxxHandle = h;
    
    return 0;
}


int ecxxx_deinit(void)
{
    ecxxx_handle_t* eh=exxxHandle;
    
    if(!eh) {
        return -1;
    }
    
    dal_gpio_deinit(eh->hdtr);
    dal_gpio_deinit(eh->hpwr);
    dal_uart_init(eh->hurt);
    free(eh);
    
    return 0;
}


int ecxxx_write(void *data, int len, int timeout)
{
    ecxxx_handle_t* eh=exxxHandle;
    
    if(!eh || !data || !len) {
        return -1;
    }
    
    return ec_write(data, len, timeout, NULL, NULL);
}



int ecxxx_wait(U32 ms, char *ok, char *err)
{
    ecxxx_handle_t* eh=exxxHandle;
    
    return rx_wait(eh, ms, ok, err);
}


int ecxxx_stat(ecxxx_stat_t *st)
{
    int r;
    char tmp[100];
    
    sprintf(tmp, "AT+CSQ=?\r\n");
    r = ec_write(tmp, strlen(tmp), 5000, "OK", "ERROR");
    if(r) { LOGE("AT+CSQ failed\n"); return -1; }
    
    if(st) {
        //
    }
    
    return 0;
}


static U8 is_pwr_on(ecxxx_handle_t *h)
{
    U8 r=0;
    char tmp[100];
    int wait_ms=500;
    U32 x,time;
    
    sprintf(tmp, "AT+QPOWD=?\r\n");
    dal_uart_write(h->hurt, (U8*)tmp, strlen(tmp));
    
    time = dal_get_tick_ms();
    h->rxLen = 0;
    if(wait_ms>0) {
        while(1) {
            if(wait_ms--==0) {
                break;
            }
            
            if(h->rxLen>0) {
                r = 1;
                break;
            }
            
            dal_delay_ms(1);
        }
    }
    x = dal_get_tick_ms()-time;
    LOGD("___ x: %dms\n", x);
    
    return r;
}
int ecxxx_power(U8 on)
{
    int r=0,r1;
    char tmp[100];
    U8 pwr_on=0;
    ecxxx_handle_t* eh=exxxHandle;
    
    if(!eh) {
        return -1;
    }
    
    LOGD("ecxxx power is %s ...\n", on?"on":"down");
    
    pwr_on = is_pwr_on(eh);
    if(pwr_on!=on) {
        if(on) {        //power on
            dal_gpio_set_hl(eh->hpwr, 0);
            dal_delay_ms(600);
            dal_gpio_set_hl(eh->hpwr, 1);
            
            r = rx_wait(eh, 2000, "RDY", "ERROR");
            if(r<0) {
                LOGE("ecxxx power on failed!\n");
            }
        }
        else {          //power off
            dal_gpio_set_hl(eh->hpwr, 0);       //pull low power key
            dal_delay_ms(600);
            dal_gpio_set_hl(eh->hpwr, 1);
            
            r = rx_wait(eh, 2000, "POWERED DOWN", "ERROR");
            if(r) {
                LOGE("ecxxx power down failed!\n");
            }
            else {
                LOGD("ecxxx power down ok!\n");
            }
        }  
    }
    
    if(on && r==0) {
        LOGD("set ATE0 now ...\n");
                
        sprintf(tmp, "ATE0\r\n");
        r1 = ec_write(tmp, strlen(tmp), 500, "OK", "ERROR");
        if(r1<0) {
            LOGE("---ATE0--- fail!\n");
        }
    }
    
    return r;
}


int ecxxx_restart(void)
{
    int r,r1;
    char tmp[100];
    U8 pwr_on=0;
    ecxxx_handle_t* eh=exxxHandle;
    
    if(!eh) {
        return -1;
    }
    
    pwr_on = is_pwr_on(eh);
    if(pwr_on) {        //if already powered on, power down it
        dal_gpio_set_hl(eh->hpwr, 0);       //pull low power key
        dal_delay_ms(700);
        dal_gpio_set_hl(eh->hpwr, 1);
        
        r = rx_wait(eh, 2000, "POWERED DOWN", "ERROR");
        if(r) {
            LOGE("ecxxx power down failed!\n");
        }
        else {
            LOGD("ecxxx power down ok!\n");
        }
        
    }
    
    //power on it
    {
        dal_gpio_set_hl(eh->hpwr, 0);
        dal_delay_ms(1000);
        dal_gpio_set_hl(eh->hpwr, 1);
        
        r = rx_wait(eh, 2000, "RDY", "ERROR");
        if(r<0) {
            LOGE("ecxxx power on failed!\n");
        }
        else {
            LOGD("ecxxx power on ok, set ATE0 now ...\n");
            
            sprintf(tmp, "ATE0\r\n");
            r1 = ec_write(tmp, strlen(tmp), 500, "OK", "ERROR");
            if(r1<0) {
                LOGE("---ATE0--- fail!\n");
            }
        }
    }
    
    return r;
}


int ecxxx_ntp(char *server, U16 port)
{
    int r;
    char tmp[100];
    date_time_t dt;
    ecxxx_handle_t* eh=exxxHandle;
    
    if(!eh) {
        return -1;
    }
    
    sprintf(tmp, "AT+QNTP=%d,\"%s\",%d\r\n", eh->contID, server, port);
    r = ec_write(tmp, strlen(tmp), 5000, "1/5", "ERROR");
    if(r) { LOGE("---QNTP--- fail\n"); return -1; }
    
    r = get_datetime((char*)eh->rxBuf, &dt);
    if(r==0) {
        rtcx_write_time(&dt);
    }
    
    return 0;
}


int ecxxx_reg(APN_ID apnID)
{
    int i,j,r;
    char tmp[100];
    ecxxx_handle_t* eh=exxxHandle;
    apn_info_t apnInfo[APN_ID_MAX]={
                {"CMNET",  1},
                {"UNINET", 1},
                {"CTNET",  1},};

    if(!eh) {
        return -1;
    }
                
#if 0
    sprintf(tmp, "AT+CPIN?\r\n");
    r = ec_write(tmp, strlen(tmp), 1000, "READY", "ERROR");
    if(r) {
        LOGE("---CPIN--- fail\n");
        return r;
    }
    LOGD("---CPIN--- ok!\n");


    sprintf(tmp, "AT+CEREG?\r\n");
    r = ec_write(tmp, strlen(tmp), 5000, "1/5", "ERROR");
    if(r) { LOGE("---CEREG--- fail\n"); return -1; }
    LOGD("---CEREG--- ok!\n");
    
    sprintf(tmp, "AT+QENG=\"SERVINGCELL\"\r\n");
    r = ec_write(tmp, strlen(tmp), 5000, "OK", "ERROR");
    if(r) { LOGE("---QENG--- fail\n"); return -1; }
    LOGD("---QENG--- ok!\n");
#endif

#if 1
    sprintf(tmp, "AT+QICSGP=%d,3,\"%s\",\"\",\"\",%d\r\n", eh->contID, apnInfo[apnID].apn, apnInfo[apnID].auth);
    r = ec_write(tmp, strlen(tmp), 5000, "OK", "ERROR");
    if(r) { LOGE("---QICSGP fail\n"); return -1; }
    LOGD("---QICSGP--- ok!\n");
    
    
    sprintf(tmp, "AT+QIDEACT=%d\r\n", eh->contID);
    r = ec_write(tmp, strlen(tmp), 5000, "OK", "ERROR");
    if(r) { LOGE("---QIDEACT fail\n"); return -1; }

    
    sprintf(tmp, "AT+QIACT=%d\r\n", eh->contID);
    r = ec_write(tmp, strlen(tmp), 5000, "OK", "ERROR");
    if(r) { LOGE("---QIACT fail\n"); return -1; }
    LOGD("---QIACT--- ok!\n");
#endif
    
    return 0;
}
//////////////////////////////////////////////////////////////////
static int ec_tcp_conn(ecxxx_tcp_t *tcp)
{
    int i,j,r;
    char tmp[100];
    ecxxx_handle_t* eh=exxxHandle;
    
    sprintf(tmp, "AT+QIOPEN=%d,%d,\"TCP\",\"%s\",%d,0,2\r\n", eh->contID, eh->connID, tcp->ip, tcp->port);
    for(i=0; i<5; i++) {
        r = ec_write(tmp, strlen(tmp), 5000, "CONNECT", "ERROR");
        if(r==0) {
            LOGD("---QIOPEN--- ok!\n");
            break;
        }
        LOGE("---QIOPEN--- fail %d\n", i+1);
    }
    if(r) return r;
    
    return 0;
}


static int ec_tcp_mode_switch(ecxxx_handle_t *h, U8 mode)
{
    int r;
    char tmp[20];
    
    if(mode==0) {   //command mode
        
        dal_gpio_set_hl(h->hdtr, 0);
        dal_delay_ms(100);
        dal_gpio_set_hl(h->hdtr, 1);
        
        sprintf(tmp, "AT&D2\r\n");
        r = ec_write(tmp, strlen(tmp), 500, "OK", "ERROR");
        if(r) { LOGE("AT&D failed\n"); return -1; }
    }
    else {          //data mode
        sprintf(tmp, "ATO0\r\n");
        r = ec_write(tmp, strlen(tmp), 500, "OK", "ERROR");
        if(r) { LOGE("ATO0 failed\n"); return -1; }
    }
    
    return r;
}


static int ec_tcp_disconn(void)
{
    int r;
    char tmp[100];
    ecxxx_handle_t* eh=exxxHandle;
    
    if(!eh) {
        return -1;
    }

#if 0
    dal_delay_ms(1500);
    
    sprintf(tmp, "+++");
    r = ec_write(tmp, strlen(tmp), 5000, "OK", "ERROR");
    if(r) { LOGE("disconn 1 failed\n"); return -1; }
    
    dal_delay_ms(1500);
#else
    r = ec_tcp_mode_switch(eh, 0);
    if(r) { LOGE("tcp mode switch failed\n"); return -1; }
#endif
    
    sprintf(tmp, "AT+QICLOSE=0\r\n");
    r = ec_write(tmp, strlen(tmp), 5000, "OK", "ERROR");
    if(r) { LOGE("QICLOSE failed\n"); return -1; }
    
    LOGD("disconn ok!\n");
    
    return 0;
}


static int ec_tcp_send(void *data, int len, int timeout)
{
    U8 *pdata;
    int xlen,r=0;
    int i,times,remain,onceMaxLen=1024;
    ecxxx_handle_t* eh=exxxHandle;
    
    if(!eh || !data || !len) {
        return -1;
    }
    
    times = len/onceMaxLen;
    remain = len%onceMaxLen;
    for(i=0; i<times; i++) {
        pdata = ((U8*)data)+i*onceMaxLen;
        ec_write(pdata, onceMaxLen, timeout, "OK", "ERROR");
    }
    
    if(remain>0) {
        pdata = ((U8*)data)+len-remain;
        ec_write(pdata, remain, timeout, "OK", "ERROR");
    }
    
    return r;
}

//////////////////////////////////////////////////////////////////////////////////

static ecxxx_mqtt_t ecMqtt={0, 0, 0};
static int ec_mqtt_conn()
{
    int r;
    char tmp[200];
    ecxxx_handle_t* eh=exxxHandle;
    ecxxx_mqtt_t *mq=&ecMqtt;
    
    if(!eh) {
        return -1;
    }
    
    //sprintf(tmp, "AT+QMTCFG=\"version\"\r\n");
    //r = ec_write(tmp, strlen(tmp), 5000, "OK", "ERROR");
    //if(r) { LOGE("QMTCFG=\"version\" failed\n"); return -1; }
    
    sprintf(tmp, "AT+QIACT?\r\n");
    r = ec_write(tmp, strlen(tmp), 5000, "OK", "ERROR");
        
#if 0
    sprintf(tmp, "AT+QMTCFG=\"ssl\",%d,0\r\n", mq->cid);
    r = ec_write(tmp, strlen(tmp), 5000, "OK", "ERROR");
    if(r) { LOGE("QMTCFG=\"ssl\" failed\n"); return -1; }
    
    sprintf(tmp, "AT+QMTCFG=\"dataformat\",%d,%d,%d\r\n", mq->cid, mq->fmt, mq->fmt);
    r = ec_write(tmp, strlen(tmp), 5000, "OK", "ERROR");
    if(r) { LOGE("QMTCFG=\"dataformat\" failed\n"); return -1; }
    
    sprintf(tmp, "AT+QMTCFG=\"view/mode\",%d,0\r\n", mq->cid);
    r = ec_write(tmp, strlen(tmp), 5000, "OK", "ERROR");
    if(r) { LOGE("QMTCFG=\"view/mode\" failed\n"); return -1; }
#endif
    
    
    /*
        https://iot.console.aliyun.com/
        user: iotadm@1292450854183053.onaliyun.com
        passwd: iotAdm@Xzl123!
    
        https://www.jianshu.com/p/09a2e81e0f65
    
        在线测试：
        http://www.daq-iot.com/
        tcp.org.eclipse.org:1883
    */
    
    /*
        
        ProductSecret: YDUM1FWC0lJKxJ5b
        ProductKey: h471xrlbuE9
    
        deviceName: gasLeap
        deviceSecret: 6fdeea63a1579f77dac9d99da38a7891
    
        hostName: iot-06z00ers5w6yi1p.mqtt.iothub.aliyuncs.com
        port: 1883
    
        AT+QMTCFG="aliauth",client_idx,"product_key","device_name","device_secret"
    */
    
    sprintf(tmp, "AT+QMTCFG=\"%s\",%d,\"%s\",\"%s\",\"%s\"\r\n", "aliauth", mq->cid, "h471xrlbuE9", "gasLeap", "6fdeea63a1579f77dac9d99da38a7891");
    r = ec_write(tmp, strlen(tmp), 5000, "OK", "ERROR");
    if(r) { LOGE("QMTCFG failed\n"); return -1; }
    
    
    sprintf(tmp, "AT+QMTDISC=%d\r\n", mq->cid);
    r = ec_write(tmp, strlen(tmp), 5000, "OK", "ERROR");
    if(r) { LOGE("QMTDISC failed\n"); return -1; }
    
    sprintf(tmp, "AT+QMTOPEN==%d,\"%s\",%d\r\n", mq->cid, "iot-06z00ers5w6yi1p.mqtt.iothub.aliyuncs.com", 1883);
    r = ec_write(tmp, strlen(tmp), 5000, "OK", "ERROR");
    if(r) { LOGE("QMTOPEN failed\n"); return -1; }
    
    sprintf(tmp, "AT+QMTCONN=%d,\"%s\"\r\n", mq->cid, "h471xrlbuE9.gasLeap|securemode=2,signmethod=hmacsha256,timestamp=1689734994831|");
    r= ec_write(tmp, strlen(tmp), 5000, "OK", "ERROR");
    if(r) { LOGE("QMTCONN failed\n"); return -1; }

#if 0
    sprintf(tmp, "AT+QMTSUB=%d,%d,\"%s\"\r\n", mq->cid, mq->mid, /h471xrlbuE9/gasLeap/user/XXX);
    r = ec_write(tmp, strlen(tmp), 5000, "OK", "ERROR");
    if(r) { LOGE("QMTSUB failed\n"); return -1; }
#endif
    
    LOGD("ec mqtt conn ok!\n");
    
    return 0;
}


static int ec_mqtt_disconn()
{
    int r;
    char tmp[100];
    ecxxx_handle_t* eh=exxxHandle;
    ecxxx_mqtt_t *mq=&ecMqtt;
    
    if(!eh) {
        return -1;
    }
    
    sprintf(tmp, "AT+QMTDISC=%d\r\n", mq->cid);
    r = ec_write(tmp, strlen(tmp), 5000, "OK", "ERROR");
    if(r) { LOGE("QMTDISC failed\n"); return -1; }
    
    sprintf(tmp, "AT+QMTCLOSE=%d\r\n", mq->cid);
    r= ec_write(tmp, strlen(tmp), 5000, "OK", "ERROR");
    if(r) { LOGE("QMTCLOSE failed\n"); return -1; }
    
    LOGD("ec mqtt disconn ok!\n");
    
    return 0;
}


static int mqtt_send(ecxxx_handle_t *h, char *topic, void *data, int len, int timeout)
{
    int r=0;
    ecxxx_handle_t* eh=exxxHandle;
    ecxxx_mqtt_t *mq=&ecMqtt;
    char *pbuf=(char*)eh->txBuf;
    U8 *pdata=NULL;
    int i,times,remain,onceMaxLen=1500;
    
    if(!eh || !data || !len) {
        return -1;
    }
    
    times = len/onceMaxLen;
    remain = len%onceMaxLen;
    
    sprintf(pbuf, "AT+QMTPUBEX=%d,%d,0,0,%s,%d\r\n", mq->cid, mq->mid, topic, len);
    r = ec_write(pbuf, strlen(pbuf), timeout, "OK", "ERROR");
    if(r) { LOGE("QMTPUBEX failed\n"); return -1; }
    
    for(i=0; i<times; i++) {
        pbuf = ((char*)data)+i*onceMaxLen;
        r = ec_write(pbuf, strlen(pbuf), timeout, "OK", "ERROR");
        if(r) { LOGE("QMTPUBEX failed\n"); break; }
    }
    
    if(r==0 && remain>0) {
        pbuf = ((char*)data)+len-remain;
        r = ec_write(pbuf, remain, timeout, "OK", "ERROR");
    }
    
    return r;
}
static int ec_mqtt_send(void *data, int len, int timeout)
{
    int r=0;
    char tmp[100];
    U8 *pdata=NULL;
    ecxxx_handle_t* eh=exxxHandle;
    int i,times,remain,onceMaxLen=1500*5;
    
    if(!eh || !data || !len) {
        return -1;
    }
    
    times = len/onceMaxLen;
    remain = len%onceMaxLen;
    for(i=0; i<times; i++) {
        pdata = ((U8*)data)+i*onceMaxLen;
        r = mqtt_send(eh, EC_TOPIC, pdata, onceMaxLen, timeout);
        if(r) {
            break;
        }
    }
    
    if(remain>0) {
        pdata = ((U8*)data)+len-remain;
        r = mqtt_send(eh, EC_TOPIC, pdata, remain, timeout);
    }
    
    return 0;
}




int ecxxx_conn(int proto, void *para)
{
    int r=0;
    
    switch(proto) {
        case PROTO_TCP:
        r = ec_tcp_conn((ecxxx_tcp_t*)para);
        break;
        
        case PROTO_MQTT:
        r = ec_mqtt_conn();
        break;
        
        default:
        return -1;
    }
    
    return r;
}


int ecxxx_disconn(int proto)
{
    int r=0;
    
    switch(proto) {
        case PROTO_TCP:
        r = ec_tcp_disconn();
        break;
        
        case PROTO_MQTT:
        r = ec_mqtt_disconn();
        break;
        
        default:
        return -1;
    }
    
    return r;
}

int ecxxx_send(int proto, void *data, int len, int timeout)
{
    int r=0;
    
    switch(proto) {
        case PROTO_TCP:
        r = ec_tcp_send(data, len, timeout);
        break;
        
        case PROTO_MQTT:
        r = ec_mqtt_send(data, len, timeout);
        break;
        
        default:
        return -1;
    }
    
    return r;
}



#define SEND_DATA_CNT       100
#define TEST_IP             "47.108.25.57"
#define TEST_PORT           7
#define RETRY_TIMES         1

#define TEST_PROTO          PROTO_MQTT
#define TEST_TIMEOUT        5000

typedef struct {
    
    int power_flag;
    int conn_flag;
    int reg_flag;
    int send_cnt;
    int send_flag;
}test_flag_t;
int ecxxx_test(void)
{
    int i,r;
    char tmp[100];
    test_flag_t tflag;
    ecxxx_cfg_t ec;
    ecxxx_tcp_t tcp={TEST_IP, TEST_PORT};
    ecxxx_mqtt_t mqtt;
    static int init_flag=0;
    int proto;
    void *param;
    
    proto = TEST_PROTO;
    if(proto==PROTO_TCP) {
        param = &tcp;
    }
    else {
        param = &mqtt;
    }
    
    if(init_flag==0) {
        ec.baudrate = 115200;
        ec.callback = NULL;
        ec.port = UART_1;
        ecxxx_init(&ec);
        init_flag = 1;
    }
    
    tflag.power_flag = 0;
    tflag.reg_flag   = 0;
    tflag.conn_flag  = -1;
    tflag.send_cnt   = 0;
    tflag.send_flag  = 0;
    
    r = ecxxx_power(1);
    if(r==0) {
        tflag.power_flag = 1;
        dal_delay_ms(500);
    }
    
    if(tflag.power_flag) {
        for(i=0; i<RETRY_TIMES; i++) {
            r = ecxxx_reg(APN_ID_CMNET);
            if(r==0) {
                
                ecxxx_ntp("120.25.115.20", 123);
                
                tflag.reg_flag=1;
                r = ecxxx_conn(proto, param);
                if(r==0) {
                    tflag.conn_flag=1;
                    break;
                }
            }
        }
        //ecxxx_restart();
    }
    
    if(tflag.conn_flag>0) {
        while(1) {
            
            if(tflag.send_cnt<SEND_DATA_CNT) {
                sprintf((char*)tmp, "ecxxx send test %d\n", tflag.send_cnt);
                r = ecxxx_send(PROTO_TCP, tmp, strlen(tmp), TEST_TIMEOUT);
                
                tflag.send_cnt++;
                LOGD("send cnt: %d\n", tflag.send_cnt);
                if(tflag.send_cnt==SEND_DATA_CNT) {
                    r = ecxxx_disconn(PROTO_TCP);
                    if(r==0) {
                        tflag.conn_flag = -2;
                    }
                }
            }
            
            if(!tflag.power_flag && tflag.conn_flag==-2) {
                r = ecxxx_power(0);
                if(r==0) {
                    tflag.power_flag = 1;
                }
            }
        }
    }
    else {
        LOGE("ecxxx connect failed !!\n");
        while(1);
    }
    
}




