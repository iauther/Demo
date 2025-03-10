#include "ecxxx.h"
#include "dal_gpio.h"
#include "dal_delay.h"
#include "dal_rtc.h"
#include "mem.h"
#include "rtc.h"
#include "dal.h"
#include "list.h"
#include "log.h"
#include "net.h"
#include "cfg.h"

#define RETRY_TIMES             5
#define RX_LEN                  (1024*50)
#define TX_LEN                  (1024*2)

#define CONT_ID                 TCP_CONT_ID_0
#define CONN_ID                 TCP_CONN_ID_1


#define MQTT_TOPIC              "/h471xrlbuE9/gasLeap/user/XXX"
#define MQTT_FORMAT             0       //0: char   1: binary
#define MQTT_QOS                1

//#define MQTT_SELF


#define PASS_THROUGH

//#define RX_DEBUG
//#define TX_DEBUG


#ifdef BOARD_V134
#define POWER_PORT  GPIOB
#define POWER_PIN   GPIO_PIN_12
#elif defined BOARD_V136
#define POWER_PORT  GPIOB
#define POWER_PIN   GPIO_PIN_1
#endif



typedef struct {
    TCP_CONT_ID  contID;
    TCP_CONN_ID  connID;
}tcp_para2_t;

typedef struct {
    MQTT_CLI_ID   cliID;
    U8            forMat;    //0:字符   1：二进制
    U16           msgID;
}mqtt_para2_t;


typedef struct {
    U8  power;
    U8  csgp;
    U8  reg;
    U8  open;
    U8  conn;
}ecxxx_flag_t;


typedef struct {
    ecxxx_cfg_t cfg;
    handle_t    hurt;
    
    handle_t    hpwr;
    handle_t    hdtr;
        
    char        txBuf[TX_LEN+4];
    int         txLen;
    
    char        rxBuf[RX_LEN+4];
    int         rxLen;
    int         busying;
    
    tcp_para2_t  tcpPara;
    mqtt_para2_t mqttPara;
    
    ecxxx_flag_t flag;
    handle_t    list;
}ecxxx_handle_t;

#define ASC2INT(x) (((x)>='A')?(0x41+(x)-'A'):(0x30+(x)-'0'))
#define INT2ASC(x) (((x)<10)?('0'+(x)):('A'+((x)-9)))

static ecxxx_handle_t *exxxHandle=NULL;
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
static int ec_write(ecxxx_handle_t *h, void *data, int len)
{
    if(len<64) {
        #ifdef TX_DEBUG
        LOGD("#:%s", (char*)data);
        #endif
    }
    
    dal_uart_write(h->hurt, (U8*)data, len);
    return 0;
}
static int ec_recv_callback(handle_t h, void *addr, U32 evt, void *data, int len, int flag)
{
    int r,xlen,xvt=-1;
    U8 *pdata=(U8*)data;
    ecxxx_handle_t* eh=(ecxxx_handle_t*)h;
    
    if(eh) {
        ecxxx_cb_t *cb=&eh->cfg.cb;
        
        if(!eh->busying && ((eh->rxLen+len)<RX_LEN)) {
            eh->busying = 1;
            memcpy(eh->rxBuf+eh->rxLen, pdata, len);
            eh->rxLen += len;
            eh->rxBuf[eh->rxLen] = 0;
            eh->busying = 0;
        }
        
        if(len<100) {
            char *p=(char*)data;
            p[len] = 0;
            //LOGD("$$:%s", eh->rxBuf);
            
            #ifdef RX_DEBUG
            LOGD("$:%s", p);
            #endif
        }
        
        if(eh->rxLen>0 && cb->data) {
            char *pstr=(char*)eh->rxBuf;
            
            cb->data(0, evt, eh->rxBuf, eh->rxLen);      //回调
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
    
    if(r==0) {
        x = dal_get_tick_ms()-time;
        //LOGD("waitfor %s ok: %d ms\n", ok, x);
    }
    else {
        //LOGD("waitfor %s failed, rsp: %s\n", ok, h->rxBuf);
    }
    
    return r;
}

static int ec_write_wait(ecxxx_handle_t *h, void *data, int len, int timeout_ms)
{
    int r;
    
    ec_write(h, data, len);
    r = ec_wait(h, timeout_ms, 1);
    return r;
}
static int ec_write_waitfor(ecxxx_handle_t *h, void *data, int len, int timeout_ms, char *ok, char *err)
{
    int r;
    
    ec_write(h, data, len);
    r = ec_waitfor(h, timeout_ms, ok, err);
    return r;
}
///////////////////////////////////////////////////////
handle_t ecxxx_init(ecxxx_cfg_t *cfg)
{
    ecxxx_handle_t *h=NULL;
    dal_uart_cfg_t uc;
    dal_gpio_cfg_t gc;
    
    if(!cfg) {
        return NULL;
    }
    
    h = malloc(sizeof(ecxxx_handle_t));
    if(!h) {
        return NULL;
    }
    
    h->cfg = *cfg;
    
    uc.baudrate = h->cfg.baudrate;
    uc.mode = MODE_IT;
    uc.port = h->cfg.port;
    uc.msb  = 0;
    uc.callback = ec_recv_callback;
    uc.handle = h;
    uc.rx.blen = 16*KB;
    uc.rx.buf = eMalloc(uc.rx.blen);
    uc.rx.dlen = 0;
    
    h->hurt = dal_uart_init(&uc);
    
    gc.gpio.grp = POWER_PORT;
    gc.gpio.pin = POWER_PIN;
    gc.mode = MODE_OUT_PP;
    gc.pull = PULL_NONE;
    gc.hl = 1;
    h->hpwr = dal_gpio_init(&gc); //配置

#if 1
    gc.gpio.grp = GPIOA;
    gc.gpio.pin = GPIO_PIN_7;
    gc.mode = MODE_OUT_OD;
    gc.pull = PULL_NONE;
    gc.hl = 0;
    h->hdtr = dal_gpio_init(&gc);
#endif

    h->tcpPara.contID = CONT_ID;
    h->tcpPara.connID = CONN_ID;
    
    h->mqttPara.cliID = MQTT_CLI_ID_1;
    h->mqttPara.forMat = MQTT_FORMAT;
    h->mqttPara.msgID  = 1;
    
    list_cfg_t lc;
    lc.log = 1;
    lc.max = 100;
    lc.mode = LIST_FULL_FILO;
    h->list = list_init(&lc);
    
    memset(&h->flag, 0, sizeof(h->flag));

    return h;
}


int ecxxx_deinit(handle_t h)
{
    ecxxx_handle_t* eh=(ecxxx_handle_t*)h;
    
    if(!eh) {
        return -1;
    }
    
    dal_gpio_deinit(eh->hdtr);
    dal_gpio_deinit(eh->hpwr);
    dal_uart_init(eh->hurt);
    free(eh);
    
    return 0;
}


int ecxxx_write(handle_t h, void *data, int len, int timeout)
{
    ecxxx_handle_t* eh=(ecxxx_handle_t*)h;
    
    if(!eh || !data || !len) {
        return -1;
    }
    
    return ec_write_waitfor(eh, data, len, timeout, NULL, NULL);
}



int ecxxx_wait(handle_t h, U32 ms, char *ok, char *err)
{
    ecxxx_handle_t* eh=exxxHandle;
    
    if(!eh) {
        return -1;
    }
    
    return ec_waitfor(eh, ms, ok, err);
}


int ecxxx_stat(handle_t h, ecxxx_stat_t *st)
{
    int r;
    char tmp[100];
    ecxxx_handle_t* eh=(ecxxx_handle_t*)h;
    
    sprintf(tmp, "AT+CSQ=?\r\n");
    r = ec_write_waitfor(eh, tmp, strlen(tmp), 5000, "OK", "ERROR");
    if(r) { LOGE("AT+CSQ failed\n"); return -1; }
    
    if(st) {
        //
    }
    
    return 0;
}


static U8 is_pwron(ecxxx_handle_t *h)
{
    U8 r=0;
    char tmp[100];
    int wait_ms=50;
    U32 x,time;
    
    sprintf(tmp, "AT\r\n");
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
int ecxxx_power(handle_t h, U8 on)
{
    int r=0,r1;
    char tmp[100];
    U8 pwron=0;
    char *str[2]={"off","on"};
    ecxxx_handle_t* eh=(ecxxx_handle_t*)h;
    
    if(!eh) {
        return -1;
    }
    
    pwron = is_pwron(eh);
    LOGD("at power is %s, switch to %s\n", str[pwron], str[on]);
    
    if(pwron!=on) {
        if(on) {        //power on
            dal_gpio_set_hl(eh->hpwr, 0);
            dal_delay_ms(600);
            dal_gpio_set_hl(eh->hpwr, 1);
            
            dal_delay_ms(2000);
        }
        else {          //power off
            dal_gpio_set_hl(eh->hpwr, 0);       //pull low power key
            dal_delay_ms(700);
            dal_gpio_set_hl(eh->hpwr, 1);
            
            dal_delay_ms(2000);
        } 
    }

#if 1
    if(on && r==0) {    
        sprintf(tmp, "ATE0\r\n");
        r1 = ec_write_waitfor(eh, tmp, strlen(tmp), 500, "OK", "ERROR");
        if(r1<0) {
            LOGE("___ATE0 failed!\n");
        }
        
    #ifdef PASS_THROUGH
        sprintf(tmp, "AT&D1\r\n");
        r1 = ec_write_waitfor(h, tmp, strlen(tmp), 500, "OK", "ERROR");
        if(r1<0) {
            LOGE("___DTR failed!\n");
        }
    #endif
    }
#endif
    
    return r;
}


int ecxxx_restart(handle_t h)
{
    int r,r1;
    char tmp[100];
    U8 pwron=0;
    ecxxx_handle_t* eh=(ecxxx_handle_t*)h;
    
    if(!eh) {
        return -1;
    }
    
    pwron = is_pwron(eh);
    if(pwron) {        //if already powered on, power down it
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
            sprintf(tmp, "ATE0\r\n");
            r1 = ec_write_waitfor(eh, tmp, strlen(tmp), 500, "OK", "ERROR");
            if(r1<0) {
                LOGE("---ATE0--- fail!\n");
            }
        }
    }
    
    return r;
}


int ecxxx_ntp(handle_t h, char *server, U16 port)
{
    int r;
    char tmp[100];
    datetime_t dt;
    ecxxx_handle_t* eh=(ecxxx_handle_t*)h;
    
    if(!eh) {
        return -1;
    }
    
    sprintf(tmp, "AT+QNTP=%d,\"%s\",%d\r\n", eh->tcpPara.contID, server, port);
    r = ec_write_waitfor(eh, tmp, strlen(tmp), 5000, "1/5", "ERROR");
    if(r) { LOGE("---QNTP--- fail\n"); return -1; }
    
    r = get_datetime((char*)eh->rxBuf, &dt);
    if(r==0) {
        rtc2_set_time(&dt);
    }
    
    return 0;
}


int ecxxx_reg(handle_t h, APN_ID apnID)
{
    int i,j,r;
    char tmp[100];
    ecxxx_handle_t* eh=(ecxxx_handle_t*)h;
    apn_info_t apnInfo[APN_ID_MAX]={
                {"CMNET",  1},
                {"UNINET", 1},
                {"CTNET",  1},};

    if(!eh) {
        return -1;
    }
                
#if 0
    sprintf(tmp, "AT+CPIN?\r\n");
    r = ec_write_waitfor(eh, tmp, strlen(tmp), 1000, "READY", "ERROR");
    if(r) {
        LOGE("---CPIN--- fail\n");
        return r;
    }
    LOGD("---CPIN--- ok!\n");


    sprintf(tmp, "AT+CEREG?\r\n");
    r = ec_write_waitfor(eh, tmp, strlen(tmp), 5000, "1/5", "ERROR");
    if(r) { LOGE("---CEREG--- fail\n"); return -1; }
    LOGD("---CEREG--- ok!\n");
#endif
    

#if 0
    sprintf(tmp, "AT+QENG=\"SERVINGCELL\"\r\n");
    r = ec_write_wait(eh, tmp, strlen(tmp), 200);
#endif

    if(eh->flag.csgp==0) {
        sprintf(tmp, "AT+QICSGP=%d,3,\"%s\",\"\",\"\",%d\r\n", eh->tcpPara.contID, apnInfo[apnID].apn, apnInfo[apnID].auth);
        r = ec_write_waitfor(eh, tmp, strlen(tmp), 5000, "OK", "ERROR");
        if(r) { LOGE("---QICSGP fail\n"); return -1; }
        
        eh->flag.csgp = 1;
        LOGD("---QICSGP--- ok!\n");
    }
    
    if(eh->flag.reg==0) {
        sprintf(tmp, "AT+QIDEACT=%d\r\n", eh->tcpPara.contID);
        r = ec_write_waitfor(eh, tmp, strlen(tmp), 5000, "OK", "ERROR");
        if(r) { LOGE("---QIDEACT fail\n"); return -1; }
        LOGD("---QIDEACT--- ok!\n");
        
        sprintf(tmp, "AT+QIACT=%d\r\n", eh->tcpPara.contID);
        r = ec_write_waitfor(eh, tmp, strlen(tmp), 5000, "OK", "ERROR");
        if(r) { LOGE("---QIACT fail\n"); return -1; }
        
        eh->flag.reg = 1;
        LOGD("---QIACT--- ok!\n");
    }
    
    return 0;
}
//////////////////////////////////////////////////////////////////
static int ec_conn(ecxxx_handle_t *h, net_para_t *para, int timeout)
{
    int r;
    char tmp[100];

#ifdef PASS_THROUGH
    sprintf(tmp, "AT+QIOPEN=%d,%d,\"TCP\",\"%s\",%d,0,2\r\n", h->tcpPara.contID, h->tcpPara.connID, para->para.tcp.ip, para->para.tcp.port);
    r = ec_write_waitfor(h, tmp, strlen(tmp), timeout, "CONNECT", "ERROR");
#else
    sprintf(tmp, "AT+QIOPEN=%d,%d,\"TCP\",\"%s\",%d,0,1\r\n", h->tcpPara.contID, h->tcpPara.connID, para->para.tcp.ip, para->para.tcp.port, mod);
    r = ec_write_waitfor(h, tmp, strlen(tmp), timeout, "+QIOPEN", "ERROR");
#endif
    
    return r;
}
static int ec_disconn(ecxxx_handle_t *h, int timeout)
{
    int r;
    char tmp[100];
    
    sprintf(tmp, "AT+QICLOSE=%d\r\n", h->tcpPara.contID);
    r = ec_write_waitfor(h, tmp, strlen(tmp), timeout, "OK", "ERROR");
    
    return r;
}



int ecxxx_conn(handle_t h, void *para, int timemout)
{
    int i,j,r=-1,quit=0;
    char tmp[100];
    conn_para_t *conn=(conn_para_t*)para;
    ecxxx_handle_t* eh=(ecxxx_handle_t*)h;
    
    if(eh->flag.conn==1) {
        return 0;
    }
    
    LOGD("___ ec conn\n");
    for(i=0; i<RETRY_TIMES; i++) {
        r = ec_disconn(eh, timemout);
        if(r) {
            LOGE("___ ec_disconn fail, %d\n", i);
            continue;
        }
        
        for(j=0; j<RETRY_TIMES; j++) {
            r = ec_conn(eh, conn->para, timemout);
            if(r) {
                LOGE("___ ec_conn fail, %d\n", j);
                continue;
            }
            eh->flag.conn = 1; 
            quit = 1; r = 0;
            break;
        }
        
        if(quit) {
            break;
        }
    }
    
    ec_clear(eh);
    
    return -1;
}


int ecxxx_disconn(handle_t h, int timemout)
{
    int i,r;
    char tmp[100];
    ecxxx_handle_t* eh=(ecxxx_handle_t*)h;
    
    if(!eh) {
        return -1;
    }
    
    for(i=0; i<RETRY_TIMES; i++) {
        r = ec_disconn(eh, timemout);
        if(r==0) {
            LOGD("___ec_disconn ok!\n");
            break;
        }
    }
    
    return r;
}

static int ec_post_send(ecxxx_handle_t *h)
{
#ifdef PASS_THROUGH
    dal_gpio_set_hl(h->hdtr, 0);
    dal_delay_ms(1);
    dal_gpio_set_hl(h->hdtr, 1);
#endif
    return 0;
}

static int ec_send(ecxxx_handle_t *h, void *data, int len, int timeout)
{
    int r=0;

#ifndef PASS_THROUGH
    sprintf(h->txBuf, "AT+QISEND=%d,%d\r\n", h->tcpPara.connID, len);
    r = ec_write_waitfor(h, h->txBuf, strlen(h->txBuf), timeout, ">", "ERROR");
    if(r) {
        LOGE("___QISEND, '>' NOT recved!\n");
        return r;
    }
#endif
    
    ec_write(h, data, len);
    
#ifndef PASS_THROUGH
    r = ec_waitfor(h, timeout, "SEND OK", "ERROR/SEND FAIL");
    if(r) {
        LOGD("___ QISEND, 'SEND OK' NOT recved!\n");
    }
#endif
    
    return r;
}
int ecxxx_send(handle_t h, void *data, int len, int timeout)
{
    U8 *pdata;
    int xlen,r=0;
    int i,j,times,remain,onceMaxLen=1024,sendLen=0;
    ecxxx_handle_t* eh=(ecxxx_handle_t*)h;
    
    LOGD("___ ecxxx send len: %d\n", len);
    
    times = len/onceMaxLen;
    remain = len%onceMaxLen;
    if(times>0) {
        for(i=0; i<times; i++) {
            pdata = ((U8*)data)+i*onceMaxLen;
            for(j=0; j<RETRY_TIMES; j++) {
                r = ec_send(eh, pdata, onceMaxLen, timeout);
                if(r==0) {
                    break;
                }
                LOGW("___ retry %d\n", j);
            }
            
            if(r==0) {
                sendLen += onceMaxLen;
            }
            else {
                break;
            }
        }
    }
    
    if(r==0 && remain>0) {
        pdata = ((U8*)data)+len-remain;
        r = ec_send(eh, pdata, remain, timeout);
        if(r==0) {
            sendLen += remain;
        }
        
        r = ec_post_send(h);
    }
    LOGD("___ totalLen: %d, sendLen: %d\n", len, sendLen);
    
    return r;
}



int ecxxx_test(void)
{
    int i,r,maxCnt=10,sflag=0;
    int rlen=0,slen=1024*40;
    U8 *p;
    int cnt=0,quit=0;
    handle_t h;
    ecxxx_cfg_t ec;
    conn_para_t cp;
    ecxxx_handle_t *eh=NULL;
    net_para_t  np={
        .mode = 0,
        .para = {
            .tcp = {
                .ip = "47.108.25.57",
                .port = 2551,     //with data echo
                //.port = 2552,       //with datalen echo
                //.port = 7,
            }
        }
    };
    
    ec.baudrate = 115200;
    ec.port = UART_1;
    ec.cb.data = 0;
    h = ecxxx_init(&ec);
    if(!h) {
        return -1;
    }
    ecxxx_power(h, 1);
    
    eh = (ecxxx_handle_t*)h;
    ecxxx_flag_t *flag=&eh->flag;
    
    cp.callback = 0;
    cp.para = &np;
    cp.proto = PROTO_TCP;
    
    p = (U8*)eMalloc(slen);
    if(!p) {
        return -1;
    }
    for(i=0; i<slen; i++) {
        p[i] = i&0xff;
    }
    
    while(1) {
        r = ecxxx_reg(h, APN_ID_CMNET);
        
        if(flag->reg) {
            r = ecxxx_conn(h, &cp, 2000);
        }
        
        if(flag->conn) {
            
            if(!sflag && cnt<maxCnt) {
                
                r = ecxxx_send(h, p, slen, 2000);
                if(r==0) {
                    cnt++;
                    sflag = 1;
                    LOGD("____ ecxxx_send %d ok\n", cnt);
                }
                else {
                    LOGE("____ ecxxx_send %d failed\n", cnt);
                    flag->conn = 0;
                }
            }
#if 0            
            if(!eh->busying && eh->rxLen>0) {
                eh->busying = 1;
                list_append(eh->list, 0, eh->rxBuf, eh->rxLen);
                rlen += eh->rxLen;
                eh->rxLen = 0;
                eh->busying = 0;
            }
#endif
            
            if(eh->rxLen>=slen) {
                r = memcmp(eh->rxBuf, p, slen);
                eh->rxLen = 0;
                sflag = 0;
                LOGD("____ memcmp %d, cnt: %d, len: %d\n", r, cnt, slen);
                
                if(cnt==maxCnt) {
                    xFree(p);
                    break;
                }
            }
        }
        else {
            dal_delay_ms(500);
        }
    }
    while(1);
    
    return 0;
}




