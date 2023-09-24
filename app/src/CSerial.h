#ifndef __CSERIAL_Hx__
#define __CSERIAL_Hx__

//#pragma once

#include <string>
#include <windows.h>

#define MAXLEN      4096
#define RITO_TIME   10
#define MAX_PORT	50


typedef void(*rx_callback_t)(DWORD len, PVOID pContext);

typedef struct {
	string portName;
	string description;
}SerialPortInfo;



enum BaudRate
{
	BaudRate110 = 110,       ///< 110
	BaudRate300 = 300,       ///< 300
	BaudRate600 = 600,       ///< 600
	BaudRate1200 = 1200,     ///< 1200
	BaudRate2400 = 2400,     ///< 2400
	BaudRate4800 = 4800,     ///< 4800
	BaudRate9600 = 9600,     ///< 9600 recommend 推荐
	BaudRate14400 = 14400,   ///< 14400
	BaudRate19200 = 19200,   ///< 19200
	BaudRate38400 = 38400,   ///< 38400
	BaudRate56000 = 56000,   ///< 56000
	BaudRate57600 = 57600,   ///< 57600
	BaudRate115200 = 115200, ///< 115200
	BaudRate921600 = 921600  ///< 921600
};


enum DataBits
{
	DataBits5 = 5, ///< 5 data bits 5位数据位
	DataBits6 = 6, ///< 6 data bits 6位数据位
	DataBits7 = 7, ///< 7 data bits 7位数据位
	DataBits8 = 8  ///< 8 data bits 8位数据位
};

enum Parity
{
	ParityNone = 0,  ///< No Parity 无校验
	ParityOdd = 1,   ///< Odd Parity 奇校验
	ParityEven = 2,  ///< Even Parity 偶校验
	ParityMark = 3,  ///< Mark Parity 1校验
	ParitySpace = 4, ///< Space Parity 0校验
};

enum StopBits
{
	StopOne = 0,        ///< 1 stop bit 1位停止位
	StopOneAndHalf = 1, ///< 1.5 stop bit 1.5位停止位 - This is only for the Windows platform
	StopTwo = 2         ///< 2 stop bit 2位停止位
};




class CSerial
{
public:
    // 用于窗体模式的数据到达消息
    enum {WM_ONCOMM = WM_USER + 2005};

	CSerial();
    ~CSerial(void);

	bool open(LPCTSTR port, int baudrate);
    bool close(void);
	bool get_list(SerialPortInfo** plist, int* cnt);
	int  read(void* buf, int len, int waitms = 0);
    int  write(const void* buf, int len);
	bool is_opened(void);
	
    
private:
	bool clear();
    static DWORD WINAPI threadProc(void* lpParam);
	bool setup(int baudrate = 115200, char bytesize = 8, char parity = NOPARITY, char stopbits = ONESTOPBIT);
	bool set_buflen(int rxLen = MAXLEN, int txLen = MAXLEN);
	void set_notify_len(int len);
	bool start_listen();
	bool stop_listen();
	void set_rx_callback(rx_callback_t callback, PVOID pContext);

protected:
    inline int listen();
    inline bool set_timeout(int ms=RITO_TIME);
    inline bool set_mask();

protected:
    HANDLE        m_hComm;    
    HANDLE        m_hCommEvent;
    HANDLE        m_hThread;
    OVERLAPPED    m_ReadOverlapped;
    OVERLAPPED    m_WriteOverlapped;
    OVERLAPPED    m_WaitOverlapped;
    bool		  m_bListen;
	rx_callback_t m_rxCallback;

	PVOID		  m_Context;
	string        m_portName;

	DCB           m_DCB;
    DWORD		  m_nNotifyNum;

	int           m_PortNum;
	SerialPortInfo m_PortInfo[MAX_PORT];
};

#endif
