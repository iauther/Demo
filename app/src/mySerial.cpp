#include "mySerial.h"

#define USE_SELF


#ifdef USE_SELF
static CSerial     mSerial;
#else
#pragma comment(lib,"libcserialport.lib") 

using namespace std;
using namespace itas109;

static CSerialPort mSerial;
#endif

mySerial::mySerial()
{
    nPort = 0;
}

mySerial::~mySerial()
{

}


int mySerial::open(int id)
{
    bool r=0;

    if (nPort <= 0) {
        list(NULL);
    }

    if (nPort<0) {
        return -1;
    }

    if (id>= nPort) {
        return -1;
    }

#ifdef USE_SELF

#else
    mSerial.setReadIntervalTimeout(1);
    mSerial.setOperateMode(itas109::SynchronousOperate);
    mSerial.init(sInfo[id].portName,             // windows:COM1 Linux:/dev/ttyS0
        itas109::BaudRate115200,                    // baudrate
        itas109::ParityNone,                        // parity
        itas109::DataBits8,                         // data bit
        itas109::StopOne,                   // stop bit
        itas109::FlowNone,                // flow
        4096                            // read buffer size
    );
    mSerial.setReadIntervalTimeout(1);
    r = mSerial.open();
#endif

    return  r?0:-1;
}

int mySerial::open(char *port)
{
    bool r = 0;

#ifdef USE_SELF
    r = mSerial.open(port, 115200);
#else
    mSerial.setReadIntervalTimeout(1);
    mSerial.setOperateMode(itas109::SynchronousOperate);
    mSerial.init((const char*)port,
        itas109::BaudRate115200,                    // baudrate
        itas109::ParityNone,                        // parity
        itas109::DataBits8,                         // data bit
        itas109::StopOne,                   // stop bit
        itas109::FlowNone,                // flow
        4096                            // read buffer size
    );
    r = mSerial.open();
#endif

    return  r ? 0 : -1;
}

int mySerial::list(sp_info_t**info)
{
#ifdef USE_SELF
    SerialPortInfo* vInfo;
    mSerial.get_list(&vInfo, &nPort);
#else
    std::vector<itas109::SerialPortInfo>  vInfo;

    vInfo = CSerialPortInfo::availablePortInfos();
    nPort =(vInfo.size()> SPORT_MAX)? SPORT_MAX : vInfo.size();
#endif

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
#ifdef USE_SELF
    return mSerial.is_opened();
#else
    return  (int)mSerial.isOpen();
#endif
}

int mySerial::close(void)
{
    mSerial.close();
    return 0;
}

int mySerial::write(void* data, int len)
{
#ifdef USE_SELF
    return mSerial.write(data, len);
#else
    return mSerial.writeData(data, len);
#endif
}


int mySerial::read(void* data, int len)
{
#ifdef USE_SELF
    return mSerial.read(data, len, 1);
#else
    return mSerial.readData(data, len);
#endif
}

