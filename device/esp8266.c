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
//��ESP8266ģ�鷢��ATָ��
// cmd �����͵�ָ��
// ack1,ack2;�ڴ�����Ӧ��ΪNULL������Ӧ������Ϊ���߼���ϵ
// time �ȴ���Ӧʱ��
//����1���ͳɹ��� 0ʧ��
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
    else if( ack1 != 0 )  //strstr(s1,s2);���s2�Ƿ�Ϊs1��һ���֣��Ƿ��ظ�λ�ã����򷵻�false����ǿ��ת��Ϊbool������
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


//ESP8266�����ⲿ��WIFI
//pSSID WiFi�ʺ�
//pPassWord WiFi����
//���óɹ�����true ��֮false
int esp8266_connect_ap(char *ssid, char *password)
{
    char tmp[80];
	
    sprintf(tmp, "AT+CWJAP=\"%s\",\"%s\"", ssid, password);
    return ESP8266_Send_AT_Cmd(tmp, "OK", NULL, 5000);
}

//ESP8266 ͸��ʹ��
//enumEnUnvarnishTx  �Ƿ�����ӣ�bool����
//���óɹ�����true����֮false
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


//ESP8266�˳�͸��ģʽ
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


//ESP8266 �������״̬
//����0����ȡ״̬ʧ��
//����2�����ip
//����3���������� 
//����4��ʧȥ���� 
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
*MQTT�����û�����
*LinkID ����ID,Ŀǰֻ֧��0
*scheme ���ӷ�ʽ������ѡ��MQTT over TCP,��������Ϊ1
*client_id MQTTclientID ���ڱ�־client���
*username ���ڵ�¼ MQTT ������ �� username
*password ���ڵ�¼ MQTT ������ �� password
*cert_key_ID ֤�� ID, Ŀǰ֧��һ�� cert ֤��, ����Ϊ 0
*CA_ID Ŀǰ֧��һ�� CA ֤��, ����Ϊ 0
*path ��Դ·������������Ϊ""
*���óɹ�����true ��֮false
*/
int esp8266_mqtt_set(char *clientID, char *user, char *password)
{
    char tmp[80];
    sprintf (tmp, "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"", clientID, user, password);
    return ESP8266_Send_AT_Cmd(tmp, "OK", NULL, 500);
}


/*
*����ָ����MQTT������
*LinkID ����ID,Ŀǰֻ֧��0
*IP��MQTT�������϶�Ӧ��IP��ַ
*ComNum MQTT�������϶�Ӧ�Ķ˿ںţ�һ��Ϊ1883
*���óɹ�����true ��֮false
*/
bool ESP8266_MQTTCONN( char * Ip, int  Num)
{
    char cCmd [120];
    sprintf ( cCmd,"AT+MQTTCONN=0,\"%s\",%d,0", Ip,Num);
    return ESP8266_Send_AT_Cmd( cCmd, "OK", NULL, 500 );
}

/*
*����ָ�����ӵ� MQTT ����, ���ظ���ζ��Ĳ�ͬ topic
*LinkID ����ID,Ŀǰֻ֧��0
*Topic ���ĵ��������֣���������ΪTopic
*Qosֵ��һ��Ϊ0����������Ϊ1
*���óɹ�����true ��֮false
*/
bool esp8266_mqtt_sub(char *topic)
{
    char cCmd[120];
    sprintf ( cCmd, "AT+MQTTSUB=0,\"%s\",1", topic);
    return ESP8266_Send_AT_Cmd( cCmd, "OK", NULL, 500 );
}


/*
*��LinkID��ͨ�� topic �������� data, ���� data Ϊ�ַ�����Ϣ
*LinkID ����ID,Ŀǰֻ֧��0
*Topic ���ĵ��������֣���������ΪTopic
*data���ַ�����Ϣ
*���óɹ�����true ��֮false
*/
bool esp8266_mqtt_pub(char *topic, char *data)
{
    char cCmd [120];
    sprintf (cCmd, "AT+MQTTPUB=0,\"%s\",\"%s\",1,0", topic ,data);
    return ESP8266_Send_AT_Cmd( cCmd, "OK", NULL, 1000 );
}

/*
*�ر� MQTT Client Ϊ LinkID ������, ���ͷ��ڲ�ռ�õ���Դ
*LinkID ����ID,Ŀǰֻ֧��0
*Topic ���ĵ��������֣���������ΪTopic
*data���ַ�����Ϣ
*���óɹ�����true ��֮false
*/
int esp8266_mqtt_clean(void)
{
    char cCmd [120];
    sprintf ( cCmd, "AT+MQTTCLEAN=0");
    return ESP8266_Send_AT_Cmd( cCmd, "OK", NULL, 500 );
}



