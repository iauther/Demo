#include <windows.h>
#include "serial.h"

typedef struct {
    HANDLE        hComm;
    HANDLE        hMutex;
    OVERLAPPED    rol;
    OVERLAPPED    wol;

    int			  bOpened;
    int           bAsync;
}serial_handle_t;


static int do_close(serial_handle_t *h)
{
    CloseHandle(h->hComm);

    h->hComm = NULL;
    h->bOpened = 0;
    return 0;
}
static int do_setup(HANDLE hcom, int baudrate, int bytesize, int parity, int stopbits)
{
    BOOL r;
    DCB  dcb;

    GetCommState(hcom, &dcb);
    dcb.BaudRate = baudrate;
    dcb.ByteSize = bytesize;
    dcb.Parity = parity;
    dcb.StopBits = stopbits;
    r = SetCommState(hcom, &dcb);

    return (r == TRUE) ? 0 : -1;
}



handle_t serial_init(void)
{
    serial_handle_t* h = calloc(1, sizeof(serial_handle_t));
    if (!h) {
        return NULL;
    }

    h->hComm = NULL;
    h->bOpened = 0;
    h->rol.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    h->wol.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    h->hMutex = CreateMutex(NULL, FALSE, NULL);

    return h;
}


int serial_free(handle_t* h)
{
    serial_handle_t** sh=(serial_handle_t**)h;

    if (!sh || !(*sh)) {
        return -1;
    }

    CloseHandle((*sh)->hMutex);
    CloseHandle((*sh)->rol.hEvent);
    CloseHandle((*sh)->wol.hEvent);
    do_close(*sh);

    serial_close(*sh);

    return -1;
}

int serial_open(handle_t h, int port, int baudrate, int async)
{
    int r = 0;
    DWORD  err, flag;
    COMMTIMEOUTS to = { 0 };
    TCHAR portName[100];
    serial_handle_t* sh = (serial_handle_t*)h;

    if (!sh) {
        return -1;
    }

    WaitForSingleObject(sh->hMutex, 0);
    sh->bAsync = async; wsprintf(portName, L"%s%d", L"\\\\.\\COM", port);
    flag = (async > 0) ? FILE_FLAG_OVERLAPPED : 0;

    do_close(sh);
    sh->hComm = CreateFile(portName, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, flag, 0);
    if (sh->hComm) {
        SetCommMask(sh->hComm, EV_RXCHAR | EV_ERR);
        SetupComm(sh->hComm, 4000, 4000);

        to.ReadIntervalTimeout = 1;
        SetCommTimeouts(sh->hComm, &to);

        do_setup(sh->hComm, baudrate, DataBits8, ParityNone, StopOne);
        PurgeComm(sh->hComm, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);
    }
    else {
        r = -1;
    }

    ReleaseMutex(sh->hMutex);
    sh->bOpened = 1;

    return r;
}
int serial_close(handle_t h)
{
    serial_handle_t* sh = (serial_handle_t*)h;

    if (!sh || !sh->hComm) {
        return -1;
    }

    WaitForSingleObject(sh->hMutex, 0);
    do_close(sh);
    ReleaseMutex(sh->hMutex);

    return 0;
}

int serial_setup(handle_t h, int baudrate, int bytesize, int parity, int stopbits)
{
    int r;
    serial_handle_t* sh = (serial_handle_t*)h;

    if (!sh || !sh->hComm) {
        return -1;
    }

    WaitForSingleObject(sh->hMutex, 0);
    r = do_setup(sh->hComm, baudrate, DataBits8, ParityNone, StopOne);
    ReleaseMutex(sh->hMutex);

    return r;
}


int  serial_read(handle_t h, void* data, int len)
{
    DWORD err, rl, rlen = 0;
    serial_handle_t* sh = (serial_handle_t*)h;

    if (!sh || !sh->hComm || !sh->bOpened) {
        return -1;
    }

    WaitForSingleObject(sh->hMutex, 0);
    if (sh->bAsync) {
        if (!ReadFile(sh->hComm, data, len, &rlen, &sh->rol)) {
            if (GetLastError() != ERROR_IO_PENDING) {
                while (!GetOverlappedResult(sh->hComm, &sh->rol, &rlen, TRUE)) {
                    break;
                }
            }
        }
    }
    else {
        COMSTAT stat;
        //ClearCommError(hComm, &err, &stat);
        //rl = (stat.cbInQue > len) ? len : stat.cbInQue;
        if (!ReadFile(sh->hComm, data, len, &rlen, NULL)) {
            rlen = 0;
        }
    }
    ReleaseMutex(sh->hMutex);

    return rlen;



}
int  serial_write(handle_t h, void* data, int len)
{
    DWORD err, wlen = 0;
    serial_handle_t* sh = (serial_handle_t*)h;

    if (!sh || !sh->hComm || !sh->bOpened) {
        return -1;
    }

    WaitForSingleObject(sh->hMutex, 0);
    if (sh->bAsync) {
        if (!WriteFile(sh->hComm, data, len, &wlen, &sh->wol)) {
            if (GetLastError() != ERROR_IO_PENDING) {
                while (!GetOverlappedResult(sh->hComm, &sh->wol, &wlen, TRUE)) {
                    break;
                }
            }
        }
    }
    else {
        if (!WriteFile(sh->hComm, data, len, &wlen, NULL)) {
            wlen = 0;
        }
    }
    ReleaseMutex(sh->hMutex);

    return wlen;
}
int  serial_is_opened(handle_t h)
{
    serial_handle_t* sh = (serial_handle_t*)h;

    if (!sh) {
        return -1;
    }

    return sh->bOpened;
}


