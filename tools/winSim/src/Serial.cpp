#include "Serial.h"


CSerial::CSerial(void)
{
    memset(&rol, 0, sizeof(OVERLAPPED));
    memset(&wol, 0, sizeof(OVERLAPPED));

    hComm = NULL;
    bOpened = 0;
    rol.hEvent  = CreateEvent(NULL, TRUE, FALSE, NULL);
    wol.hEvent  = CreateEvent(NULL, TRUE, FALSE, NULL);
    hMutex = CreateMutex(NULL, FALSE, NULL);
}


int CSerial::Open(int port, int baudrate, int async)
{
    int r=0;
    string  name;
    DWORD  err, flag;
    COMMTIMEOUTS to = { 0 };
    TCHAR portName[100];

    WaitForSingleObject(hMutex, 0);

    bAsync = async; wsprintf(portName, L"%s%d", L"\\\\.\\COM", port);
    flag = (async > 0) ? FILE_FLAG_OVERLAPPED : 0;

    myClose();
    hComm = CreateFile(portName, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, flag, 0);
	if (hComm) {
        SetCommMask(hComm, EV_RXCHAR | EV_ERR);
        SetupComm(hComm, 4000, 4000);

        to.ReadIntervalTimeout = 1;
        SetCommTimeouts(hComm, &to);

        Setup(baudrate, DataBits8, ParityNone, StopOne);
        PurgeComm(hComm, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);
	}
    else {
        r = -1;
    }

    ReleaseMutex(hMutex);
    
    bOpened = 1;

    return r;
}


int CSerial::Setup(DWORD baudrate, BYTE bytesize, BYTE parity, BYTE stopbits)
{
    BOOL r;
    DCB  dcb;

    if(!hComm)
        return -1;

    WaitForSingleObject(hMutex, 0);

    GetCommState(hComm, &dcb);
	dcb.BaudRate = baudrate;
    dcb.ByteSize = bytesize;
    dcb.Parity   = parity;
    dcb.StopBits = stopbits;
	r = SetCommState(hComm, &dcb);

    ReleaseMutex(hMutex);

    return (r == TRUE) ? 0 : -1;
}

   
int CSerial::Close(void)
{
    if(!hComm)
        return -1;

    WaitForSingleObject(hMutex, 0);
    myClose();
    ReleaseMutex(hMutex);

    return 0;
}


int CSerial::Read(void* lpBuffer, DWORD len)
{
    DWORD err, rl,rlen = 0;

    if (!hComm || !bOpened)
        return -1;

    WaitForSingleObject(hMutex, 0);
    if (bAsync) {
        if (!ReadFile(hComm, lpBuffer, len, &rlen, &rol)) {
            if (GetLastError() != ERROR_IO_PENDING) {
                while (!GetOverlappedResult(hComm, &rol, &rlen, TRUE)) {
                    break;
                }
            }
        }
    }
    else{
        COMSTAT stat;
        //ClearCommError(hComm, &err, &stat);
        //rl = (stat.cbInQue > len) ? len : stat.cbInQue;
        if (!ReadFile(hComm, lpBuffer, len, &rlen, NULL)) {
            rlen = 0;
        }
    }
    ReleaseMutex(hMutex);

    return rlen;
}



int CSerial::Write(void* lpBuffer, DWORD len)
{
    DWORD err, wlen = 0;

    if(!hComm || !bOpened)
        return -1;

    WaitForSingleObject(hMutex, 0);
    if (bAsync) {
        if (!WriteFile(hComm, lpBuffer, len, &wlen, &wol)) {
            if (GetLastError() != ERROR_IO_PENDING) {
                while (!GetOverlappedResult(hComm, &wol, &wlen, TRUE)) {
                    break;
                }
            }
        }
    }
    else {
        if (!WriteFile(hComm, lpBuffer, len, &wlen, NULL)) {
            wlen = 0;
        }
    }
    ReleaseMutex(hMutex);

    return wlen;
}


CSerial::~CSerial(void)
{
    Close();
}
    

int CSerial::isOpened(void)
{
    return (hComm && bOpened) ? 1 : 0;
}


int CSerial::myClose(void)
{
    CloseHandle(hComm);
    CloseHandle(rol.hEvent);
    CloseHandle(wol.hEvent);
    hComm = NULL;
    rol.hEvent = NULL;
    wol.hEvent = NULL;
    bOpened = 0;

    return 0;
}