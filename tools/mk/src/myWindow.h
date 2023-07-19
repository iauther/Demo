
#pragma once

#include "wtl.h"
#include "myTcp.h"
#include "myMqtt.h"

enum {
	ID_CONN =0xff00,
    ID_CALI,
	ID_LOG_CLR,
	ID_LOG_STOP,
	ID_LOG_SAVE,
	ID_DAT_SAVE,
};


#define DBG_BUF_LEN   1000
static TCHAR dbgBuf[DBG_BUF_LEN];

int quit = 0;
int printEnableFlag = 1;
static HANDLE ackEvent;
static CEdit debug;
static int char2wchar(wchar_t* wchar, int wlen, char* cchar)
{
	int xlen = MultiByteToWideChar(CP_ACP, 0, cchar, strlen(cchar), NULL, 0);
	if (xlen >= wlen - 1) {
		xlen = wlen - 1;
	}
	MultiByteToWideChar(CP_ACP, 0, cchar, strlen(cchar), wchar, xlen);
	wchar[xlen] = '\0';

	return 0;
}

static int is_tailof(TCHAR *src, const TCHAR *str)
{
	int len1 = _tcslen(src);
	int len2 = _tcslen(str);
	int cmp = _tcscmp(src+len1-len2, str);

	return (cmp == 0) ? 1 : 0;

}
void print(const char* format, ...)
{
	int n = 0;
	va_list list;
	TCHAR fmt[200];
	SCROLLINFO ScroInfo;

	if (!printEnableFlag) {
		return;
	}

	char2wchar(fmt, sizeof(fmt), (char*)format);

	va_start(list, format);
	_vstprintf_s(dbgBuf, fmt, list);
	va_end(list);

	//支持\r\n和\n换行
	if (is_tailof(dbgBuf, _T("\n"))) {
		TCHAR* x = _tcsrchr(dbgBuf, _T('\n'));
		if (x) _tcscpy(x, _T("\r\n"));
	}

	//char2wchar(tmpBuf, sizeof(tmpBuf), tmp);
	//debug.AppendText(tmpBuf, 1, 0);


	int iTextLen = debug.GetWindowTextLength();
	debug.SetSel(iTextLen, iTextLen, FALSE);//移动光标到最后
	debug.SetFocus(); //移动光标到最后
	debug.ReplaceSel(dbgBuf);  //在光标的位置加入最新的输出日志行
	debug.LineScroll(debug.GetLineCount());

	//debug.GetScrollInfo(SB_VERT, &ScroInfo);
	//debug.SetScrollPos(SB_VERT, ScroInfo.nMax);
	//debug.LineScroll(ScroInfo.nMax);
}


class myWindow: public CWindowImpl<myWindow>
	            ,public CDoubleBufferImpl<myWindow>
				,public CSplitterImpl<myWindow>
				,public CWinDataExchange<myWindow>
				//,public CSplitterImpl<myWindow>
{

	public:
		myWindow();
		~myWindow();
		CHorSplitterWindow rootSplit;
		CPaneContainer dbgPane;

		int proto = 1;		//0: tcp   1: mqtt
		myTcp mTcp;
		myMqtt mMqtt;
		

		BEGIN_MSG_MAP(myWindow)
			MESSAGE_HANDLER(WM_CREATE, OnCreate)
			MESSAGE_HANDLER(WM_CLOSE, OnClose)
			MESSAGE_HANDLER(WM_TIMER, OnTimer)
			//MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
			//MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)

			COMMAND_HANDLER(ID_CONN,     BN_CLICKED,  OnBtnClicked)
			COMMAND_HANDLER(ID_CALI,     BN_CLICKED,  OnBtnClicked)
			COMMAND_HANDLER(ID_DAT_SAVE, BN_CLICKED,  OnBtnClicked)
			COMMAND_HANDLER(ID_LOG_CLR,  BN_CLICKED,  OnBtnClicked)
			COMMAND_HANDLER(ID_LOG_STOP, BN_CLICKED,  OnBtnClicked)
			COMMAND_HANDLER(ID_LOG_SAVE, BN_CLICKED,  OnBtnClicked)

			//MESSAGE_HANDLER(WM_PAINT, OnPaint)
			//MSG_WM_CLOSE(OnClose)
		END_MSG_MAP()


		void CreateSplitWindow(void)
		{
			CRect rc;
			GetClientRect(&rc);

			//create root hor splitter
			rootSplit.Create(m_hWnd, rc, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
			rootSplit.m_bFullDrag = true;
			rootSplit.m_cxyMin = 80;
			//rootSplit.m_cxySplitBar = 4;
			rootSplit.SetSplitterPos(200);


			dbgPane.Create(rootSplit.m_hWnd);
			dbgPane.SetTitle(_T("debug"));
			dbgPane.SetPaneContainerExtendedStyle(PANECNT_NOCLOSEBUTTON);

			rootSplit.SetSplitterPane(SPLIT_PANE_BOTTOM, dbgPane, true);

			debug.Create(dbgPane.m_hWnd, &rc, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS| 
				                                                                      WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_WANTRETURN);
			debug.SetReadOnly(1);
			debug.LimitText(0x99999999);

			mTcp.set_print(print);
			mMqtt.set_print(print);
		}
		

		int startClient(void)
		{
			if (proto==0) {
				extern DWORD WINAPI sockRecvThread(LPVOID lpParam);
				char* ip =(char*) "192.168.2.88";
				mTcp.conn(ip, 8888, sockRecvThread);
			}
			else {
				char* host=(char*)"iot-06z00ers5w6yi1p.mqtt.iothub.aliyuncs.com";
				int port = 1883;
				char* user = (char*)"fufyqdawjqEyJDSVaRNI";
				char* passwd = (char*)"958c71af2c8b4b6a0dd8495dbb79f11a";
				mMqtt.conn(host, port, user, passwd);

				SetTimer(88, 1000, NULL);
			}

			return 0;
		}

		int stopClient(void)
		{
			if (proto == 0) {
				mTcp.disconn();
			}
			else {
				mMqtt.disconn();

				KillTimer(88);
			}

			return 0;
		}


		CButton btConn, btCali,btSav,logClr;

		LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
		{
			CRect rc;
			CFont font;
			
#if 1
			rc.left = 100;
			rc.right = 200;
			rc.top = 50;
			rc.bottom = 100;
			btConn.Create(m_hWnd, rc, _T("connect"), WS_VISIBLE | WS_CHILD, NULL, ID_CONN, NULL);

			rc.left = 300;
			rc.right = 400;
			btCali.Create(m_hWnd, rc, _T("cali start"), WS_VISIBLE | WS_CHILD, NULL, ID_CALI, NULL);

			rc.left = 500;
			rc.right = 600;
			btSav.Create(m_hWnd, rc, _T("save start"), WS_VISIBLE | WS_CHILD, NULL, ID_DAT_SAVE, NULL);

			rc.left = 100;
			rc.right = 200;
			rc.top = 120;
			rc.bottom = 170;
			logClr.Create(m_hWnd, rc, _T("log clear"), WS_VISIBLE | WS_CHILD, NULL, ID_LOG_CLR, NULL);
#endif
			CreateSplitWindow();
			return 0;
		}
		

		LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
		{
			int r;
			char tmp[100];
			char* topic=(char*)"/h471xrlbuE9/gasLeap/user/XXX";
			static int pub_cnt = 0;
			
			if (mMqtt.is_connected()) {
				memset(tmp, 0, sizeof(tmp));
				sprintf(tmp, "hello %d", pub_cnt++);

				print("___publish: %s\n", tmp);
				r = mMqtt.publish(topic, tmp);
				print("___publish result %d\n", r);
			}
			else {
				print("___mqtt not connected!\n");
			}

			return 0;
		}

		
		LRESULT OnBtnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
		{
			static int connFlag = 0;
			static int caliFlag = 0;

			CButton btn = hWndCtl;

			switch (wID) {
			case ID_CONN:
			{
				if (connFlag == 0) {
					startClient();
					connFlag = 1;
					btConn.SetWindowText(L"disconnect");
				}
				else {
					stopClient();
					connFlag = 0;
					btConn.SetWindowText(L"connect");
				}
			}
			break;

			case ID_CALI:
			{
				if (caliFlag==0) {

					btCali.SetWindowText(L"cali stop");
					caliFlag = 1;
				}
				else {

					btCali.SetWindowText(L"cali start");
					caliFlag = 0;
				}
			}
			break;

			case ID_LOG_CLR:
			{
				debug.SetWindowText(_T(""));
			}
			break;

			case ID_LOG_STOP:
			{
				if (printEnableFlag == 0) {


					printEnableFlag = 1;
				}
				else {


					printEnableFlag = 0;
				}
			}
			break;

			case ID_LOG_SAVE:
			{
				//extern void writeFile(U8 ch, void* data, int len);
			}
			break;

			case ID_DAT_SAVE:
			{
				extern int saveFlag;
				extern void stopRecAll(void);
				if (saveFlag == 0) {
					saveFlag = 1;
					btSav.SetWindowText(_T("save stop"));
				}
				else {
					saveFlag = 0;
					stopRecAll();
					btSav.SetWindowText(_T("save start"));
				}
			}
			break;

			return 0;
			}
		}

		int wait_ack_evt(void)
		{
			DWORD r;

			r = WaitForSingleObject(ackEvent, 500);
			if (r!=0) {
				return -1;
			}

			return 0;
		}
		static void set_ack_evt(void)
		{
			SetEvent(ackEvent);
		}



		LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
		{

			DestroyWindow();
			PostQuitMessage(0);

			return 0;
		}

		LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
		{

			return 0;
		}

		LRESULT DoPaint(CDCHandle dc)
		{

			return 0;
		}

		LRESULT OnLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)	{return 0;}
		LRESULT OnLButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)	{return 0;}

		


		void OnSize(UINT nType, CSize size)
		{
			CRect clientRect, scrRect;

			GetClientRect(&clientRect);

			scrRect.top = clientRect.top;
			scrRect.left = clientRect.left;
			scrRect.right = scrRect.left + 20;
			scrRect.bottom = clientRect.bottom;
			//scroller.MoveWindow(scrRect);
		}


	private:
		CButton		cmd[40];
		CComboBox	aesKeyLen;
		CComboBox	aesMode;
		CStatic     label;
		CString     str;
		CListBox    lbox;
		CScrollBar  sbar;

};



myWindow::myWindow()
{
	ackEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
}


myWindow::~myWindow()
{
	stopClient();
	CloseHandle(ackEvent);
}































