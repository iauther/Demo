#include <windows.h>
#include "framework.h"
#include "winsim.h"
#include "win.h"
#include "port.h"
#include "task.h"
#include "icfg.h"


int exit_flag = 0;
static DWORD mThreadId;

static void send_evt(void)
{
    PostThreadMessage(mThreadId, WM_USER, 0, 0);
}

static void load_paras(void)
{

}


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    MSG msg;

    mThreadId = GetCurrentThreadId();
    port_init();
    win_init(MCU_SIM);


    // 主消息循环:
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        if (exit_flag) {
            break;
        }
    }

    return (int) msg.wParam;
}

int task_quit(void)
{
    exit_flag = 1;
    send_evt();

    return 0;
}


