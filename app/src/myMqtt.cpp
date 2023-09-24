#include <stdio.h>
#include "myMqtt.h"
#include "log.h"

#define RX_BUF_LEN   8000


#ifdef USE_HV
#include "hv.h"
//#pragma comment(lib, "lib/hv.lib")
#pragma comment(lib, "lib/hv_static.lib")


#define TEST_SSL        0
#define TEST_AUTH       1
#define TEST_RECONNECT  1
static void on_mqtt(mqtt_client_t* cli, int type)
{
    LOGD ("on_mqtt type=%d\n", type);
    switch(type) {
    case MQTT_TYPE_CONNECT:
        LOGD("mqtt connected!\n");
        break;
    case MQTT_TYPE_DISCONNECT:
        LOGD("mqtt disconnected!\n");
    {
        mqtt_message_t* msg = (mqtt_message_t*)mqtt_client_get_userdata(cli);
        HV_FREE(msg);
        mqtt_client_set_userdata(cli, NULL);
        mqtt_client_stop(cli);
    }
        break;
    case MQTT_TYPE_CONNACK:
        LOGD("mqtt connack!\n");
    {
        mqtt_message_t* msg = (mqtt_message_t*)mqtt_client_get_userdata(cli);
        if (msg == NULL) return;
        int mid = mqtt_client_publish(cli, msg);
        LOGD("mqtt publish mid=%d\n", mid);
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
        LOGD("mqtt puback mid=%d\n", cli->mid);
        mqtt_client_disconnect(cli);
        break;
    case MQTT_TYPE_PUBREC: /* qos = 2 */
        LOGD("mqtt pubrec mid=%d\n", cli->mid);
        // wait MQTT_TYPE_PUBCOMP
        break;
    case MQTT_TYPE_PUBCOMP: /* qos = 2 */
        LOGD("mqtt pubcomp mid=%d\n", cli->mid);
        mqtt_client_disconnect(cli);
        break;
    default:
        break;
    }
}
#else
#pragma comment(lib, "lib/paho-mqtt3c.lib")
#endif

typedef struct {
#ifdef USE_HV
    mqtt_client_t* mc;
#else
    MQTTClient     mc;
#endif

    mqtt_callback_t mcb;

    HANDLE  hMutex;
    HANDLE  hThread;
    DWORD   threadId;
    int     quit;

    int     dlen;
    char    buffer[RX_BUF_LEN];
}mqtt_conn_t;


static void lock_new(void* conn)
{
    if (conn) {
        mqtt_conn_t* hconn = (mqtt_conn_t*)conn;
        hconn->hMutex = CreateMutex(nullptr, FALSE, NULL);
    }
}
static void lock_free(void* conn)
{
    if (conn) {
        mqtt_conn_t* hconn = (mqtt_conn_t*)conn;
        CloseHandle(hconn->hMutex);
    }
}

static void lock(void* conn)
{
    if (conn) {
        mqtt_conn_t* hconn = (mqtt_conn_t*)conn;
        WaitForSingleObject(hconn->hMutex, INFINITE);
    }
}
static void unlock(void* conn)
{
    if (conn) {
        mqtt_conn_t* hconn = (mqtt_conn_t*)conn;
        ReleaseMutex(hconn->hMutex);
    }
}


static DWORD WINAPI mqttThread(LPVOID lpParam)
{
    mqtt_conn_t* conn=(mqtt_conn_t*)lpParam;

    LOGD("___mqtt run start\n");

#ifdef USE_HV
    mqtt_client_run(cli->mc);
#else
    while(conn && conn->quit==0) {
        
        Sleep(2);
    }
#endif

    LOGD("___mqtt run quit\n");

    return 0;
}

#ifdef USE_HV

#else
static void msg_published(void* conn, int dt, int packet_type, MQTTProperties* properties, enum MQTTReasonCodes reasonCode)
{
    mqtt_conn_t* hconn = (mqtt_conn_t*)conn;


}
static int msg_arrived(void* conn, char* topicName, int topicLen, MQTTClient_message* m)
{
    mqtt_conn_t* hconn = (mqtt_conn_t*)conn;

    LOGD("recv: %s\n", (char*)m->payload);

    lock(hconn);
    memcpy(hconn->buffer, m->payload, m->payloadlen);
    hconn->dlen = m->payloadlen;
    unlock(hconn);

    MQTTClient_freeMessage(&m);
    MQTTClient_free(topicName);

    return 1;
}
static void msg_complete(void* conn, MQTTClient_deliveryToken dt)
{
    mqtt_conn_t* hconn = (mqtt_conn_t*)conn;

    LOGD("msgComplete\n");

}
static void msg_lostconn(void* conn, char* cause)
{
    mqtt_conn_t* hconn = (mqtt_conn_t*)conn;
}
static void msg_disconn(void* conn, MQTTProperties* properties, enum MQTTReasonCodes reasonCode)
{
    mqtt_conn_t* hconn = (mqtt_conn_t*)conn;
}
#endif

////////////////////////////////////////////////////////////////////////////


myMqtt::myMqtt()
{
#ifdef USE_HV
    
#else
    MQTTClient_global_init(NULL);

    cb.published = msg_published;
    cb.arrived = msg_arrived;
    cb.lostconn = msg_lostconn;
    cb.complete = msg_complete;
    cb.disconn = msg_disconn;
#endif

    
}

myMqtt::~myMqtt()
{
#ifdef USE_HV
    mqtt_client_free(mc);
#else
    
#endif

    
}

void* myMqtt::conn(meta_data_t* md)
{
    sign_data_t sign;

    if (!md) {
        return NULL;
    }

    utils_hmac_sign(md, &sign, SIGN_HMAC_SHA1);
    return conn(md->hostUrl, md->port, sign.clientId, sign.username, sign.password);
}

void* myMqtt::conn(char *host, int port, char *cliendID, char *username, char *password)
{
    int r;
    mqtt_conn_t* hconn = new mqtt_conn_t;

    if (!hconn) {
        return NULL;
    }

#ifdef USE_HV
    reconn_setting_t rc;

    hconn->mc = mqtt_client_new(NULL);
    
    reconn_setting_init(&rc);
    rc.min_delay = 1000;
    rc.max_delay = 10000;
    rc.delay_policy = 2;
    
    hconn->->keepalive = 100;
    
    mqtt_client_set_reconnect(hconn->mc, &rc);
    mqtt_client_set_connect_timeout(hconn->mc, 500);
    
    mqtt_client_set_id(hconn->mc, cliendID);

    mqtt_client_set_auth(hconn->mc, username, password);
    
    mqtt_client_set_callback(hconn->mc, on_mqtt);

    r = mqtt_client_connect(hconn->mc, host, port, 0);
    LOGD("____ mqtt conn %d\n", r);

#else
    MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer;
    MQTTClient_willOptions wopts = MQTTClient_willOptions_initializer;

    r = MQTTClient_create(&hconn->mc, host, cliendID,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (r != MQTTCLIENT_SUCCESS) {
        LOGE("__MQTTClient_create failed, %d\n", r);
        goto failed;
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

    r = MQTTClient_connect(hconn->mc, &opts);
    if (r != MQTTCLIENT_SUCCESS) {
        LOGE("__MQTTClient_connect failed, %d\n", r);
        goto failed;
    }
    LOGD("__MQTTClient_connect ok!\n");
#endif

    r = set_cb(hconn);
    if (r) {
        LOGE("__mqtt set_cb, %d\n", r);
        goto failed;
    }

    lock_new(hconn);
    hconn->quit = 0;
    hconn->dlen = 0;
    hconn->hThread = CreateThread(NULL, NULL, mqttThread, (LPVOID)hconn, 0, &hconn->threadId);
    
    return hconn;

failed:
#ifdef USE_HV

#else
    MQTTClient_destroy(&hconn->mc);
#endif

    return NULL;
}


int myMqtt::disconn(void* conn)
{
    int r = 0;
    mqtt_conn_t* hconn = (mqtt_conn_t*)conn;

    if (!hconn) {
        return -1;
    }

#ifdef USE_HV
    mqtt_client_stop(hconn->mc);
    r = mqtt_client_disconnect(hconn->mc);
#else
    r = MQTTClient_disconnect(hconn->mc, 0);
    if (r!= MQTTCLIENT_SUCCESS) {
       LOGE("MQTTClient_disconnect failed, %d\n", r);
    }
#endif

    MQTTClient_destroy(&hconn->mc);

    hconn->quit = 1;
    WaitForSingleObject(hconn->hThread, INFINITE);
    CloseHandle(hconn->hThread);
    lock_free(hconn);
    delete hconn;

    return r;
}


int myMqtt::is_connected(void *conn)
{
    mqtt_conn_t* hconn = (mqtt_conn_t*)conn;

    if (!hconn) {
        return 0;
    }


#ifdef USE_HV
    return mqtt_client_is_connected(hconn->mc);
#else
    return MQTTClient_isConnected(hconn->mc);
#endif

}


int myMqtt::publish(void* conn, char *topic, char *payload, int qos)
{
    int r=0;
    mqtt_conn_t* hconn = (mqtt_conn_t*)conn;

    if (!hconn) {
        return -1;
    }

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
    r = MQTTClient_publishMessage(hconn->mc, topic, &msg, &dt);
#endif
    
    return r;
}


int myMqtt::subscribe(void* conn, char* topic)
{
    int r;
    mqtt_conn_t* hconn = (mqtt_conn_t*)conn;

    if (!hconn) {
        return -1;
    }

#ifdef USE_HV
    r = mqtt_client_subscribe(hconn->mc, topic, 1);
#else
    r = MQTTClient_subscribe(hconn->mc, topic, 1);
#endif
   
    return r;
}


int myMqtt::read(void* conn, void* buf, int buflen)
{
    int rlen=0;
    mqtt_conn_t* hconn = (mqtt_conn_t*)conn;

    if (!hconn) {
        return -1;
    }

    if (!MQTTClient_isConnected(hconn->mc)) {
        LOGD("__mqtt_read, mqtt not connected!\n");
        return -1;
    }

    lock(hconn);
    if (buflen >= hconn->dlen) {
        rlen = hconn->dlen;
        memcpy(buf, hconn->buffer, rlen);
        hconn->dlen = 0;
    }
    unlock(hconn);

    return rlen;
}


int myMqtt::set_cb(void* conn)
{
    int r=0;
    mqtt_conn_t* hconn = (mqtt_conn_t*)conn;

    if (!hconn) {
        return -1;
    }

#ifdef USE_HV

#else
    MQTTClient_setTraceCallback(cb.trace);

    r = MQTTClient_setPublished(hconn->mc, NULL, cb.published);
    if (r) {
        LOGE("MQTTClient_setPublished failed\n");
        return r;
    }

    r = MQTTClient_setDisconnected(hconn->mc, NULL, cb.disconn);
    if (r) {
        LOGE("MQTTClient_setDisconnected failed\n");
        return r;
    }

    r = MQTTClient_setCallbacks(hconn->mc, NULL, cb.lostconn, cb.arrived, cb.complete);
    if (r) {
        LOGE("MQTTClient_setCallbacks failed\n");
        return r;
    }
#endif

    return r;
}



