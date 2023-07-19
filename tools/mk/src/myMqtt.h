#ifndef __MYMQTT_Hx__
#define __MYMQTT_Hx__

#include "mqtt_client.h"

typedef void (*mymqtt_print_t)(const char* format, ...);


class myMqtt {
public:
    myMqtt();
    ~myMqtt();
    
    int set_print(mymqtt_print_t fn);
    int conn(char *host, int port, char* username, char* password);
    int disconn(void);
    int is_connected(void);
    int publish(char *topic, char *payload);
    int subscribe(char* topic);
    
private:
    mqtt_client_t *mc;
    HANDLE hThread;
    DWORD dwThreadId;
    mymqtt_print_t printFunc;
};


#endif

