#include "port.h"
#include "windows.h"

#define PORT_MAX  20
static CSerialPort mSerial;
vector<SerialPortInfo> portList;

#pragma comment(lib,"libcserialport.lib") 


static vector<SerialPortInfo> getPortInfo(void)
{
    portList = CSerialPortInfo::availablePortInfos();
    return portList;
}


int port_open(int id)
{
    if (portList.empty()) {
        getPortInfo();
        if (portList.empty()) {
            return -1;
        }

        if (id>=portList.size()) {
            return -1;
        }
    }

    mSerial.init(portList[id].portName,             // windows:COM1 Linux:/dev/ttyS0
        itas109::BaudRate115200,                    // baudrate
        itas109::ParityNone,                        // parity
        itas109::DataBits8,                         // data bit
        itas109::StopOne,                   // stop bit
        itas109::FlowNone,                // flow
        4096                            // read buffer size
    );
    mSerial.setReadIntervalTimeout(1);
    mSerial.open();

    return  0;// mSerial.open(port, 115200);
}

vector<SerialPortInfo> port_info(void)
{
    return getPortInfo();
}

int port_is_opened(void)
{
    return  (int)mSerial.isOpen();
}

int port_close(void)
{
    mSerial.close();
    return 0;
}

int port_send(void* data, int len)
{
    return mSerial.writeData(data, len);;
}


int port_recv(void* data, int len)
{
    return mSerial.readData(data, len);
}

