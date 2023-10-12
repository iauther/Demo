
#include "protocol.h"
#include "myLog.h"
#include "myFile.h"
#include "pkt.h"
#include "rbuf.h"
#include "comm.h"
#include "log.h"

//#define USE_MYFRAME
//#define USE_CMYRAME

#define DEBUG_ON
//#define USE_SERIAL

#define BUF_LEN				80000
#define MY_TIMER			(WM_USER+0x01)

#ifdef USE_MYFRAME
#include "myFrame.h"
#elif defined USE_CMYRAME
#include "CMyFrame.h"
#else
#include "myWindow.h"
#endif


#ifdef USE_SERIAL
U8 chkID = CHK_SUM;
#else
U8 chkID = CHK_NONE;
#endif

typedef struct {
	BYTE rx[BUF_LEN];
	BYTE tx[BUF_LEN];
	
	BYTE tmp[BUF_LEN];
	BYTE pkt[BUF_LEN];

	BYTE rb[BUF_LEN];
	handle_t hrb;
}buf_data_t;

typedef struct {
	U8          sysState;

#ifdef USE_MYFRAME
	myFrame	     myWin;
#elif defined USE_CMYRAME
	CMyFrame     myWin;
#else
	myWindow	 myWin;
#endif

	buf_data_t  com;
	buf_data_t  sock;

	myFile      mFile;
	
	handle_t    hcomm;
}my_handle_t;


static UINT_PTR	tmrID;
static my_handle_t myHandle;
all_para_t allPara;

static U8 get_checksum(U8* data, U16 len)
{
	U8 sum = 0;
	for (int i = 0; i < len; i++) {
		sum += data[i];
	}
	return sum;
}
static void post_event(DWORD thdID, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	PostThreadMessage(thdID, Msg, wParam, lParam);
}




#if 1
static int io_read(void *buf, int buflen)
{
	return myHandle.myWin.port_read(buf, buflen);
}
static int io_write(U8 type, U8 nack, void* data, int datalen)
{
	return myHandle.myWin.port_write(type, nack, data, datalen);
}
static int io_data_proc(void* data, int datalen)
{
	return myHandle.myWin.data_proc(data, datalen, chkID);
}
int rbuf_clr(void)
{
	return rbuf_reset(myHandle.com.hrb);
}


static int quit = 0;
static DWORD dataRecvThread(LPVOID lpParam)
{
	int rlen, find, wlen;
	BYTE* rxBuf = myHandle.com.rx;

	memset(rxBuf, 0, BUF_LEN);
	while (1) {
		rlen = io_read((char*)rxBuf, BUF_LEN);
		if (rlen > 0) {
			wlen = rbuf_write(myHandle.com.hrb, rxBuf, rlen);
		}

		if (quit) {
			break;
		}
	}

	return 0;
}

static DWORD dataProcThread(LPVOID lpParam)
{
	int i, rlen, plen;
	int err, find;
	pkt_hdr_t* hdr;
	BYTE* rbBuf = myHandle.com.rb;
	BYTE* tmpBuf = myHandle.com.tmp;
	BYTE* pktBuf = myHandle.com.pkt;

	myHandle.com.hrb = rbuf_init(rbBuf, BUF_LEN);
	while (1) {
		rlen = rbuf_read(myHandle.com.hrb, tmpBuf, BUF_LEN, 0);
		if (rlen > PKT_HDR_LENGTH) {
			find = plen = 0;
			for (i = 0; i < rlen; i++) {
				hdr = (pkt_hdr_t*)(tmpBuf + i);
				if (hdr->magic == PKT_MAGIC) {
					int plen = hdr->dataLen + PKT_HDR_LENGTH;
					if (rlen - i >= plen) {
						err = pkt_check_hdr(hdr, plen, rlen - i, chkID);
						if (err) {
							LOGE("pkt hdr error: 0x%02x\n", err);
							//return err;
							continue;
						}

						if (plen <= BUF_LEN) {
							memcpy(pktBuf, hdr, (plen));
							rbuf_read_shift(myHandle.com.hrb, plen);
							find = 1;
							break;
						}
					}
				}
			}

			if (find) {
				io_data_proc(pktBuf, plen);
			}
			//print(_T("___\n"));
		}

	}

	return 0;
}
#endif

#define WIN_WIDTH		1200
#define WIN_HEIGHT		900
CAppModule appModule;
int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
	CMessageLoop* pLoop = appModule.GetMessageLoop();

	ATLASSERT(pLoop != NULL);


	myHandle.myWin.ShowWindow(nCmdShow);

	int nRet = pLoop->Run();

	appModule.RemoveMessageLoop();

	return nRet;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
	CMessageLoop theLoop;

	HRESULT hRes = ::OleInitialize(NULL);
	ATLASSERT(SUCCEEDED(hRes));

	//::DefWindowProc(NULL, 0, 0, 0L);

	AtlInitCommonControls(ICC_COOL_CLASSES | ICC_BAR_CLASSES | ICC_USEREX_CLASSES);

	hRes = appModule.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	
	appModule.AddMessageLoop(&theLoop);

	CRect rc = CRect(POINT{ 400, 100 }, POINT{ 1400, 900 });
	myHandle.myWin.Create(NULL, rc, _T("test"), WS_VISIBLE | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME);

	CreateThread(NULL, NULL, dataProcThread, NULL, 0, NULL);
	CreateThread(NULL, NULL, dataRecvThread, NULL, 0, NULL);

	int nRet = Run(lpstrCmdLine, nCmdShow);

	appModule.RemoveMessageLoop();

	appModule.Term();
	::OleUninitialize();

	return nRet;
}

