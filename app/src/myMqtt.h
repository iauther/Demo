#ifndef __MYMQTT_Hx__
#define __MYMQTT_Hx__

#include "mqtt_client.h"
#include "MQTTClient.h"

typedef void (*mymqtt_print_t)(const char* format, ...);

//#define USE_HV

#ifndef USE_HV
typedef struct {
    MQTTClient_disconnected     *disconn;
    MQTTClient_messageArrived   *arrived;
    MQTTClient_deliveryComplete *complete;
    MQTTClient_connectionLost   *lostconn;
    MQTTClient_published        *published;

    MQTTClient_traceCallback    *trace;
}mqtt_callback_t;
#endif



class myMqtt {
public:
    myMqtt();
    ~myMqtt();
    
    int set_print(mymqtt_print_t fn);
    int conn(char *host, int port, char* cliendID, char* username, char* password);
    int disconn(void);
    int is_connected(void);
    int publish(char *topic, char *payload, int qos);
    int subscribe(char* topic);

#ifndef USE_HV
    int set_cb(mqtt_callback_t *cb);
#endif

private:

#ifdef USE_HV
    mqtt_client_t *mc;
#else
    MQTTClient mc;
    mqtt_callback_t mcb;
#endif

    HANDLE hThread;
    DWORD dwThreadId;
    mymqtt_print_t printFunc;


    


};


#endif

