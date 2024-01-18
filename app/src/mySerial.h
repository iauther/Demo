#ifndef __MYSERIAL_Hx__
#define __MYSERIAL_Hx__

#include <windows.h>

#include "CSerial.h"
#include "SerialPort.h"
#include "SerialPortInfo.h"

#define SPORT_MAX   20

typedef itas109::SerialPortInfo sp_info_t;

class mySerial
{
public:
    mySerial();
    ~mySerial();


    int open(int id);
    int open(char* port);
    int close(void);
    int is_opened(void);
    
    int write(void* data, int len);
    int read(void* data, int len);
    int list(sp_info_t** info);
    

private:
    int            nPort;
    sp_info_t      sInfo[SPORT_MAX];
};


#endif