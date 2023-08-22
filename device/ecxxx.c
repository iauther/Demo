#include "ecxxx.h"
#include "dal_gpio.h"
#include "dal_delay.h"
#include "rtc.h"
#include "dal.h"
#include "date.h"
#include "log.h"


#define RX_LEN                  (1024*2)
#define TX_LEN                  (1024*2)

#define MQTT_TOPIC              "/h471xrlbuE9/gasLeap/user/XXX"
#define MQTT_FORMAT             0       //0: char   1: binary
#define MQTT_QOS                1

//#define MQTT_SELF


typedef struct {
    TCP_CONT_ID  contID;
    TCP_CONN_ID  connID;
}tcp_para_t;

typedef struct {
    MQTT_CLI_ID   cliID;
    U8            forMat;    //0:字符   1：二进制
    U16           msgID;
}mqtt_para_t;


typedef struct {
    U8  power;
    U8  reg;
    U8  open;
    U8  conn;
}ecxxx_flag_t;


typedef struct {
    ecxxx_cfg_t cfg;
    handle_t    hurt;
    
    handle_t    hpwr;
    handle_t    hdtr;
        
    U8          txBuf[TX_LEN+4];
    int         txLen;
    
    U8          rxBuf[RX_LEN+4];
    int         rxLen;
    
    tcp_para_t  tcpPara;
    mqtt_para_t mqttPara;
    
    ecxxx_flag_t flag;
}ecxxx_handle_t;

#define ASC2INT(x) (((x)>='A')?(0x41+(x)-'A'):(0x30+(x)-'0'))
#define INT2ASC(x) (((x)<10)?('0'+(x)):('A'+((x)-9)))

static ecxxx_flag_t ecxxxflag;
static ecxxx_handle_t* exxxHandle=NULL;
static int bin_to_str(U8 *bin, int bin_len, U8 *str, int str_len)
{
    int i,j=0;
    
    if(str_len<bin_len*2+2) {
        return -1;
    }
    
    for(i=0; i<bin_len; i++) {
        str[j++] = INT2ASC(bin[i]>>4);
        str[j++] = INT2ASC(bin[i]&0x0f);
    }
    
    return j;
}
    
static int str_to_bin(U8 *str, int str_len, U8 *bin, int bin_len)
{
    int i,j=0;
    U8 a,b;
    
    if(bin_len<str_len*2) {
        return -1;
    }
    
    for(i=0; i<str_len; i+=2) {
        bin[j++] = (ASC2INT(str[i])<<4) | ASC2INT(str[i+1]);
    }
    
    return j;
}


static void inline ec_clear(ecxxx_handle_t *h)
{
    h->rxLen = 0;
    h->rxBuf[0] = 0;
}



static int ec_recv_callback(handle_t h, void *addr, U32 evt, void *data, int len)
{
    int r,xlen,xvt=-1;
    U8 *pdata=(U8*)data;
    ecxxx_handle_t* eh=exxxHandle;
    
    if(eh) {
        ecxxx_cb_t *cb=&eh->cfg.cb;
        
        if(eh->rxLen+len<=RX_LEN) {
            memcpy(eh->rxBuf+eh->rxLen, pdata, len);
            eh->rxLen += len;
            eh->rxBuf[eh->rxLen] = 0;
        }
        
        LOGD("$$:%s", eh->rxBuf);
        if(eh->rxLen>0) {
            char *pstr=(char*)eh->rxBuf;
            
            if(strstr(pstr, "+QMTSTAT:")) {
                int cid,err=0;
                char *ptr=(char*)data;
                
                r = sscanf(ptr, "+QMTSTAT: %d,%d", &cid, &err);
                if(r==2 && cb->err) {
                    LOGE("___ MQTT err: %d\n", err);
                     cb->err(PROTO_MQTT, err);
                }
            }
            else if(strstr(pstr, "+QMTRECV:")) {
                xvt = 0;
                r = str_to_bin(data, len, eh->rxBuf, eh->rxLen);
            }
            
            if(xvt>=0 && cb->data) {
                cb->data(PROTO_MQTT, eh->rxBuf, eh->rxLen);
            }
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
    char tmp[100];
    const char *delim="/";
    
	str = strstr(dst, delim);
    if(str) {
        strcpy(tmp, dst);
        str = strtok(tmp, delim);
        while (str) {
            if(strstr(buf, str)) {
                r = 1;
                break;
            }
            str = strtok(NULL, delim);
        }
    }
    else {
        if(strstr(buf, dst)) {
            r = 1;
        }
    }
    
    return r;
}
static int ec_wait(ecxxx_handle_t *h, int ms, U8 clr_data)
{
    int r=0;
    U32 rxLen=h->rxLen;
    U32 x,time=dal_get_tick_ms();
    U32 wait_ms = (ms==-1)?0xffffffff:ms;
    
    if(clr_data) {
        rxLen = 0;
        ec_clear(h);
    }
    while(1) {
        if(wait_ms--==0) {
            r = -1;
            break;    //timeout
        }
        
        if(h->rxLen>rxLen) {
            break;
        }
        
        dal_delay_ms(1);
    }
    
    return r;  
}

/*
   返回0： 成功
   返回-1：超时
   返回-2：出错
*/
static int ec_waitfor(ecxxx_handle_t *h, int ms, char *ok, char *err)
{
    int r=0;
    U32 x,time=dal_get_tick_ms();
    U32 wait_ms = (ms==-1)?0xffffffff:ms;
    
    ec_clear(h);
    if(wait_ms>0) {
        while(1) {
            if(wait_ms--==0) {
                r = -1;  break;    //timeout
            }
            
            if(h->rxLen>0) {
                if(ok && ec_search((char*)h->rxBuf, ok)) {
                    break;   //ok
                }
                
                if(err && ec_search((char*)h->rxBuf, err)) {
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


static int ec_write(void *data, int len)
{
    ecxxx_handle_t* eh=exxxHandle;
    
    LOGD("##:%s", (char*)data);
    dal_uart_write(eh->hurt, (U8*)data, len);
    return 0;
}
static int ec_write_wait(void *data, int len, int timeout_ms)
{
    int r;
    ecxxx_handle_t* eh=exxxHandle;
    
    ec_write(data, len);
    r = ec_wait(eh, timeout_ms, 1);
    return r;
}
static int ec_write_waitfor(void *data, int len, int timeout_ms, char *ok, char *err)
{
    int r;
    ecxxx_handle_t* eh=exxxHandle;
    
    ec_write(data, len);
    r = ec_waitfor(eh, timeout_ms, ok, err);
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
    
#if 1
    gc.gpio.grp = GPIOB;
    gc.gpio.pin = GPIO_PIN_12;
    gc.mode = MODE_OUT_OD;
    gc.pull = PULL_NONE;
    gc.hl = 1;
    h->hpwr = dal_gpio_init(&gc); //配置
   
    gc.gpio.grp = GPIOA;
    gc.gpio.pin = GPIO_PIN_7;
    gc.mode = MODE_OUT_OD;
    gc.pull = PULL_NONE;
    gc.hl = 0;
    h->hdtr = dal_gpio_init(&gc);
#endif

    h->tcpPara.contID = TCP_CONT_ID_0;
    h->tcpPara.connID = TCP_CONN_ID_0;
    
    h->mqttPara.cliID = MQTT_CLI_ID_1;
    h->mqttPara.forMat = MQTT_FORMAT;
    h->mqttPara.msgID  = 1;
    
    memset(&h->flag, 0, sizeof(h->flag));

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
    
    return ec_write_waitfor(data, len, timeout, NULL, NULL);
}



int ecxxx_wait(U32 ms, char *ok, char *err)
{
    ecxxx_handle_t* eh=exxxHandle;
    
    return ec_waitfor(eh, ms, ok, err);
}


int ecxxx_stat(ecxxx_stat_t *st)
{
    int r;
    char tmp[100];
    
    sprintf(tmp, "AT+CSQ=?\r\n");
    r = ec_write_waitfor(tmp, strlen(tmp), 5000, "OK", "ERROR");
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
    
    ec_clear(h);
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
            
            r = ec_waitfor(eh, 2000, "RDY", "ERROR");
            if(r<0) {
                LOGE("ecxxx power on failed!\n");
            }
        }
        else {          //power off
            dal_gpio_set_hl(eh->hpwr, 0);       //pull low power key
            dal_delay_ms(600);
            dal_gpio_set_hl(eh->hpwr, 1);
            
            r = ec_waitfor(eh, 2000, "POWERED DOWN", "ERROR");
            if(r) {
                LOGE("ecxxx power down failed!\n");
            }
            else {
                LOGD("ecxxx power down ok!\n");
            }
        }  
    }

#if 1
    if(on && r==0) {
        LOGD("set ATE0 now ...\n");
                
        sprintf(tmp, "ATE0\r\n");
        r1 = ec_write_waitfor(tmp, strlen(tmp), 500, "OK", "ERROR");
        if(r1<0) {
            LOGE("---ATE0--- fail!\n");
        }
    }
#endif
    
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
        
        r = ec_waitfor(eh, 2000, "POWERED DOWN", "ERROR");
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
        
        r = ec_waitfor(eh, 2000, "RDY", "ERROR");
        if(r<0) {
            LOGE("ecxxx power on failed!\n");
        }
        else {
            LOGD("ecxxx power on ok, set ATE0 now ...\n");
            
            sprintf(tmp, "ATE0\r\n");
            r1 = ec_write_waitfor(tmp, strlen(tmp), 500, "OK", "ERROR");
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
    
    sprintf(tmp, "AT+QNTP=%d,\"%s\",%d\r\n", eh->tcpPara.contID, server, port);
    r = ec_write_waitfor(tmp, strlen(tmp), 5000, "1/5", "ERROR");
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
    r = ec_write_waitfor(tmp, strlen(tmp), 1000, "READY", "ERROR");
    if(r) {
        LOGE("---CPIN--- fail\n");
        return r;
    }
    LOGD("---CPIN--- ok!\n");


    sprintf(tmp, "AT+CEREG?\r\n");
    r = ec_write_waitfor(tmp, strlen(tmp), 5000, "1/5", "ERROR");
    if(r) { LOGE("---CEREG--- fail\n"); return -1; }
    LOGD("---CEREG--- ok!\n");
#endif
    

#if 0
    sprintf(tmp, "AT+QENG=\"SERVINGCELL\"\r\n");
    r = ec_write_wait(tmp, strlen(tmp), 200);

    sprintf(tmp, "AT+QICSGP=%d,3,\"%s\",\"\",\"\",%d\r\n", eh->tcpPara.contID, apnInfo[apnID].apn, apnInfo[apnID].auth);
    r = ec_write_waitfor(tmp, strlen(tmp), 5000, "OK", "ERROR");
    if(r) { LOGE("---QICSGP fail\n"); return -1; }
    LOGD("---QICSGP--- ok!\n");
#endif
    
    sprintf(tmp, "AT+QIDEACT=%d\r\n", eh->tcpPara.contID);
    r = ec_write_waitfor(tmp, strlen(tmp), 5000, "OK", "ERROR");
    
    sprintf(tmp, "AT+QIACT=%d\r\n", eh->tcpPara.contID);
    r = ec_write_waitfor(tmp, strlen(tmp), 5000, "OK", "ERROR");
    if(r) { LOGE("---QIACT fail\n"); return -1; }
    LOGD("---QIACT--- ok!\n");

    
    return 0;
}
//////////////////////////////////////////////////////////////////
static int ec_tcp_conn(ecxxx_tcp_t *tcp)
{
    int i,j,r;
    char tmp[100];
    ecxxx_handle_t* eh=exxxHandle;
    
    sprintf(tmp, "AT+QIOPEN=%d,%d,\"TCP\",\"%s\",%d,0,2\r\n", eh->tcpPara.contID, eh->tcpPara.connID, tcp->ip, tcp->port);
    for(i=0; i<5; i++) {
        r = ec_write_waitfor(tmp, strlen(tmp), 5000, "CONNECT", "ERROR");
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
        r = ec_write_waitfor(tmp, strlen(tmp), 500, "OK", "ERROR");
        if(r) { LOGE("AT&D failed\n"); return -1; }
    }
    else {          //data mode
        sprintf(tmp, "ATO0\r\n");
        r = ec_write_waitfor(tmp, strlen(tmp), 500, "OK", "ERROR");
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
    r = ec_write_waitfor(tmp, strlen(tmp), 5000, "OK", "ERROR");
    if(r) { LOGE("disconn 1 failed\n"); return -1; }
    
    dal_delay_ms(1500);
#else
    r = ec_tcp_mode_switch(eh, 0);
    if(r) { LOGE("tcp mode switch failed\n"); return -1; }
#endif
    
    sprintf(tmp, "AT+QICLOSE=0\r\n");
    r = ec_write_waitfor(tmp, strlen(tmp), 5000, "OK", "ERROR");
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
    
    times = len/onceMaxLen;
    remain = len%onceMaxLen;
    for(i=0; i<times; i++) {
        pdata = ((U8*)data)+i*onceMaxLen;
        ec_write_waitfor(pdata, onceMaxLen, timeout, "OK", "ERROR");
    }
    
    if(remain>0) {
        pdata = ((U8*)data)+len-remain;
        ec_write_waitfor(pdata, remain, timeout, "OK", "ERROR");
    }
    
    return r;
}

static int ec_tcp_err_proc(int err)
{
    int r=0;
    
    switch(err) {
        
        default: return -1;
    }
    
}

//////////////////////////////////////////////////////////////////////////////////
/*
mqttClientId:  clientId+"|securemode=3,signmethod=hmacsha1,timestamp=132323232|"
               clientId：表示客户端ID，64字符内。 
               timestamp：表示当前时间毫秒值。 
               mqttClientId： 格式中||内为扩展参数。 signmethod：表示签名算法类型。 securemode：表示目前安全模式，可选值有2(TLS加密)和3(普通)

mqttUsername:  deviceName+"&"+productKey
mqttPassword:  sign_hmac(deviceSecret,content)
               sign_hmac为mqttClientId中的signmethod
               content为 "clientId${clientId}deviceName${deviceName}productKey${productKey}timestamp${timestamp}"
*/


#include "mqtt_sign.h"

#ifdef MQTT_SELF
const static dev_meta_t devMeta={
    .productKey    = "test",
    .productSecret = "test",
    .deviceName    = "test",
    .deviceSecret  = "test",
    
    .hostName      = "47.108.25.57",
    .port          = 1883,
};
#else
const static dev_meta_t devMeta={
    .productKey    = "h471xrlbuE9",
    .productSecret = "YDUM1FWC0lJKxJ5b",
    .deviceName    = "gasLeap",
    .deviceSecret  = "6fdeea63a1579f77dac9d99da38a7891",
    
    .hostName      = "iot-06z00ers5w6yi1p.mqtt.iothub.aliyuncs.com",
    .port          = 1883,
};
#endif

const static char *topicStr[TOPIC_MAX]={
    "/h471xrlbuE9/gasLeap/user/XXX",
    "/h471xrlbuE9/gasLeap/user/XXX",
    "/h471xrlbuE9/gasLeap/user/XXX",
};
static void my_sign(dev_meta_t *meta, mqtt_sign_t *sign)
{
    strcpy(sign->clientId, "test");
    strcpy(sign->username, "test");
    strcpy(sign->password, "test");
}
static ecxxx_login_t ecLogin={
    .username = "iauther",
    .password = "guohui78",
};

static void get_clientID(void)
{
    mcu_info_t info;

    dal_get_info(&info);
    sprintf(ecLogin.clientID, "%08X%08X%08X", info.uniqueID[2], info.uniqueID[1], info.uniqueID[0]);
}


static int ec_mqtt_conn()
{
    int r;
    char tmp[200];
    mqtt_sign_t sign;
    ecxxx_handle_t* eh=exxxHandle;
    
    if(!eh) {
        return -1;
    }

#ifdef MQTT_SELF
    my_sign((dev_meta_t*)&devMeta, &sign);
#else
    mqtt_sign((dev_meta_t*)&devMeta, &sign, SIGN_HMAC_SHA1);
#endif
    
    LOGD("___mqtt conn start.\n");
    
    sprintf(tmp, "AT+QMTCFG=\"version\",%d,4\r\n", eh->mqttPara.cliID);
    r = ec_write_waitfor(tmp, strlen(tmp), 5000, "OK", "ERROR");
    if(r) { LOGE("QMTCFG=\"version\" failed\n"); return -1; }
    
#if 0
    sprintf(tmp, "AT+QIACT?\r\n");
    r = ec_write_waitfor(tmp, strlen(tmp), 5000, "OK", "ERROR");

    sprintf(tmp, "AT+QMTCFG=\"ssl\",%d,0\r\n", mq->cid);
    r = ec_write_waitfor(tmp, strlen(tmp), 5000, "OK", "ERROR");
    if(r) { LOGE("QMTCFG=\"ssl\" failed\n"); return -1; }
    
    sprintf(tmp, "AT+QMTCFG=\"view/mode\",%d,0\r\n", eh->mqttPara.cliID);
    r = ec_write_waitfor(tmp, strlen(tmp), 5000, "OK", "ERROR");
    if(r) { LOGE("QMTCFG=\"view/mode\" failed\n"); return -1; }
    
    
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
        
        clientId: xxyy-100|securemode=2,signmethod=hmacsha1,timestamp=1689833956124|
        username; gasLeap&h471xrlbuE9
        password: C01A2ABED450D8584CBBB3B9EA7B947D9B8D6D26
        
    
        AT+QMTCFG="aliauth",client_idx,"product_key","device_name","device_secret"
    */
    
    sprintf(tmp, "AT+QMTCFG=\"%s\",%d,\"%s\",\"%s\",\"%s\"\r\n", "aliauth", eh->mqttPara.cliID, "h471xrlbuE9", "gasLeap", "6fdeea63a1579f77dac9d99da38a7891");
    r = ec_write_waitfor(tmp, strlen(tmp), 5000, "OK", "ERROR");
    if(r) { LOGE("QMTCFG failed\n"); return -1; }
#endif


    sprintf(tmp, "AT+QMTCFG=\"dataformat\",%d,%d,%d\r\n", eh->mqttPara.cliID, eh->mqttPara.forMat, eh->mqttPara.forMat);
    r = ec_write_waitfor(tmp, strlen(tmp), 5000, "OK", "ERROR");
    if(r) { LOGE("QMTCFG=\"dataformat\" failed\n"); return -1; }


    //sprintf(tmp, "ATI\r\n");
    //r = ec_write_waitfor(tmp, strlen(tmp), 5000, "OK", "ERROR");
    
    sprintf(tmp, "AT+QMTCLOSE=%d\r\n", eh->mqttPara.cliID);
    r = ec_write_wait(tmp, strlen(tmp), 2000);
    
    sprintf(tmp, "AT+QMTOPEN=%d,\"%s\",%d\r\n", eh->mqttPara.cliID, devMeta.hostName, devMeta.port);
    {
        char ok[40];
        
        sprintf(ok, "QMTOPEN: %d,0", eh->mqttPara.cliID);
        r = ec_write_waitfor(tmp, strlen(tmp), 5000, ok, NULL);
        if(r) { LOGE("___QMTOPEN %s!\n", (r==-1)?"timeout":"error"); return -1; }
    }
    
    sprintf(tmp, "AT+QMTDISC=%d\r\n", eh->mqttPara.cliID);
    r = ec_write_wait(tmp, strlen(tmp), 2000);
    
    sprintf(tmp, "AT+QMTCONN=%d,\"%s\",\"%s\",\"%s\"\r\n", eh->mqttPara.cliID, sign.clientId, sign.username, sign.password);
    {
        char ok[40];
        
        sprintf(ok, "QMTCONN: %d,0,0", eh->mqttPara.cliID);
        r = ec_write_waitfor(tmp, strlen(tmp), 5000, ok, "ERROR");
        if(r) { LOGE("___QMTCONN %s!\n", (r==-1)?"timeout":"error"); return -1; }
    }
    
    sprintf(tmp, "AT+QMTSUB=%d,%d,\"%s\",%d\r\n", eh->mqttPara.cliID, eh->mqttPara.msgID, MQTT_TOPIC, MQTT_QOS);
    {
        char ok[40];
        
        sprintf(ok, "QMTSUB: %d,%d,0", eh->mqttPara.cliID, eh->mqttPara.msgID);
        r = ec_write_waitfor(tmp, strlen(tmp), 5000, ok, "ERROR");
        if(r) { LOGE("___QMTSUB %s!\n", (r==-1)?"timeout":"error"); return -1; }
    }
    
    LOGD("ec mqtt conn ok!\n");
    
    return 0;
}


static int ec_mqtt_disconn()
{
    int r;
    char tmp[100];
    ecxxx_handle_t* eh=exxxHandle;
    
    if(!eh) {
        return -1;
    }
    
    sprintf(tmp, "AT+QMTDISC=%d\r\n", eh->mqttPara.cliID);
    r = ec_write_waitfor(tmp, strlen(tmp), 5000, "OK", "ERROR");
    if(r) { LOGE("QMTDISC failed\n"); return -1; }
    
    sprintf(tmp, "AT+QMTCLOSE=%d\r\n", eh->mqttPara.cliID);
    r= ec_write_waitfor(tmp, strlen(tmp), 5000, "OK", "ERROR");
    if(r) { LOGE("QMTCLOSE failed\n"); return -1; }
    
    LOGD("ec mqtt disconn ok!\n");
    
    return 0;
}


static int mqtt_send(ecxxx_handle_t *h, char *topic, U8 qos, void *data, int len, int timeout)
{
    int r=0,xlen;
    ecxxx_handle_t* eh=exxxHandle;
    char *pbuf=(char*)eh->txBuf;
    U8 *pdata=NULL;
    int i,times,remain,onceMaxLen=700;
    
    times = len/onceMaxLen;
    remain = len%onceMaxLen;
    
    sprintf(pbuf, "AT+QMTPUBEX=%d,%d,%d,0,%s,%d\r\n", eh->mqttPara.cliID, eh->mqttPara.msgID+1, qos, topic, MQTT_FORMAT?len:len*2);
    r = ec_write_waitfor(pbuf, strlen(pbuf), 5000, ">", "ERROR");
    if(r) { LOGE("QMTPUBEX failed\n"); return -1; }
    
    for(i=0; i<times; i++) {
        pdata = ((U8*)data)+i*onceMaxLen;
        
#if (MQTT_FORMAT==0)
        xlen = bin_to_str(pdata, onceMaxLen, (U8*)pbuf, TX_LEN);
        r = ec_write_waitfor(pbuf, xlen, 5000, "OK", "ERROR");
#else
        r = ec_write_waitfor(pdata, onceMaxLen, timeout, "OK", "ERROR");
#endif
        if(r) { LOGE("QMTPUBEX failed\n"); break; }
    }
    
    if(r==0 && remain>0) {
        pdata = ((U8*)data)+len-remain;
        
#if (MQTT_FORMAT==0)
        xlen = bin_to_str(pdata, remain, (U8*)pbuf, TX_LEN);
        
        pbuf[xlen] = '\r'; pbuf[xlen+1] = '\n'; pbuf[xlen+2] = 0;
        
        //LOGD("%s", pbuf);
        r = ec_write_waitfor(pbuf, xlen+2, 5000, "OK", "ERROR");
#else
        r = ec_write_waitfor(pdata, remain, timeout, "OK", "ERROR");
#endif
        if(r) { LOGE("QMTPUBEX failed\n"); }
    }
    
    return r;
}
static int ec_mqtt_send(char *topic, U8 qos, void *data, int len, int timeout)
{
    int r=0;
    char tmp[100];
    U8 *pdata=NULL;
    ecxxx_handle_t* eh=exxxHandle;
    int i,times,remain,onceMaxLen=700*5;    
    
    times = len/onceMaxLen;
    remain = len%onceMaxLen;
    for(i=0; i<times; i++) {
        pdata = ((U8*)data)+i*onceMaxLen;
        r = mqtt_send(eh, topic, qos, pdata, onceMaxLen, timeout);
        if(r) {
            break;
        }
    }
    
    if(remain>0) {
        pdata = ((U8*)data)+len-remain;
        r = mqtt_send(eh, topic, qos, pdata, remain, timeout);
    }
    
    return 0;
}


static int ec_mqtt_err_proc(int err)
{
    int r=0;
    
    switch(err) {
        case MQTT_ERR_DISCONN:
        //QMTOPEN, 重新连接
        break;
        
        case MQTT_ERR_PING_FAIL:
        //激活PDP, 并重新连接MQTT
        break;
        
        case MQTT_ERR_CONN_FAIL:
        //重新QMTCONN
        break;
        
        case MQTT_ERR_CONNACK_FAIL:
        //重新QMTCONN
        break;
        
        case MQTT_ERR_LINK_NOT_WORK:
        //报错，检查硬件线路
        break;
        
        //下面三种错误是客户端主动行为
        case MQTT_ERR_SERVER_DISCONN:
        case MQTT_ERR_CLIENT_DISCONN1:
        case MQTT_ERR_CLIENT_DISCONN2:
        break;
        
        default: return -1;
    }
    
    return r;
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
    ecxxx_handle_t* eh=exxxHandle;
    
    if(!eh || !data || !len || (!eh->flag.conn)) {
        return -1;
    }
    
    switch(proto) {
        case PROTO_TCP:
        r = ec_tcp_send(data, len, timeout);
        break;
        
        case PROTO_MQTT:
        r = ec_mqtt_send(MQTT_TOPIC, MQTT_QOS, data, len, timeout);
        break;
        
        default:
        return -1;
    }
    
    return r;
}


int ecxxx_clear(void)
{
    ecxxx_handle_t* eh=exxxHandle;
    
    if(!eh) {
        return -1;
    }
    
    ec_clear(eh);
    
    return 0;
}


int ecxxx_err_proc(int proto, int err)
{
    int r=0;
    
    if(proto==PROTO_TCP) {
        r = ec_tcp_err_proc(err);
    }
    else if(proto==PROTO_MQTT) {
        r = ec_mqtt_err_proc(err);
    }
    else {
        return -1;
    }
    
    return r;
}


int ecxxx_poll(void)
{
    int r;
    int proto=PROTO_MQTT;
    ecxxx_handle_t* eh=exxxHandle;
    ecxxx_flag_t *flag;
    
    if(!eh) {
        return -1;
    }
    flag = &eh->flag;
    
    
    if(flag->conn) {
        return 0;
    }
    
    
    if(!flag->power) {
        r = ecxxx_power(1);
        if(r==0) {
            flag->power = 1;
        }
    }
    else {          //开机成功
        if(!flag->reg) {
            r = ecxxx_reg(APN_ID_CMNET);
            if(r==0) {
                flag->reg = 1;
            }
        }
        
        if(flag->reg) {      //注册/激活成功 
            if(!flag->conn) {
                r = ecxxx_conn(proto, NULL);
                if(r==0) {
                    flag->conn = 1;
                }
            }
        }
    }
    
    return r; 
}


#define SEND_DATA_CNT       100
#define TEST_IP             "47.108.25.57"
#define TEST_PORT           7
#define RETRY_TIMES         5

#define TEST_PROTO          PROTO_MQTT
#define TEST_TIMEOUT        5000

int ecxxx_test(void)
{
    int i,r;
    
    
    return 0;
}




