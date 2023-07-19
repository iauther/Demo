#include <stdio.h>
#include "hv/hv.h"
#include "myMqtt.h"

//#pragma comment(lib, "lib/hv.lib")
#pragma comment(lib, "lib/hv_static.lib")



#define TEST_SSL        0
#define TEST_AUTH       1
#define TEST_RECONNECT  1

mymqtt_print_t g_mqtt_print = (mymqtt_print_t)printf;
static void on_mqtt(mqtt_client_t* cli, int type)
{
    g_mqtt_print("on_mqtt type=%d\n", type);
    switch(type) {
    case MQTT_TYPE_CONNECT:
        g_mqtt_print("mqtt connected!\n");
        break;
    case MQTT_TYPE_DISCONNECT:
        g_mqtt_print("mqtt disconnected!\n");
    {
        mqtt_message_t* msg = (mqtt_message_t*)mqtt_client_get_userdata(cli);
        HV_FREE(msg);
        mqtt_client_set_userdata(cli, NULL);
        mqtt_client_stop(cli);
    }
        break;
    case MQTT_TYPE_CONNACK:
        g_mqtt_print("mqtt connack!\n");
    {
        mqtt_message_t* msg = (mqtt_message_t*)mqtt_client_get_userdata(cli);
        if (msg == NULL) return;
        int mid = mqtt_client_publish(cli, msg);
        g_mqtt_print("mqtt publish mid=%d\n", mid);
        if (msg->qos == 0) {
            mqtt_client_disconnect(cli);
        } else if (msg->qos == 1) {
            // wait MQTT_TYPE_PUBACK
        } else if (msg->qos == 2) {
            // wait MQTT_TYPE_PUBREC
        }
    }
        break;
    case MQTT_TYPE_PUBACK: /* qos = 1 */
        g_mqtt_print("mqtt puback mid=%d\n", cli->mid);
        mqtt_client_disconnect(cli);
        break;
    case MQTT_TYPE_PUBREC: /* qos = 2 */
        g_mqtt_print("mqtt pubrec mid=%d\n", cli->mid);
        // wait MQTT_TYPE_PUBCOMP
        break;
    case MQTT_TYPE_PUBCOMP: /* qos = 2 */
        g_mqtt_print("mqtt pubcomp mid=%d\n", cli->mid);
        mqtt_client_disconnect(cli);
        break;
    default:
        break;
    }
}
static DWORD WINAPI mqttThread(LPVOID lpParam)
{
    mqtt_client_t* mc = (mqtt_client_t*)lpParam;

    g_mqtt_print("___mqtt run start\n");
    mqtt_client_run(mc);
    g_mqtt_print("___mqtt run quit\n");

    return 0;
}

myMqtt::myMqtt()
{
    printFunc = (mymqtt_print_t)printf;
    mc = mqtt_client_new(NULL);
    
}

myMqtt::~myMqtt() {
    if (mc) {
        disconn();
        mqtt_client_free(mc);
        mc = NULL;
    }
}


int myMqtt::set_print(mymqtt_print_t fn)
{
	printFunc = fn;
    g_mqtt_print = fn;
	return 0;
}


int myMqtt::conn(char *host, int port, char *username, char *password)
{
    int r;
    reconn_setting_t reconn;
    
    reconn_setting_init(&reconn);
    reconn.min_delay = 1000;
    reconn.max_delay = 10000;
    reconn.delay_policy = 2;
    
    mc->keepalive = 10;
    
    mqtt_client_set_reconnect(mc, &reconn);
    mqtt_client_set_connect_timeout(mc, 500);
    
    mqtt_client_set_id(mc, "test 000");

    mqtt_client_set_auth(mc, username, password);
    
    mqtt_client_set_callback(mc, on_mqtt);
    mqtt_client_set_userdata(mc, NULL);

    r = mqtt_client_connect(mc, host, port, 0);
    printFunc("____ mqtt conn %d\n", r);
    
    hThread = ::CreateThread(NULL, NULL, mqttThread, (LPVOID)mc, 0, &dwThreadId);
    if (!hThread) {
        printFunc("mqtt thread creat failed!\n");
        return -1;
    }
    
    return 0;
}


int myMqtt::disconn(void)
{
    mqtt_client_stop(mc);
    mqtt_client_disconnect(mc);
    return 0;
}


int myMqtt::is_connected(void)
{
    return mqtt_client_is_connected(mc);
}


int myMqtt::publish(char *topic, char *payload)
{
    int r;
    mqtt_message_t msg;
    memset(&msg, 0, sizeof(msg));
        
    msg.topic = topic;
    msg.topic_len = strlen(topic);
    msg.payload = payload;
    msg.payload_len = strlen(payload);
    msg.qos = 2;
    msg.retain = 0;
    r = mqtt_client_publish(mc, &msg);
    
    return r;
}


int myMqtt::subscribe(char* topic)
{
    mqtt_client_subscribe(mc, topic, 2);
   
    return 0;
}




