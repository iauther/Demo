#include "mySerial.h"

#pragma comment(lib,"libcserialport.lib") 

using namespace std;
using namespace itas109;

static CSerialPort mSerial;

mySerial::mySerial()
{
    nPort = -1;
}

mySerial::~mySerial()
{

}


int mySerial::open(int id)
{
    if (nPort<0) {
        list(NULL);
        if (nPort<0) {
            return -1;
        }

        if (id>= nPort) {
            return -1;
        }
    }

    mSerial.init(sInfo[id].portName,             // windows:COM1 Linux:/dev/ttyS0
        itas109::BaudRate115200,                    // baudrate
        itas109::ParityNone,                        // parity
        itas109::DataBits8,                         // data bit
        itas109::StopOne,                   // stop bit
        itas109::FlowNone,                // flow
        4096                            // read buffer size
    );
    mSerial.setReadIntervalTimeout(1);
    bool r = mSerial.open();

    return  r?0:-1;
}

int mySerial::list(sp_info_t**info)
{
    std::vector<itas109::SerialPortInfo>  vInfo;

    vInfo = CSerialPortInfo::availablePortInfos();
    nPort =(vInfo.size()> SPORT_MAX)? SPORT_MAX : vInfo.size();

    for(int i=0; i< nPort; i++) {
        sInfo[i] = vInfo[i];
    }
    
    if(info) {
        *info = sInfo;
    }
    
    return nPort;
}

int mySerial::is_opened(void)
{
    return  (int)mSerial.isOpen();
}

int mySerial::close(void)
{
    mSerial.close();
    return 0;
}

int mySerial::write(void* data, int len)
{
    return mSerial.writeData(data, len);;
}


int mySerial::read(void* data, int len)
{
    return mSerial.readData(data, len);
}

