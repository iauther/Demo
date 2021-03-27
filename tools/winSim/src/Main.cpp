#include <windows.h>
#include "win.h"
#include "task.h"


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

    win_init();
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        if (exit_flag) {
            task_dev_stop();
            task_app_stop();
            break;
        }
    }

    return 0;
}

