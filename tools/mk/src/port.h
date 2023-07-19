#ifndef __PORT_Hx__
#define __PORT_Hx__

#include "types.h"
#include "SerialPort.h"
#include "SerialPortInfo.h"

using namespace std;
using namespace itas109;

int port_open(int id);
int port_close(void);
int port_is_opened(void);
vector<SerialPortInfo> port_info(void);
int port_send(void* data, int len);
int port_recv(void* data, int len);

#endif



