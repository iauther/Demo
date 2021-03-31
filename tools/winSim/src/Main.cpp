#include <windows.h>
#include "win.h"
#include "task.h"


#pragma comment  (lib, "User32.lib")
//#pragma comment  (linker, "/subsystem: "windows" /entry:"mainCRTStartup" ")

int exit_flag=0;
static DWORD mainThreadId;
int main(void)
{
    MSG msg;

    mainThreadId = GetCurrentThreadId();

    win_init(0);
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        if (exit_flag) {
            break;
        }
    }
    task_dev_stop();
    task_app_stop();

    return 0;
}



int task_quit(void)
{
    exit_flag = 1;
    PostQuitMessage(0xff);
    return 0;
}