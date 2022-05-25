#include "esp8266.h"
#include "drv/uart.h"
#include "drv/delay.h"
#include "myCfg.h"

typedef struct {
    handle_t        huart;
    esp8266_data_t  esp;
    
}uart_handle_t;
static uart_handle_t espHandle={0};

static int rxLength=0;
static U8 tmpBuf[FRAME_MAX_LEN];
static void rx_callback(U8 *data, U16 len)
{
    if(len<=FRAME_MAX_LEN) {
        memcpy(tmpBuf, data, len);
        rxLength = len;
    }
}
static int rx_wait(int ms)
{
    int n=1;
    while(!rxLength) {
        if(ms>0 && n++<ms) {
            break;
        }
        
        delay_ms(1);
    }
    return (n==ms)?-1:n;
}



void esp8266_init(void)
{
    uart_cfg_t cfg;
    
    cfg.mode = MODE_DMA;
    cfg.port = ESP8266_PORT;
    cfg.baudrate = ESP8266_BAUDRATE;
    cfg.para.rx  = rx_callback;
    cfg.para.buf = espHandle.esp.rxBuf;
    cfg.para.blen = sizeof(espHandle.esp.rxBuf);
    cfg.para.dlen = 0;
    
    espHandle.huart = uart_init(&cfg);
}
//对ESP8266模块发送AT指令
// cmd 待发送的指令
// ack1,ack2;期待的响应，为NULL表不需响应，两者为或逻辑关系
// time 等待响应时间
//返回1发送成功， 0失败
int esp8266_send_cmd(void *cmd, char *ack1, char *ack2, U32 time)
{
    int r;
    
    uart_write(espHandle.huart, cmd, strlen(cmd));
    r = rx_wait(200);
    if(r>0) {
    tmpBuf[0] = 0; rxLength = 0;
    if(ack1 && ack2) {
        return (strstr((char*)tmpBuf, ack1) || strstr ((char*)tmpBuf, ack2));
    }
    else if( ack1 != 0 )  //strstr(s1,s2);检测s2是否为s1的一部分，是返回该位置，否则返回false，它强制转换为bool类型了
        return (strstr(ESP8266_Fram_Record_Struct .Data_RX_BUF, ack1));

    else
        return (strstr(ESP8266_Fram_Record_Struct .Data_RX_BUF, ack2));

}



void esp8266_reset(void)
{
    
}


void esp8266_restore(void)
{
    char count=0;
        
    while(count++ < 10) {
        if(ESP8266_Send_AT_Cmd("AT+RESTORE", "OK", NULL, 500))  {
            //printf("OK\r\n");
            return;
        }
    }
}


int esp8266_set_mode(net_mode_t mode)
{
    switch (mode){
        case MODE_STA:
            return ESP8266_Send_AT_Cmd ( "AT+CWMODE=1", "OK", "no change", 2500 ); 

        case MODE_AP:
            return ESP8266_Send_AT_Cmd ( "AT+CWMODE=2", "OK", "no change", 2500 ); 

        case MODE_APSTA:
            return ESP8266_Send_AT_Cmd ( "AT+CWMODE=3", "OK", "no change", 2500 ); 

        default:
          return false;
    }
}


//ESP8266连接外部的WIFI
//pSSID WiFi帐号
//pPassWord WiFi密码
//设置成功返回true 反之false
int esp8266_connect_ap(char *ssid, char *password)
{
    char tmp[80];
	
    sprintf(tmp, "AT+CWJAP=\"%s\",\"%s\"", ssid, password);
    return ESP8266_Send_AT_Cmd(tmp, "OK", NULL, 5000);
}

//ESP8266 透传使能
//enumEnUnvarnishTx  是否多连接，bool类型
//设置成功返回true，反之false
int esp8266_enable_multiID(FunctionalState enumEnUnvarnishTx )
{
    char cStr [20];

    sprintf ( cStr, "AT+CIPMUX=%d", ( enumEnUnvarnishTx ? 1 : 0 ) );

    return ESP8266_Send_AT_Cmd ( cStr, "OK", 0, 500 );

}



int esp8266_connect_server(net_prot_t prot, char *ip, char *port, net_id_t id)
{
    char cStr [100] = { 0 }, cCmd [120];

    switch (prot )
    {
        case PROT_TCP:
          sprintf ( cStr, "\"%s\",\"%s\",%s", "TCP", ip, port);
          break;

        case PROT_UDP:
          sprintf ( cStr, "\"%s\",\"%s\",%s", "UDP", ip, port);
          break;

        default:
            break;
    }

    if ( id < 5 )
        sprintf ( cCmd, "AT+CIPSTART=%d,%s", id, cStr);

    else
        sprintf ( cCmd, "AT+CIPSTART=%s", cStr );

    return ESP8266_Send_AT_Cmd ( cCmd, "OK", "ALREAY CONNECT", 4000 );

}


//ESP8266退出透传模式
void esp8266_set_transparent(U8 on)
{
    if(on) {
       ESP8266_Send_AT_Cmd ( "AT+CIPMODE=1", "OK", 0, 500 ); 
        ESP8266_Send_AT_Cmd( "AT+CIPSEND", "OK", ">", 500 );
    }
    else {
        delay_ms(1000);
        ESP8266_USART( "+++" );
        delay_ms( 500 );
    }
}


//ESP8266 检测连接状态
//返回0：获取状态失败
//返回2：获得ip
//返回3：建立连接 
//返回4：失去连接 
int ESP8266_get_link_stat (void)
{
    if (ESP8266_Send_AT_Cmd( "AT+CIPSTATUS", "OK", 0, 500 ) )
    {
        if ( strstr ( ESP8266_Fram_Record_Struct .Data_RX_BUF, "STATUS:2\r\n" ) )
            return 1;

        else if ( strstr ( ESP8266_Fram_Record_Struct .Data_RX_BUF, "STATUS:3\r\n" ) )
            return 2;

        else if ( strstr ( ESP8266_Fram_Record_Struct .Data_RX_BUF, "STATUS:4\r\n" ) )
            return 3;       

    }

    return -1;
}



/*
*MQTT配置用户属性
*LinkID 连接ID,目前只支持0
*scheme 连接方式，这里选择MQTT over TCP,这里设置为1
*client_id MQTTclientID 用于标志client身份
*username 用于登录 MQTT 服务器 的 username
*password 用于登录 MQTT 服务器 的 password
*cert_key_ID 证书 ID, 目前支持一套 cert 证书, 参数为 0
*CA_ID 目前支持一套 CA 证书, 参数为 0
*path 资源路径，这里设置为""
*设置成功返回true 反之false
*/
int esp8266_mqtt_set(char *clientID, char *user, char *password)
{
    char tmp[80];
    sprintf (tmp, "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"", clientID, user, password);
    return ESP8266_Send_AT_Cmd(tmp, "OK", NULL, 500);
}


/*
*连接指定的MQTT服务器
*LinkID 连接ID,目前只支持0
*IP：MQTT服务器上对应的IP地址
*ComNum MQTT服务器上对应的端口号，一般为1883
*设置成功返回true 反之false
*/
bool ESP8266_MQTTCONN( char * Ip, int  Num)
{
    char cCmd [120];
    sprintf ( cCmd,"AT+MQTTCONN=0,\"%s\",%d,0", Ip,Num);
    return ESP8266_Send_AT_Cmd( cCmd, "OK", NULL, 500 );
}

/*
*订阅指定连接的 MQTT 主题, 可重复多次订阅不同 topic
*LinkID 连接ID,目前只支持0
*Topic 订阅的主题名字，这里设置为Topic
*Qos值：一般为0，这里设置为1
*设置成功返回true 反之false
*/
bool esp8266_mqtt_sub(char *topic)
{
    char cCmd[120];
    sprintf ( cCmd, "AT+MQTTSUB=0,\"%s\",1", topic);
    return ESP8266_Send_AT_Cmd( cCmd, "OK", NULL, 500 );
}


/*
*在LinkID上通过 topic 发布数据 data, 其中 data 为字符串消息
*LinkID 连接ID,目前只支持0
*Topic 订阅的主题名字，这里设置为Topic
*data：字符串信息
*设置成功返回true 反之false
*/
bool esp8266_mqtt_pub(char *topic, char *data)
{
    char cCmd [120];
    sprintf (cCmd, "AT+MQTTPUB=0,\"%s\",\"%s\",1,0", topic ,data);
    return ESP8266_Send_AT_Cmd( cCmd, "OK", NULL, 1000 );
}

/*
*关闭 MQTT Client 为 LinkID 的连接, 并释放内部占用的资源
*LinkID 连接ID,目前只支持0
*Topic 订阅的主题名字，这里设置为Topic
*data：字符串信息
*设置成功返回true 反之false
*/
int esp8266_mqtt_clean(void)
{
    char cCmd [120];
    sprintf ( cCmd, "AT+MQTTCLEAN=0");
    return ESP8266_Send_AT_Cmd( cCmd, "OK", NULL, 500 );
}



