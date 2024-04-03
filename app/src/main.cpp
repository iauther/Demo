
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


typedef struct {
	int     pr;
	int     pw;
	int     dlen;
	int     size;
	U8* buf;
	int     mode;

	handle_t lock;
}rbuf_handle_t;


typedef struct {
	BYTE rx[BUF_LEN];
	BYTE tx[BUF_LEN];
	
	BYTE tmp[BUF_LEN];
	BYTE pkt[BUF_LEN];

	BYTE rb[BUF_LEN];
	handle_t hrb;

	BYTE dbg[BUF_LEN];
}io_buf_t;

typedef struct {
	int         port;
	io_buf_t    buf;
}io_handle_t;

typedef struct {
	U8          sysState;

#ifdef USE_MYFRAME
	myFrame	     myWin;
#elif defined USE_CMYRAME
	CMyFrame     myWin;
#else
	myWindow	 myWin;
#endif

	myFile       mFile;
	io_handle_t  hio[PORT_MAX];
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

typedef struct {
	int     pr;
	int     pw;
	int     dlen;
	int     size;
	U8* buf;
	int     mode;

	handle_t lock;
}rbuf_handle2_t;
#if 1
static int io_read(int port, void *buf, int buflen)
{
	return myHandle.myWin.port_read(port, buf, buflen);
}
static int io_write(int port, U8 type, U8 nack, void* data, int datalen)
{
	return myHandle.myWin.port_write(port, type, nack, data, datalen);
}
static int io_data_proc(int port, void* data, int datalen)
{
	int chkID = (port == PORT_NET) ? CHK_NONE : CHK_SUM;
	return myHandle.myWin.data_proc(port, data, datalen, chkID);
}



static int quit_flag = 0;
static int io_data_init(io_handle_t* hio)
{
	rbuf_cfg_t rc;

	rc.mode = RBUF_FULL_FIFO;
	rc.size = BUF_LEN;
	rc.buf = hio->buf.rb;

	memset(hio->buf.rx, 0, BUF_LEN);
	hio->buf.hrb = rbuf_init(&rc);

	return 0;
}
static int io_data_free(io_handle_t* hio)
{
	rbuf_free(hio->buf.hrb);
	return 0;
}
static int io_data_recv(io_handle_t* hio)
{
	int rlen, wlen;

	rlen = io_read(hio->port, (char*)hio->buf.rx, BUF_LEN);
	if (rlen <= 0) {
		return -1;
	}

	wlen = rbuf_write(hio->buf.hrb, hio->buf.rx, rlen);

	return wlen;
}

#define DEV_LOG_SHOW
static inline int is_print(int c)
{
	if (c=='\n') {
		return 2;
	}
	else if (c == '\r' || (c >= 32 && c <= 126)) {
		return 1;
	}

	return 0;
}
static int is_all_print(BYTE* p, int len)
{
	int i=0,flag=0;

	while(i<len) {
		flag = is_print(p[i]);
		if (flag==0 || flag==2) {
			break;
		}
		i++;
	}

	return (flag==2)?(i+1):0;
}
static void io_data_print(BYTE *p, int len)
{
#ifdef DEV_LOG_SHOW
	if (!p || !len) {
		return;
	}

	char* tmp = new char[len + 1];
	if (tmp) {
		memcpy(tmp, p, len);
		tmp[len] = 0;
		LOGD("[dev]: %s", tmp);
		delete[] tmp;
	}
#endif
}
static int io_data_proc(io_handle_t* hio)
{
	int i, rlen, plen;
	int err, find=0;
	pkt_hdr_t* hdr;
	int chkID = (hio->port==PORT_NET) ? CHK_NONE : CHK_SUM;

	rlen = rbuf_read(hio->buf.hrb, hio->buf.tmp, BUF_LEN, 0);
	if (rlen >= PKT_HDR_LENGTH) {
		find = plen = 0;
		for (i = 0; i < rlen; i++) {
			hdr = (pkt_hdr_t*)(hio->buf.tmp + i);
			if (hdr->magic == PKT_MAGIC) {
				int plen = hdr->dataLen + PKT_HDR_LENGTH + pkt_chk_len(chkID);
				if (rlen - i >= plen) {
					err = pkt_check_hdr(hdr, plen, rlen - i, chkID);
					if (err) {
						LOGE("pkt hdr error: 0x%02x\n", err);
						continue;
					}

					if (plen <= BUF_LEN) {
						memcpy(hio->buf.pkt, hdr, plen);
						rbuf_read_shift(hio->buf.hrb, plen+i);
						find = 1; break;
					}
				}
			}
		}

		if (find) {
			io_data_proc(hio->port, hio->buf.pkt, plen);
		}
		else {
			int xlen = is_all_print(hio->buf.tmp, rlen);
			if (xlen>0) {
				io_data_print(hio->buf.tmp, xlen);
				rbuf_read_shift(hio->buf.hrb, xlen);
			}
		}
	}

	return find;
}
static DWORD dataRecvThread(LPVOID lpParam)
{
	int i;
	io_handle_t* hio = myHandle.hio;

	while (!quit_flag) {
		for (i = 0; i < PORT_MAX; i++) {
			io_data_recv(&hio[i]);
		}
	}

	return 0;
}
static DWORD dataProcThread(LPVOID lpParam)
{
	int i;
	io_handle_t* hio = myHandle.hio;
	
	while (!quit_flag) {
		for (i = 0; i < PORT_MAX; i++) {
			io_data_proc(&hio[i]);
		}
	}

	return 0;
}
static DWORD sendThreadId;
int my_post(int e, int id)
{
	my_msg_t* m = new my_msg_t();
	if (!m) {
		LOGE("___ new my_msg_t() error\n");
		return -1;
	}
	m->e = e;
	m->id = id;
	return PostThreadMessage(sendThreadId, MY_MSG, (WPARAM)m, NULL);
}
static DWORD dataSendThread(LPVOID lpParam)
{
	MSG msg;
	while (!quit_flag) {

		if (GetMessage(&msg, 0, 0, 0)) {
			if (!quit_flag && msg.message == MY_MSG) {
				my_msg_t* m = (my_msg_t*)msg.wParam;
				myHandle.myWin.my_proc(m);
				delete m;
			}
		}
	}

	return 0;
}
void my_quit(void)
{
	quit_flag = 1;
	my_post(MY_MSG, 0x800);
}
void my_init(void)
{
	int i;
	io_handle_t* hio = myHandle.hio;
	for (i = 0; i < PORT_MAX; i++) {
		hio[i].port = i;
		io_data_init(&hio[i]);
	}
}
void my_free(void)
{
	int i;
	io_handle_t* hio = myHandle.hio;
	for (i = 0; i < PORT_MAX; i++) {
		io_data_free(&hio[i]);
	}
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
#define THREAD_NUM  3
	HANDLE hThread[THREAD_NUM];

	my_init();

	HRESULT hRes = ::OleInitialize(NULL);
	ATLASSERT(SUCCEEDED(hRes));

	//::DefWindowProc(NULL, 0, 0, 0L);

	AtlInitCommonControls(ICC_COOL_CLASSES | ICC_BAR_CLASSES | ICC_USEREX_CLASSES);

	hRes = appModule.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));


	appModule.AddMessageLoop(&theLoop);

	CRect rc = CRect(POINT{ 400, 100 }, POINT{ 1400, 900 });
	myHandle.myWin.Create(NULL, rc, _T("test"), WS_VISIBLE | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME);

	hThread[0] = CreateThread(NULL, NULL, dataProcThread, NULL, 0, NULL);
	hThread[1] = CreateThread(NULL, NULL, dataRecvThread, NULL, 0, NULL);
	hThread[2] = CreateThread(NULL, NULL, dataSendThread, NULL, 0, &sendThreadId);

	int nRet = Run(lpstrCmdLine, nCmdShow);

	appModule.RemoveMessageLoop();
	
	my_quit();
	WaitForMultipleObjects(THREAD_NUM, hThread, TRUE, INFINITE);
	for (int i = 0; i < THREAD_NUM; i++) {
		CloseHandle(hThread[i]);
	}

	appModule.Term();
	::OleUninitialize();

	my_free();

	return nRet;
}

