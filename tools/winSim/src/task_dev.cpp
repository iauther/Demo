#include <windows.h>
#include "dev.h"
#include "win.h"
#include "log.h"
#include "data.h"
#include "task.h"

static BYTE dev_rx_buf[1000];
static BYTE dev_tx_buf[1000];
static int dev_exit_flag=0;
extern sett_t* pCurSett;

const setts_t DEFAULT_SETTS = {

    MODE_CONTINUS,
    {
        {
            MODE_CONTINUS,
            {0,0,0},
            90.0F,
            100.0F
         },

         {
            MODE_INTERVAL,
            {0,0,0},
            90.0F,
            100.0F
         },

         {
            MODE_FIXED_TIME,
            {0,0,0},
            90.0F,
            100.0F
         },

         {
            MODE_FIXED_VOLUME,
            {0,0,0},
            90.0F,
            100.0F
         },
    },
};





static int upgrade_proc(pkt_hdr_t* p)
{
    int r = 0;
    static U16 upg_pid = 0;
    upgrade_pkt_t* upg = (upgrade_pkt_t*)p->data;

    if (upg->pid == 0) {
        upg_pid = 0;
    }

    if (upg->pid != upg_pid) {
        return -1;
    }

    if (upg->pid == upg->pkts - 1) {
        //upgrade finished
    }
    else {
        upg_pid++;
    }

    return r;
}
static int stat_proc(pkt_hdr_t* p)
{
    int r = 0;
    static U16 upg_pid = 0;
    stat_t* st = (stat_t*)p->data;

}
static int ack_proc(pkt_hdr_t* p)
{
    int r = 0;
    static U16 upg_pid = 0;
    stat_t* st = (stat_t*)p->data;

}
static U8 get_checksum(U8* data, U16 len)
{
    U8 sum = 0;
    for (int i = 0; i < len; i++) {
        sum += data[i];
    }
    return sum;
}
static int dev_data_proc(void *data, int len)
{
    U8  checksum;
    U8* buf = (U8*)data;
    pkt_hdr_t* p = (pkt_hdr_t*)data;

    if (p->magic != PKT_MAGIC) {
        log("___ pkt magic is 0x%04x, magic is wrong!\n", p->magic);
        return -1;
    }

    if (len != p->dataLen + PKT_HDR_LENGTH + 1) {
        log("___ pkt length is wrong\n");
        return -1;
    }

    checksum = get_checksum(buf, len - 1);
    if (checksum != buf[len - 1]) {
        log("___ pkt checksum is wrong!\n", checksum);
        return -1;
    }

    switch (p->type) {
        case TYPE_STAT:
        {
            log("___ TYPE_STAT\n");
            stat_t* stat = (stat_t*)p->data;
            win_stat_update(stat);
        }
        break;

        case TYPE_ACK:
        {
            log("___ TYPE_ACK\n");
        }
        break;

        case TYPE_SETT:
        {
            log("___ TYPE_SETT\n");
            memcpy(&curSetts, p->data, sizeof(curSetts));
        }
        break;

        case TYPE_UPGRADE:
        {
            log("___ TYPE_UPGRADE\n");
        }
        break;

        case TYPE_FWINFO:
        {
            log("___ TYPE_FWINFO\n");
        }
        break;

        default:
            break;
    }

    return 0;
}
static int dev_send_stat(void)
{
    stat_t stat;
                
    stat.temp = 88.9F;
    stat.aPres = 88.9F;
    stat.dPres = 88.9F;
    stat.pump.speed = 500;
    stat.pump.current = 650.0F;
    stat.pump.temp = 44.9F;
    stat.pump.fault = 0;
    stat.pump.cmdAck = 1;
    dev_send_pkt(TYPE_STAT, 0, &stat, sizeof(stat));
    
    return 0;
}




static DWORD WINAPI dev_rx_thread(LPVOID lpParam)
{
    int r;
    int rlen;
    
    while (1) {
        rlen = dev_recv(dev_rx_buf, sizeof(dev_rx_buf));
        if (rlen > 0) {
            r = dev_data_proc(dev_rx_buf, rlen);
        }

        if (dev_exit_flag) {
            break;
        }
    }

    return 0;
}


#define WM_DEV_QUIT   (WM_USER+1)
static HANDLE devRxThreadHandle, devTxThreadHandle;
static void inline send_evt(UINT evt)
{
    PostThreadMessage(GetThreadId(devTxThreadHandle), evt, 0, 0);
}



static DWORD WINAPI dev_tx_thread(LPVOID lpParam)
{
    int r;
    MSG msg;
    
    SetTimer(NULL, 1, 500, NULL);
    while (GetMessage(&msg, NULL, 0, 0)) {

        switch (msg.message) {
            case WM_TIMER:
            {
                dev_send_stat();

            }
            break;

            case WM_DEV_QUIT:
            {
                break;
            }
            break;
        }

        if (dev_exit_flag) {
            break;
        }
    }

    return 0;
}




int task_dev_start(void)
{
    dev_exit_flag = 0;
    devRxThreadHandle = CreateThread(NULL, 0, dev_rx_thread, NULL, 0, NULL);
    devTxThreadHandle = CreateThread(NULL, 0, dev_tx_thread, NULL, 0, NULL);

    return 0;
}

int task_dev_stop(void)
{
    dev_exit_flag = 1;
    send_evt(WM_DEV_QUIT);

    if (devRxThreadHandle) {
        WaitForSingleObject(devRxThreadHandle, INFINITE);
        CloseHandle(devRxThreadHandle);
        devRxThreadHandle = NULL;
    }

    if (devTxThreadHandle) {
        WaitForSingleObject(devTxThreadHandle, INFINITE);
        CloseHandle(devTxThreadHandle);
        devTxThreadHandle = NULL;
    }

    return 0;
}

