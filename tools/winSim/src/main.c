#include <windows.h>
#include "win.h"
#include "port.h"
#include "task.h"
#include "icfg.h"

#pragma comment  (lib, "User32.lib")
int exit_flag=0;
static DWORD mainThreadId;
#define WM_MYTIMER     WM_USER+1 
#define WM_MYLOOP      WM_USER+2 
static void CALLBACK timer_callback(HWND hwnd, UINT message, UINT_PTR iTimerID, DWORD dwTime)
{
    //PostThreadMessage(mainThreadId, WM_MYLOOP, NULL, NULL);
}

int main(void)
{
    MSG msg;

    port_init();
    win_init(MCU_SIM);

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        if (exit_flag) {
            break;
        }
    }

    task_dev_stop();
    task_app_stop();
    port_free();

    return 0;
}



int task_quit(void)
{
    exit_flag = 1;
    PostQuitMessage(0xff);
    return 0;
}