#include <stdio.h>
#include "hv.h"
#include "myMqtt.h"

//#pragma comment(lib, "lib/hv.lib")

#pragma comment(lib, "lib/hv_static.lib")
#pragma comment(lib, "lib/paho-mqtt3c.lib")



mymqtt_print_t gPrintFunc = (mymqtt_print_t)printf;

#ifdef USE_HV
#define TEST_SSL        0
#define TEST_AUTH       1
#define TEST_RECONNECT  1
static void on_mqtt(mqtt_client_t* cli, int type)
{
    gPrintFunc("on_mqtt type=%d\n", type);
    switch(type) {
    case MQTT_TYPE_CONNECT:
        gPrintFunc("mqtt connected!\n");
        break;
    case MQTT_TYPE_DISCONNECT:
        gPrintFunc("mqtt disconnected!\n");
    {
        mqtt_message_t* msg = (mqtt_message_t*)mqtt_client_get_userdata(cli);
        HV_FREE(msg);
        mqtt_client_set_userdata(cli, NULL);
        mqtt_client_stop(cli);
    }
        break;
    case MQTT_TYPE_CONNACK:
        gPrintFunc("mqtt connack!\n");
    {
        mqtt_message_t* msg = (mqtt_message_t*)mqtt_client_get_userdata(cli);
        if (msg == NULL) return;
        int mid = mqtt_client_publish(cli, msg);
        gPrintFunc("mqtt publish mid=%d\n", mid);
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
        gPrintFunc("mqtt puback mid=%d\n", cli->mid);
        mqtt_client_disconnect(cli);
        break;
    case MQTT_TYPE_PUBREC: /* qos = 2 */
        gPrintFunc("mqtt pubrec mid=%d\n", cli->mid);
        // wait MQTT_TYPE_PUBCOMP
        break;
    case MQTT_TYPE_PUBCOMP: /* qos = 2 */
        gPrintFunc("mqtt pubcomp mid=%d\n", cli->mid);
        mqtt_client_disconnect(cli);
        break;
    default:
        break;
    }
}
#else
static void msgComplete(void* context, MQTTClient_deliveryToken dt)
{
    gPrintFunc("msgComplete\n");
    
}

static int msgArrived(void* context, char* topicName, int topicLen, MQTTClient_message* m)
{
    MQTTClient_free(topicName);
    MQTTClient_freeMessage(&m);
    return 1;
}


#endif

static DWORD WINAPI mqttThread(LPVOID lpParam)
{
    gPrintFunc("___mqtt run start\n");

#ifdef USE_HV
    mqtt_client_t* mc = (mqtt_client_t*)lpParam;
    mqtt_client_run(mc);
#else
    //
#endif
    gPrintFunc("___mqtt run quit\n");

    return 0;
}

myMqtt::myMqtt()
{
#ifdef USE_HV
    mc = mqtt_client_new(NULL);
#else
    MQTTClient_init_options opts;

    memset(&mc, 0, sizeof(mc));
    memset(&mcb, 0, sizeof(mcb));
    MQTTClient_global_init(NULL);
#endif
    printFunc = (mymqtt_print_t)printf;
    hThread = NULL;
    dwThreadId = 0;
}

myMqtt::~myMqtt()
{
#ifdef USE_HV
    if (mc) {
        disconn();
        mqtt_client_free(mc);
        mc = NULL;
    }
#else
    MQTTClient_destroy(&mc);
    //MQTTClient_cleanup();
#endif

}


int myMqtt::set_print(mymqtt_print_t fn)
{
	printFunc = fn;
    gPrintFunc = fn;
	return 0;
}


int myMqtt::conn(char *host, int port, char *cliendID, char *username, char *password)
{
    int r;

#ifdef USE_HV
    reconn_setting_t rc;
    
    reconn_setting_init(&rc);
    rc.min_delay = 1000;
    rc.max_delay = 10000;
    rc.delay_policy = 2;
    
    mc->keepalive = 100;
    
    mqtt_client_set_reconnect(mc, &rc);
    mqtt_client_set_connect_timeout(mc, 500);
    
    mqtt_client_set_id(mc, cliendID);

    mqtt_client_set_auth(mc, username, password);
    
    mqtt_client_set_callback(mc, on_mqtt);

    r = mqtt_client_connect(mc, host, port, 0);
    printFunc("____ mqtt conn %d\n", r);
#else
    MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer;
    MQTTClient_willOptions wopts = MQTTClient_willOptions_initializer;

    r = MQTTClient_create(&mc, host, cliendID,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (r != MQTTCLIENT_SUCCESS) {
        printFunc("__MQTTClient_create failed, %d\n", r);
        MQTTClient_destroy(&mc);
        return -1;
    }

    r = MQTTClient_setCallbacks(mc, NULL, NULL, msgArrived, msgComplete);
    if (r != MQTTCLIENT_SUCCESS) {
        printFunc("__MQTTClient_setCallbacks failed, %d\n", r);
    }

    opts.keepAliveInterval = 80;
    opts.cleansession = 1;
    opts.username = username;
    opts.password = password;
    opts.MQTTVersion = MQTTVERSION_3_1_1;

    opts.will = &wopts;
    opts.will->message = "will message";
    opts.will->qos = 1;
    opts.will->retained = 0;
    opts.will->topicName = "will topic";
    opts.will = NULL;

    r = MQTTClient_connect(mc, &opts);
    if (r != MQTTCLIENT_SUCCESS) {
        printFunc("__MQTTClient_connect failed, %d\n", r);
    }

    printFunc("__MQTTClient_connect ok!\n");
#endif

    hThread = ::CreateThread(NULL, NULL, mqttThread, (LPVOID)mc, 0, &dwThreadId);
    if (!hThread) {
        printFunc("mqtt thread creat failed!\n");
        return -1;
    }
    
    return 0;
}


int myMqtt::disconn(void)
{
    int r = 0;

#ifdef USE_HV
    mqtt_client_stop(mc);
    r = mqtt_client_disconnect(mc);
#else
    r = MQTTClient_disconnect(mc, 0);
    if (r!= MQTTCLIENT_SUCCESS) {
       // printFunc("MQTTClient_disconnect failed, %d\n", r);
    }
#endif

    return r;
}


int myMqtt::is_connected(void)
{
#ifdef USE_HV
    return mqtt_client_is_connected(mc);
#else
    return MQTTClient_isConnected(mc);
#endif

}


int myMqtt::publish(char *topic, char *payload, int qos)
{
    int r=0;

#ifdef USE_HV
    mqtt_message_t msg;
    memset(&msg, 0, sizeof(msg));
        
    msg.topic = topic;
    msg.topic_len = strlen(topic);
    msg.payload = payload;
    msg.payload_len = strlen(payload);
    msg.qos = 2;
    msg.retain = 0;
    r = mqtt_client_publish(mc, &msg);
#else
    MQTTClient_message msg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken dt;

    msg.payload = payload;
    msg.payloadlen = strlen(payload);
    msg.retained = 0;
    msg.qos = qos;
    r = MQTTClient_publishMessage(mc, topic, &msg, &dt);
#endif
    
    return r;
}


int myMqtt::subscribe(char* topic)
{
    int r;

#ifdef USE_HV
    r = mqtt_client_subscribe(mc, topic, 2);
#else
    r = MQTTClient_subscribe(mc, topic, 2);
#endif
   
    return r;
}

#ifndef USE_HV
int myMqtt::set_cb(mqtt_callback_t *cb)
{
    if (!cb) {
        return -1;
    }
    mcb = *cb;

    MQTTClient_setPublished(mc, NULL, mcb.published);
    MQTTClient_setDisconnected(mc, NULL, mcb.disconn);
    MQTTClient_setCallbacks(mc, NULL, mcb.lostconn, mcb.arrived, mcb.complete);
    MQTTClient_setTraceCallback(mcb.trace);

    return 0;
}
#endif
