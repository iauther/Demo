#include "task.h"
#include "date.h"
#include "com.h"

#define TIMER_MS            100

static int dev_exit_flag=0;
static int paras_tx_flag=0;
static int dev_thread_running = 0;
static UINT_PTR timerId;
static HANDLE devRxThreadHandle, devThreadHandle;
extern paras_t DEFAULT_PARAS;


static int send_stat(U8 st)
{
    stat_t stat;
            
    stat.stat = st;
    stat.temp = 22.34F;
    stat.aPres = 93.6F;
    stat.dPres = 77.2F;
    stat.pump.speed = 500;
    stat.pump.current = 450.0F;
    stat.pump.temp = 44.9F;
    stat.pump.fault = 0;
    stat.pump.cmdAck = 1;
    pkt_send(TYPE_STAT, 0, &stat, sizeof(stat));
    
    return 0;
}


static U8 timer_proc(void)
{
    U8  err;
    int r;   

    if (!port_is_opened()) {
        return 0;
    }
    send_stat(STAT_AUTO);

    err = com_loop_check(1);

    

    return err;
}



static DWORD WINAPI dev_rx_thread(LPVOID lpParam)
{
    U8  err;
    int rlen;
    U16 rxbuflen;
    U8* rxbuf = NULL;

    com_get_buf(&rxbuf, &rxbuflen);
    while (1) {
        rlen = port_recv(rxbuf, rxbuflen);
        if (rlen > 0) {
            err = com_data_proc(rxbuf, rlen);
        }

        if (dev_exit_flag) {
            break;
        }
    }

    return 0;
}


static void dev_post_evt(int evt, void *ptr)
{
    if (dev_thread_running) {
        PostThreadMessage(GetThreadId(devThreadHandle), evt, 0, 0);
    }
}
static void CALLBACK timer_callback(HWND hwnd, UINT message, UINT_PTR iTimerID, DWORD dwTime)
{
    dev_post_evt(WM_USER, NULL);
}


static DWORD WINAPI dev_thread(LPVOID lpParam)
{
    U8  err;
    MSG msg;
    
    PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
    dev_thread_running = 1;

    while (GetMessage(&msg, NULL, 0, 0)) {
        //TranslateMessage(&msg);
        DispatchMessage(&msg);

        if (dev_exit_flag) {
            break;
        }

        switch (msg.message) {

        case WM_USER:
        {
            err = timer_proc();
        }
        break;
        }
        
    }

    return 0;
}


static void dummy_fun(U8 *data, U16 len)
{

}

int task_dev_start(void)
{
    dev_exit_flag = 0;
    dev_thread_running = 0;

    com_init(dummy_fun, TIMER_MS);

    timerId = SetTimer(NULL, 1, 100, timer_callback);
    devRxThreadHandle = CreateThread(NULL, 0, dev_rx_thread, NULL, 0, NULL);
    devThreadHandle = CreateThread(NULL, 0, dev_thread, NULL, 0, NULL);

    return 0;
}

int task_dev_stop(void)
{
    dev_exit_flag = 1;

    KillTimer(NULL, timerId);
    if (devRxThreadHandle) {
        WaitForSingleObject(devRxThreadHandle, INFINITE);
        CloseHandle(devRxThreadHandle);
        devRxThreadHandle = NULL;
    }

    if (devThreadHandle) {
        dev_post_evt(WM_USER, NULL);
        WaitForSingleObject(devThreadHandle, INFINITE);
        CloseHandle(devThreadHandle);
        devThreadHandle = NULL;
    }
    

    return 0;
}

