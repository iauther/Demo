#ifndef __MYSERIAL_Hx__
#define __MYSERIAL_Hx__

#include <windows.h>

#include "SerialPort.h"
#include "SerialPortInfo.h"

#define PORT_MAX   20

typedef itas109::SerialPortInfo sp_info_t;

class mySerial
{
public:
    mySerial();
    ~mySerial();


    int open(int id);
    int close(void);
    int is_opened(void);
    int list(sp_info_t** info);
    int write(void* data, int len);
    int read(void* data, int len);

private:
    int            nPort;
    sp_info_t      sInfo[PORT_MAX];
    std::vector<itas109::SerialPortInfo>  vInfo;
};


#endif