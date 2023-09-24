#ifndef __MYAMQP_Hx__
#define __MYAMQP_Hx__

#include <windows.h>
#include "log.h"

//#define USE_AMQP_CPP
#define USE_RABBITMQ_C
//#define USE_AZURE_UMAQP_C

#ifdef USE_AMQP_CPP
#include "uv.h"
#include "amqpcpp.h"
#include "amqpcpp/libuv.h"
#elif defined USE_RABBITMQ_C
#include <string.h>
#include "rabbitmq-c/amqp.h"
#include "rabbitmq-c/tcp_socket.h"
#elif defined USE_AZURE_UMAQP_C
//#include "azure_uamqp_c/uamqp.h"
#endif

//对于Java、.NET、Python 2.7、Node.js、Go客户端：端口号为5671。
//对于Python3、PHP客户端：端口号为61614

//主账号
//userName = clientId | iotInstanceId = ${ iotInstanceId }, authMode = aksign, signMethod = hmacsha1, consumerGroupId = ${ consumerGroupId }, authId = ${ accessKey }, timestamp = 1573489088171 |
//password = signMethod(stringToSign, accessSecret)

//RAM账号
//userName = clientId | iotInstanceId = ${ iotInstanceId }, authMode = ststoken, securityToken = ${ SecurityToken }, signMethod = hmacsha1, consumerGroupId = ${ consumerGroupId }, authId = ${ accessKey }, timestamp = 1573489088171 |
//password = signMethod(stringToSign, accessSecret)


class myAmqp
{
public:
	int conn(const char *host, int port);
	int disconn(void);
	int read(void *data, int len);
	int write(void* data, int len);


private:
	HANDLE hThread;

#ifdef USE_AMQP_CPP

#elif defined USE_RABBITMQ_C
	amqp_connection_state_t hconn;
	amqp_socket_t* socket;
	amqp_bytes_t queuename;
#elif defined USE_AZURE_UMAQP_C

#endif

};

#endif


































