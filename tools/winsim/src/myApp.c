#include "types.h"
#include "task.h"
#include "com.h"

#define TIMER_MS            1000

#define WM_DIY              (WM_USER+100)


static BYTE app_rx_buf[1000];
static int app_exit_flag;
static int paras_rx_flag = 0;
static int app_thread_running = 0;
static UINT_PTR timerId;
static HANDLE appThreadHandle, appRxThreadHandle;
extern paras_t DEFAULT_PARAS;


static U8 timer_proc(void)
{
    U8 err;
    int r;

    if (!port_is_opened()) {
        return 0;
    }

    err = com_send_data(TYPE_STAT, 0, NULL, 0);

    err = com_loop_check(0);

    return err;
}

static DWORD WINAPI app_rx_thread(LPVOID lpParam)
{
    U8  err;
    int rlen;
    U16 rxbuflen;
    U8* rxbuf=NULL;

    com_get_buf(&rxbuf, &rxbuflen);
    while (1) {
        rlen = port_recv(rxbuf, rxbuflen);
        if (rlen > 0) {
           err = com_data_proc(rxbuf, rlen);
        }

        if (app_exit_flag) {
            break;
        }
    }

    return 0;
}

static void app_post_evt(int evt, void *ptr)
{
    if (app_thread_running) {
        PostThreadMessage(GetThreadId(appThreadHandle), evt, 0, 0);
    }
}
static void CALLBACK timer_callback(HWND hwnd, UINT message, UINT_PTR iTimerID, DWORD dwTime)
{
    app_post_evt(WM_USER, NULL);
}

#pragma pack (1)
typedef struct {
    U8      type;
    U8      nAck;
    U16     dlen;
    U8      data[0];
}msg_data_t;
#pragma pack ()

static DWORD WINAPI app_thread(LPVOID lpParam)
{
    U8  err;
    MSG msg;

    PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
    app_thread_running = 1;

    while (GetMessage(&msg, NULL, 0, 0)) {
        //TranslateMessage(&msg);
        DispatchMessage(&msg);

        if (app_exit_flag) {
            break;
        }

        switch (msg.message) {

            case WM_USER:
            {
                err = timer_proc();
            }
            break;

            case WM_DIY:
            {
                msg_data_t* ptr = (msg_data_t*)msg.wParam;
                if (ptr) {
                    pkt_send(ptr->type, ptr->nAck, ptr->data, ptr->dlen);
                    free(ptr);
                }
            }
            break;
        } 
    }

    return 0;
}



int task_app_start(void)
{
    app_exit_flag = 0;
    app_thread_running = 0;

    timerId = SetTimer(NULL, 1, 100, timer_callback);
    appRxThreadHandle = CreateThread(NULL, 0, app_rx_thread, NULL, 0, NULL);
    appThreadHandle = CreateThread(NULL, 0, app_thread, NULL, 0, NULL);
    
    return 0;
}

int task_app_stop(void)
{
    app_exit_flag = 1;

    KillTimer(NULL, timerId);
    if (appRxThreadHandle) {
        WaitForSingleObject(appRxThreadHandle, INFINITE);
        CloseHandle(appRxThreadHandle);
        appRxThreadHandle = NULL;
    }

    if (appThreadHandle) {
        app_post_evt(WM_USER, NULL);
        WaitForSingleObject(appThreadHandle, INFINITE);
        CloseHandle(appThreadHandle);
        appThreadHandle = NULL;
    }

    return 0;
}


int task_app_trig(U8 type, U8 nAck, void* data, U16 len)
{
    msg_data_t* ptr = (msg_data_t*)malloc(len+sizeof(msg_data_t));
    if (ptr) {
        ptr->type = type;
        ptr->nAck = nAck;
        ptr->dlen = len;
        memcpy(ptr->data, data, len);
        app_post_evt(WM_DIY, ptr);
    }
    
    return 0;
}

