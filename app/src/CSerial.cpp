#include "CSerial.h"
#include <tchar.h>
#include <setupapi.h>
#include <iostream>

using namespace std;

#pragma warning(disable: 4800)    //disable waring of TRUE => true
#pragma warning(disable: 4267)    //disable waring of size_t => int


#pragma comment(lib, "setupapi.lib")


string wstringToString(const wstring& wstr)
{
    // https://stackoverflow.com/questions/4804298/how-to-convert-wstring-into-string
    if (wstr.empty())
    {
        return string();
    }

    int size = WideCharToMultiByte(CP_ACP, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    string ret = string(size, 0);
    WideCharToMultiByte(CP_ACP, 0, &wstr[0], (int)wstr.size(), &ret[0], size, NULL, NULL); // CP_UTF8

    return ret;
}


static int enumSerialPorts(sp_info2_t* portInfoList, int cnt)
{
    // https://docs.microsoft.com/en-us/windows/win32/api/setupapi/nf-setupapi-setupdienumdeviceinfo
    int nport = 0;
    bool bRet = false;
    sp_info2_t portInfo;

    string strFriendlyName;
    string strPortName;

    HDEVINFO hDevInfo = INVALID_HANDLE_VALUE;

    // Return only devices that are currently present in a system
    // The GUID_DEVINTERFACE_COMPORT device interface class is defined for COM ports. GUID
    // {86E0D1E0-8089-11D0-9CE4-08003E301F73}
    hDevInfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_COMPORT, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

    if (INVALID_HANDLE_VALUE != hDevInfo) {
        SP_DEVINFO_DATA devInfoData;
        // The caller must set DeviceInfoData.cbSize to sizeof(SP_DEVINFO_DATA)
        devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfoData); i++) {
            // get port name
            TCHAR portName[256];

            if (nport >= cnt) {
                return nport;
            }

            HKEY hDevKey = SetupDiOpenDevRegKey(hDevInfo, &devInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
            if (INVALID_HANDLE_VALUE != hDevKey) {
                DWORD dwCount = 255; // DEV_NAME_MAX_LEN
                RegQueryValueEx(hDevKey, _T("PortName"), NULL, NULL, (BYTE*)portName, &dwCount);
                RegCloseKey(hDevKey);
            }

            // get friendly name
            TCHAR fname[256];
            SetupDiGetDeviceRegistryProperty(hDevInfo, &devInfoData, SPDRP_FRIENDLYNAME, NULL, (PBYTE)fname,
                sizeof(fname), NULL);

#ifdef UNICODE
            strPortName = wstringToString(portName);
            strFriendlyName = wstringToString(fname);
#else
            strPortName = string(portName);
            strFriendlyName = string(fname);
#endif
            // remove (COMxx)
            strFriendlyName = strFriendlyName.substr(0, strFriendlyName.find(("(COM")));

            portInfo.portName = strPortName;
            portInfo.description = strFriendlyName;
            portInfoList[nport++] = portInfo;
        }

        if (ERROR_NO_MORE_ITEMS == GetLastError()) {
            //
        }
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);

    return nport;
}






CSerial::CSerial(void) : m_hComm(NULL), m_hThread(NULL), m_bListen(false), m_nNotifyNum(1)
{
    memset(&m_WaitOverlapped,  0, sizeof(OVERLAPPED));
    memset(&m_ReadOverlapped,  0, sizeof(OVERLAPPED));
    memset(&m_WriteOverlapped, 0, sizeof(OVERLAPPED));

    m_WaitOverlapped.hEvent  = CreateEvent(NULL, TRUE, FALSE, NULL);
    m_ReadOverlapped.hEvent  = CreateEvent(NULL, TRUE, FALSE, NULL);
    m_WriteOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    m_hCommEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	m_hComm = NULL;
	m_hCommEvent = NULL;
	m_hThread = NULL;
	m_bListen = 0;
	m_nNotifyNum = 1;
	m_rxCallback = NULL;
	m_Context = NULL;
	
	m_DCB.BaudRate = BaudRate115200;
	m_DCB.ByteSize = DataBits8;
	m_DCB.Parity = ParityNone;
	m_DCB.StopBits = StopOne;
	m_DCB.fBinary = true;
	m_DCB.fInX = false;
	m_DCB.fOutX = false;
	m_DCB.fAbortOnError = false;
	m_DCB.fNull = false;
}


bool CSerial::open(LPCTSTR port, int baudrate)
{
	DWORD err;
	TCHAR xport[100];

    if (m_hComm)
        return false;    //串口已打开

    _stprintf_s(xport, _T("\\\\.\\%s"), port);
	m_hComm = CreateFile(xport,
                         GENERIC_READ | GENERIC_WRITE,
                         0,
                         0,
                         OPEN_EXISTING,
                         FILE_FLAG_OVERLAPPED,
                         0
                         );

    bool bOpened = (m_hComm != INVALID_HANDLE_VALUE);
    
	if (!bOpened) {
		m_hComm = NULL;
		err = GetLastError();
	}
	else
    {
        PurgeComm(m_hComm, PURGE_TXCLEAR | PURGE_RXCLEAR);

        bool bState = setup(baudrate) && set_buflen() && set_mask() && set_timeout();
        bOpened = bState;
        

        if (!bState)
            close();
    }

    return bOpened;
}

bool CSerial::get_list(sp_info2_t**plist, int *cnt)
{
    m_PortNum = enumSerialPorts(m_PortInfo, MAX_PORT);

    *plist = m_PortInfo;
    *cnt = m_PortNum;

    return true;
}


bool CSerial::setup(int baudrate, char bytesize, char parity, char stopbits)
{
    if(!m_hComm)
        return false;

	//if (!GetCommState(m_hComm, &m_DCB))
    //    return false;

	m_DCB.BaudRate = baudrate;
	m_DCB.ByteSize = bytesize;
	m_DCB.Parity   = parity;
	m_DCB.StopBits = stopbits;
    
	return SetCommState(m_hComm, &m_DCB);
}
    
    
bool CSerial::set_buflen(int rxLen, int txLen)
{
    if(!m_hComm)
        return false;

    return SetupComm(m_hComm, rxLen, txLen);
}

   
bool CSerial::close(void)
{
    if(!m_hComm)
        return true;
    
    stop_listen();
    CloseHandle(m_hComm);
    m_hComm = NULL;

    return true;
}


bool CSerial::clear()
{
    DWORD   dwError;
    COMSTAT state;

    if (!m_hComm)
        return true;

    ClearCommError(m_hComm, &dwError, &state);

    return PurgeComm(m_hComm, PURGE_TXABORT | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_RXCLEAR);
}

    
int CSerial::read(void* buf, int len, int waitms)
{
    BOOL    r;
    DWORD   err;
    DWORD   dwMask=0;
    DWORD   dwRead=0;
    DWORD   dwError=0;
    DWORD   dwWaitfor;
    COMSTAT state;
    int     rlen=0;

    if (!m_hComm)
        return 0;

    if (!ClearCommError(m_hComm, &dwError, &state)) {
        PurgeComm(m_hComm, PURGE_RXABORT | PURGE_RXCLEAR);
        return 0;
    } 

    //缓冲无数据
    if (!state.cbInQue) {
        if (waitms>0) {
            if (WaitCommEvent(m_hComm, &dwMask, &m_ReadOverlapped)) {
                ClearCommError(m_hComm, &dwError, &state);
            }
            else {
                if (GetLastError() == ERROR_IO_PENDING) {
                    dwWaitfor = WaitForSingleObject(m_ReadOverlapped.hEvent, waitms);
                    if (dwWaitfor == WAIT_TIMEOUT) {
                        return -1;
                    }
                }
            }
        }
    }

    dwRead = 0;
    while (ClearCommError(m_hComm, &dwError, &state)) {
        if (!state.cbInQue) {
            break;
        }

        r = ReadFile(m_hComm, (BYTE*)buf+rlen, len-rlen, &dwRead, &m_ReadOverlapped);
        if (!r) {
            if (GetLastError() == ERROR_IO_PENDING) {
                GetOverlappedResult(m_hComm, &m_ReadOverlapped, &dwRead, TRUE);
            }
        }
        else {
            rlen += dwRead;
        }
        Sleep(1);
    }

    return rlen;
}


int CSerial::write(const void* buf, int len)
{
    if(!m_hComm)
        return 0;

    DWORD dwWritten;
    DWORD dwError;

    if (!ClearCommError(m_hComm, &dwError, NULL)) {
        PurgeComm(m_hComm, PURGE_TXABORT | PURGE_TXCLEAR);
        return 0;
    }

    if (!WriteFile(m_hComm, buf, len, &dwWritten, &m_WriteOverlapped)) {
        if (GetLastError() != ERROR_IO_PENDING)
            return 0;
        else {
            if (!GetOverlappedResult(m_hComm,&m_WriteOverlapped, &dwWritten, TRUE))
                return 0;                
        }
    }

    return dwWritten;
}


void CSerial::set_notify_len(int len)
{
    m_nNotifyNum = len;
}


bool CSerial::start_listen()
{
    if (m_hThread)
        return true;

    m_bListen = true;
    m_hThread = CreateThread(NULL, NULL, threadProc, this, NULL, NULL);
    
    return m_hThread != NULL;
}


bool CSerial::stop_listen()
{
    if (!m_hThread)
        return true;

    m_bListen = false;
    SetEvent(m_WaitOverlapped.hEvent);
    bool bDead = WaitForSingleObject(m_hThread, INFINITE) == WAIT_OBJECT_0;

    CloseHandle(m_hThread);
    m_hThread = NULL;

    return bDead;
}


void CSerial::set_rx_callback(rx_callback_t callback, PVOID pContext)
{
	m_rxCallback = callback;
	m_Context = pContext;
}

CSerial::~CSerial(void)
{
    if (m_hComm)
        SetCommMask(m_hComm, 0);

    CloseHandle(m_ReadOverlapped.hEvent);
    CloseHandle(m_WriteOverlapped.hEvent);
    CloseHandle(m_WaitOverlapped.hEvent);
    CloseHandle(m_hCommEvent);

    close();
}
    


DWORD WINAPI CSerial::threadProc(void* lpParam)
{
    return ((CSerial*)lpParam)->listen();
}


inline int CSerial::listen()
{
	bool     r;
    DWORD    dwRead;
    DWORD    dwMask=0;
    DWORD    dwError;
    COMSTAT  state;

    if (!m_hComm)
        return 1;

    if (!ClearCommError(m_hComm, &dwError, &state))
        PurgeComm(m_hComm, PURGE_RXABORT | PURGE_RXCLEAR);

    while(m_bListen) {            
        if (!state.cbInQue) {                
            if (!WaitCommEvent(m_hComm, &dwMask, &m_ReadOverlapped)) {
                //WaitCommEvent检查完即退出,如果出错就设置对应事件为未触发状态
                if (GetLastError() == ERROR_IO_PENDING)                    
                    GetOverlappedResult(m_hComm, &m_WaitOverlapped, &dwRead, TRUE);
            }                            
        }
        
        r = ClearCommError(m_hComm, &dwError, &state);
		//OutputDebugString("________ cbInQue: %d\n", state.cbInQue);
		if (r==true && state.cbInQue >= m_nNotifyNum && m_rxCallback) {
			m_rxCallback(state.cbInQue, m_Context);
        }
    }

    return 0;
}

inline bool CSerial::set_timeout(int ms)
{
    if (!m_hComm)
        return false;

    COMMTIMEOUTS timeouts;

    timeouts.ReadIntervalTimeout = MAXDWORD;          //ReadFile马上返回
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.WriteTotalTimeoutConstant = 0;     //单次写超时时间
    timeouts.WriteTotalTimeoutMultiplier = 0;

    return SetCommTimeouts(m_hComm, &timeouts);
}

//设置事件掩码
inline bool CSerial::set_mask()
{    
    if (!m_hComm)
        return false;

    return SetCommMask(m_hComm, EV_RXCHAR| EV_RXFLAG);
    //return SetCommMask(m_hComm, EV_RXCHAR | EV_ERR);
}


bool CSerial::is_opened(void)
{
    return (m_hComm != NULL);
}