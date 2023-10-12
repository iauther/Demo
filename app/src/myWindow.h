
#pragma once

#include "wtl.h"
#include "log.h"
#include "pkt.h"
#include "comm.h"
#include "mySerial.h"
#include "myMqtt.h"
#include "json.h"

enum {

	ID_MENU_HOME=0x100,
	ID_MENU_SETUP,
	ID_MENU_UPGRADE,

	ID_PORT,
	ID_PORT_LIST,

	ID_OPEN,
    ID_CALI_1,
	ID_CALI_2,
	ID_START,

	ID_CONFIG_R,
	ID_CONFIG_W,

	ID_UPGRADE,
	ID_REBOOT,

	ID_LOG_SET,
	ID_DATA_TO,
	ID_DEFAULT,

	ID_DBG_EN,
	ID_DBG_CLR,
	ID_DBG_SAV,

	ID_TAB,
	ID_PAGE_CMD,
	ID_PAGE_CALI,
	ID_PAGE_LOG,
	ID_PAGE_SETT,
	
	
};



#define PROD_KEY		"izenceucjUD"
#define PROD_SECRET		"xWYqWMNG3XQe7aEZ"
#define DEV_NAME		"PC_1"
#define DEV_SECRET		"13fb3f8472374971d3c27bced47644db"

#define HOST_URL        "iot-06z00cq4vq4hkzx.mqtt.iothub.aliyuncs.com"
#define HOST_PORT       1883

static plat_para_t platPara = {
	HOST_URL,
	HOST_PORT,

	PROD_KEY,
	PROD_SECRET,
	DEV_NAME,
	DEV_SECRET,
};




static const char* sub_topic[TOPIC_SUB_MAX] = {
	"user/get",
	"thing/model/down_raw",
	"thing/model/up_raw_reply",
};
static const char* pub_topic[TOPIC_PUB_MAX] = {
	"user/set",
	"thing/model/up_raw",
	"thing/model/down_raw_reply"
};



static HANDLE ackEvent;
extern all_para_t allPara;

class myPane : public CPaneContainer,
	           public CMessageFilter,
	           public CIdleHandler
{
	virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
		return FALSE;
	}

	virtual BOOL OnIdle()
	{
		return FALSE;
	}

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		extern CAppModule appModule;
		CMessageLoop* pLoop = appModule.GetMessageLoop();
		pLoop->AddMessageFilter(this);
		pLoop->AddIdleHandler(this);
	}
};


class myWindow : public CWindowImpl<myWindow, CWindow>
{
public:
	CFont      gFont;
	CTabCtrl   tab;
	CEdit      info;
	CPagerCtrl pageCmd,pageCali,pageLog,pageSett;
	CComboBox  port,portList;

	void* hconn;
	CSplitterWindow    vSplit;		        //主窗口，垂直分割
	CHorSplitterWindow hSplit1,hSplit2;		//右窗口，水平分割
	CPaneContainer     wavPane, infPane, cmdPane, dbgPane;
	CButton btOpen, btCali, btStart;
	CButton btEna, btSav, btClr,btTo;
	CButton btCfgr, btCfgw, btUpg,btBoot;
	CButton btCali1, btCali2, btDflt;

	handle_t hand[PORT_MAX] = {NULL};
	handle_t conn = NULL;
	int  portID = PORT_NET;
	mySerial mSerial;

	int dev_opened=0;
	int para_recved=0;
	int dev_started=0;
	int cali_cnt = 0;

	int log_started=0;
	int log_enabled=0;

	int proto = 1;		//0: tcp   1: mqtt


	BEGIN_MSG_MAP(myWindow)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_CLOSE, OnClose)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)

		MESSAGE_HANDLER(WM_SIZING, OnSize)
		//MESSAGE_HANDLER(WM_PAINT, OnPaint)
		//MSG_WM_CLOSE(OnClose)

		MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
		MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)

		COMMAND_RANGE_CODE_HANDLER(ID_OPEN, ID_DBG_SAV, BN_CLICKED, OnBtnClicked)

		//NOTIFY_HANDLER(IDC_TABCTRLDOWN, TCN_SELCHANGE, OnSelChangeTab)
		
		NOTIFY_RANGE_CODE_HANDLER(ID_PORT, ID_PORT_LIST, CBN_SELENDOK, OnPortSelect)

		NOTIFY_HANDLER(ID_TAB, TCN_SELCHANGE, OnTabChange)

		//REFLECT_NOTIFICATIONS()
		//FORWARD_NOTIFICATIONS()

	END_MSG_MAP()



public:

	void port_set_lines(int lines)
	{
		int Height;
		CRect cbSize;

		port.GetClientRect(cbSize);
		Height = port.GetItemHeight(-1);
		Height += port.GetItemHeight(0) * lines;
		port.SetWindowPos(NULL,
			0, 0,
			cbSize.right, Height,
			SWP_NOMOVE | SWP_NOZORDER
		);
	}

	void port_set_wh(int width, int height)
	{
		CRect rc;

		port.GetClientRect(&rc);

		port.SetDroppedWidth(width);
		port.SetWindowPos(NULL,
			0, 0,
			width, height,
			SWP_NOMOVE | SWP_NOZORDER
		);
	}

	void port_add(void)
	{
		const char* portName[PORT_MAX] = {
			"usb",
			"net",
			"uart",
			"mqtt"
		};
		
		port.SetFont(gFont);
		for (int i = 0; i < PORT_MAX; i++) {
			port.AddString(portName[i]);
		}

		port_set_wh(80, 100);
		port.SetCurSel(portID);
		port.ShowDropDown(FALSE);
	}

	void port_list_refresh(void)
	{
		int sel = port.GetCurSel();
		char *str = (char*)port.GetItemData(sel);

		portList.SetEditSel(0, -1);
		portList.Clear();

		switch (sel) {
			case PORT_UART:
			{
				int n;
				sp_info_t* info;

				n = mSerial.list(&info);
				if (n > 0) {
					for (int i = 0; i < n; i++) {
						portList.AddString(info[i].portName);
					}
				}
				break;

			case PORT_MQTT:
			{

			}
			break;

			case PORT_NET:
			{

			}
			break;
			}
		}
	}

	void inf_pane_init(void)
	{
		CRect rc;
	
		infPane.GetClientRect(&rc);
		info.Create(infPane, rc, NULL, WS_CHILD | ES_LEFT | ES_NOHIDESEL | WS_TABSTOP | ES_MULTILINE | ES_WANTRETURN | WS_VSCROLL | ES_AUTOVSCROLL);
		infPane.SetClient(info);
		//::SetWindowPos(h, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

		info.SetReadOnly(1);
		info.LimitText(5000);
		info.ShowScrollBar(SB_VERT);
		info.SetFont(gFont);
	}

	int tailof(char* src, char* str)
	{
		int len1 = _tcslen(src);
		int len2 = _tcslen(str);
		int cmp = _tcscmp(src + len1 - len2, str);

		return (cmp == 0) ? 1 : 0;
	}

	void info_print(char* txt)
	{
		int n=0;
		TCHAR fmt[200];
		SCROLLINFO ScroInfo;

		//支持\r\n和\n换行
		if (tailof(txt, (char*)"\n")) {
			TCHAR* x = _tcsrchr(txt, '\n');
			if (x) _tcscpy(x, "\r\n");
		}


		info.SetRedraw(FALSE);
		int txtLen = info.GetWindowTextLength();
		info.SetSel(txtLen, txtLen);				 //移动光标到最后
		info.SetFocus();                             //移动光标到最后
		info.ReplaceSel(txt);						 //在光标的位置加入最新的输出日志行
		info.LineScroll(info.GetLineCount());
		info.SetRedraw(TRUE);
	}


	void load_para(void)
	{
		int r;
		FILE* fp=NULL;
		struct stat st;
		char* pbuf = NULL;
		const char* path = "app.ini";

		r = stat(path, &st);
		if (r == 0) {		//文件存在
			fp = fopen(path, "r");
			if (!fp) {	
				LOGE("___ %s open failed\n", path);
				return;
			}

			pbuf = new char[st.st_size];
			if (!pbuf) {
				LOGE("___ pbuf new failed\n");
				return;
			}

			//解析文本，获取参数


			fread(pbuf, 1, st.st_size, fp);

			delete[] pbuf;
			fclose(fp);
		}
		else {				//文件不存在则创建
			fp = fopen(path, "wt");
			if (!fp) {
				LOGE("___ %s create failed\n");
				return;
			}

			//fprintf(fp, "", );

			fclose(fp);
		}
	}


	void info_clear(void)
	{
		info.SetWindowText("");
	}

	void info_update(void)
	{
		int i;
		char tmp[100];
		
		info.SetWindowText("");

		sys_para_t* sys = &allPara.sys;
		sprintf(tmp, "fw.version: %s\n", sys->para.fwInfo.version);				    info_print(tmp);
		sprintf(tmp, "fw.bldtime: %s\n", sys->para.fwInfo.bldtime);				    info_print(tmp);
		sprintf(tmp, "dev.devID: 0x%08x\n", sys->para.devInfo.devID);			    info_print(tmp);
		sprintf(tmp, "dev.datato: %s\n", sys->para.sett.datato?"app":"ali");	    info_print(tmp);
																				    
		//sprintf(tmp, "fw.length: %d\n", sys->para.fwInfo.length);				    info_print(tmp);
		sprintf(tmp, "sys.mode: %d\n", sys->stat.mode);							    info_print(tmp);
		sprintf(tmp, "sys.state: %d\n", sys->stat.state);						    info_print(tmp);
																				    
		usr_para_t* usr = &allPara.usr;											    
		sprintf(tmp, "card.apn: %s\n", usr->card.apn);							    info_print(tmp);
		sprintf(tmp, "card.type: %d\n", usr->card.type);						    info_print(tmp);
		sprintf(tmp, "card.auth: %d\n", usr->card.auth);						    info_print(tmp);
		if (usr->net.mode == 0) {												    
			sprintf(tmp, "net.ip: %s\n", usr->net.para.tcp.ip);					    info_print(tmp);
			sprintf(tmp, "net.port: %d\n", usr->net.para.tcp.port);				    info_print(tmp);
		}
		else {
			sprintf(tmp, "net.host: %s\n", usr->net.para.plat.host);				info_print(tmp);
			sprintf(tmp, "net.port: %d\n", usr->net.para.plat.port);				info_print(tmp);
			sprintf(tmp, "net.devKey: %s\n", usr->net.para.plat.devKey);			info_print(tmp);
			sprintf(tmp, "net.devSecret: %s\n", usr->net.para.plat.devSecret);		info_print(tmp);
			sprintf(tmp, "net.prdKey: %s\n", usr->net.para.plat.prdKey);			info_print(tmp);
			sprintf(tmp, "net.devSecret: %s\n", usr->net.para.plat.prdSecret);		info_print(tmp);
		}
		sprintf(tmp, "smp.pwr_mode: %d\n", usr->smp.pwr_mode);						info_print(tmp);
		sprintf(tmp, "smp.pwr_period: %ds\n", usr->smp.pwr_period);					info_print(tmp);
		
		for (i = 0; i < CH_MAX; i++) {
			sprintf(tmp, "ch[%d].upway: %d\n",		  i, usr->ch[i].upway);         info_print(tmp);
			sprintf(tmp, "ch[%d].upwav: %d\n",		  i, usr->ch[i].upwav);         info_print(tmp);
			sprintf(tmp, "ch[%d].smpFreq: %d\n",	  i, usr->ch[i].smpFreq);       info_print(tmp);
			sprintf(tmp, "ch[%d].smpPoints: %d\n",	  i, usr->ch[i].smpPoints);     info_print(tmp);
			sprintf(tmp, "ch[%d].smpInterval: %d\n",  i, usr->ch[i].smpInterval);   info_print(tmp);
			sprintf(tmp, "ch[%d].smpTimes: %d\n",     i, usr->ch[i].smpTimes);      info_print(tmp);
			sprintf(tmp, "ch[%d].ampThreshold: %f\n", i, usr->ch[i].ampThreshold);  info_print(tmp);
			sprintf(tmp, "ch[%d].messDuration: %d\n", i, usr->ch[i].messDuration);  info_print(tmp);
			sprintf(tmp, "ch[%d].trigDelay: %d\n",    i, usr->ch[i].trigDelay);     info_print(tmp);


			sprintf(tmp, "ch[%d].n_ev: %d\n",		i, usr->ch[i].n_ev);            info_print(tmp);
			sprintf(tmp, "ch[%d].evCalcCnt: %d\n",	i, usr->ch[i].evCalcCnt);       info_print(tmp);
			sprintf(tmp, "ch[%d].coef.a: %f\n",		i, usr->ch[i].coef.a);          info_print(tmp);
			sprintf(tmp, "ch[%d].coef.b: %f\n",		i, usr->ch[i].coef.b);          info_print(tmp);
		}

		sprintf(tmp, "dac.enable: %d\n",	usr->dac.enable);		                info_print(tmp);
		sprintf(tmp, "dac.fdiv: %d\n",		usr->dac.fdiv);		                    info_print(tmp);
		sprintf(tmp, "mbus.addr: 0x%02x\n", usr->mbus.addr);	                    info_print(tmp);
																                    
		sprintf(tmp, "dbg.enable: %d\n",	usr->dbg.enable);	                    info_print(tmp);
		sprintf(tmp, "dbg.level: %d\n",		usr->dbg.level);	                    info_print(tmp);
		sprintf(tmp, "dbg.to: %d\n",		usr->dbg.to);		                    info_print(tmp);
	}

	void cmd_pane_init(void)
	{
		CRect rc;
		HWND hwnd;

#ifdef USE_TAB
		cmdPane.GetClientRect(&rc);
		//tab.Create(cmdPane, rc, NULL, WS_CHILD | WS_VISIBLE, NULL, ID_TAB, NULL);
		//tab.ModifyStyleEx(0, WS_EX_CONTROLPARENT);

		tab.SetFont(gFont);
		tab.InsertItem(0, "cmd");
		tab.InsertItem(1, "cali");
		tab.InsertItem(2, "log");
		tab.InsertItem(3, "sett");

		tab.GetClientRect(&rc);
		pageCmd.Create(tab,  rc, NULL, WS_CHILD | WS_VISIBLE, NULL, ID_PAGE_CMD,  NULL);
		pageCali.Create(tab, rc, NULL, WS_CHILD | WS_VISIBLE, NULL, ID_PAGE_CALI, NULL);
		pageLog.Create(tab,  rc, NULL, WS_CHILD | WS_VISIBLE, NULL, ID_PAGE_LOG,  NULL);
		pageSett.Create(tab, rc, NULL, WS_CHILD | WS_VISIBLE, NULL, ID_PAGE_SETT, NULL);

		rc.top += 21;
		rc.bottom -= 1;
		rc.left += 1;
		rc.right -= 1;

		tab.SetCurSel(0);
		//tab.HighlightItem(0);

		pageCmd.MoveWindow(rc);
		pageCali.MoveWindow(rc);
		pageLog.MoveWindow(rc);
		pageSett.MoveWindow(rc);
		
		pageCmd.ShowWindow(SW_SHOW);
		pageCali.ShowWindow(SW_HIDE);
		pageLog.ShowWindow(SW_HIDE);
		pageSett.ShowWindow(SW_HIDE);
#endif

		//cmdPane.SetClient(tab);
		//cmdPane.SetClient(pageCmd);
		hwnd = cmdPane;
		//hwnd = pageCmd;
	
#define BTN_WIDTH   80
#define BTN_HEIGHT  30

#define BTN_WGAP    10
#define BTN_HGAP    20

		//command buttons
		rc.left = 20;
		rc.right = rc.left + BTN_WIDTH;
		rc.top = 60;
		rc.bottom = rc.top + BTN_HEIGHT;
		btOpen.Create(hwnd, rc, "open", WS_CHILD | WS_VISIBLE, NULL, ID_OPEN, NULL);
		btOpen.SetFont(gFont);

		rc.left = rc.right + BTN_WGAP;
		rc.right = rc.left + BTN_WIDTH;
		btStart.Create(hwnd, rc, "start", WS_CHILD | WS_VISIBLE, NULL, ID_START, NULL);
		btStart.SetFont(gFont);

		rc.left = rc.right + BTN_WGAP;
		rc.right = rc.left + BTN_WIDTH;
		btCfgr.Create(hwnd, rc, "cfg-r", WS_CHILD | WS_VISIBLE, NULL, ID_CONFIG_R, NULL);
		btCfgr.SetFont(gFont);

		rc.left = rc.right + BTN_WGAP;
		rc.right = rc.left + BTN_WIDTH;
		btCfgw.Create(hwnd, rc, "cfg-w", WS_CHILD | WS_VISIBLE, NULL, ID_CONFIG_W, NULL);
		btCfgw.SetFont(gFont);

		rc.left = rc.right + BTN_WGAP;
		rc.right = rc.left + BTN_WIDTH;
		btDflt.Create(hwnd, rc, "default", WS_CHILD | WS_VISIBLE, NULL, ID_DEFAULT, NULL);
		btDflt.SetFont(gFont);

		rc.left = 20;
		rc.top = rc.bottom + BTN_HGAP;
		rc.right = rc.left + BTN_WIDTH;
		rc.bottom = rc.top + BTN_HEIGHT;
		btCali1.Create(hwnd, rc, "cali-1", WS_CHILD | WS_VISIBLE, NULL, ID_CALI_1, NULL);
		btCali1.SetFont(gFont);

		rc.left = rc.right + BTN_WGAP;
		rc.right = rc.left + BTN_WIDTH;
		btCali2.Create(hwnd, rc, "cali-2", WS_CHILD | WS_VISIBLE, NULL, ID_CALI_2, NULL);
		btCali2.SetFont(gFont);

		rc.left = rc.right + BTN_WGAP;
		rc.right = rc.left + BTN_WIDTH;
		btUpg.Create(hwnd, rc, "upgrade", WS_CHILD | WS_VISIBLE, NULL, ID_UPGRADE, NULL);
		btUpg.SetFont(gFont);

		rc.left = rc.right + BTN_WGAP;
		rc.right = rc.left + BTN_WIDTH;
		btBoot.Create(hwnd, rc, "reboot", WS_CHILD | WS_VISIBLE, NULL, ID_REBOOT, NULL);
		btBoot.SetFont(gFont);

		//dbg control button
		rc.left = 20;
		rc.top = rc.bottom + BTN_HGAP;
		rc.right = rc.left + BTN_WIDTH;
		rc.bottom = rc.top + BTN_HEIGHT;
		btEna.Create(hwnd, rc, "dbg off", WS_CHILD | WS_VISIBLE, NULL, ID_DBG_EN, NULL);
		btEna.SetFont(gFont);

		rc.left = rc.right + BTN_WGAP;
		rc.right = rc.left + BTN_WIDTH;
		btClr.Create(hwnd, rc, "dbg clr", WS_CHILD | WS_VISIBLE, NULL, ID_DBG_CLR, NULL);
		btClr.SetFont(gFont);

		rc.left = rc.right + BTN_WGAP;
		rc.right = rc.left + BTN_WIDTH;
		btSav.Create(hwnd, rc, "dbg save", WS_CHILD | WS_VISIBLE, NULL, ID_DBG_SAV, NULL);
		btSav.SetFont(gFont);

		rc.left = rc.right + BTN_WGAP;
		rc.right = rc.left + BTN_WIDTH;
		btTo.Create(hwnd, rc, "to ali", WS_CHILD | WS_VISIBLE, NULL, ID_DATA_TO, NULL);
		btTo.SetFont(gFont);

#ifdef USE_TAB
		hwnd = pageCali;
		//在cali页面添加校准按钮
		rc.left = 20;
		rc.right = rc.left + BTN_WIDTH;
		rc.top = 20;
		rc.right = rc.left + BTN_WIDTH;
		rc.bottom = rc.top + BTN_HEIGHT;
		btCali1.Create(hwnd, rc, "cali 1", WS_CHILD | WS_VISIBLE, NULL, ID_CALI_1, NULL);
		btCali1.SetFont(gFont);

		rc.left = 20;
		rc.top = rc.bottom + BTN_HGAP;
		rc.right = rc.left + BTN_WIDTH;
		rc.bottom = rc.top + BTN_HEIGHT;
		btCali2.Create(hwnd, rc, "cali 2", WS_CHILD | WS_VISIBLE, NULL, ID_CALI_2, NULL);
		btCali2.SetFont(gFont);

#define PORT_WIDTH   80
#define PORT_HEIGHT  30

		hwnd = pageSett;
		rc.left = 20;
		rc.right = rc.left + PORT_WIDTH;
		rc.top = 40;
		rc.bottom = rc.top + PORT_HEIGHT;
		port.Create(hwnd, rc, NULL, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_HASSTRINGS, NULL, ID_PORT, NULL);
		port_add();

		rc.left = rc.right + 20;
		rc.right = rc.left + PORT_WIDTH;
		portList.Create(hwnd, rc, NULL, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_HASSTRINGS, NULL, ID_PORT_LIST, NULL);
		port_list_refresh();
#endif
	}

	void dbg_pane_init(void)
	{
		CRect rc;

		dbgPane.GetClientRect(&rc);
		log_init(dbgPane, rc, gFont);
		dbgPane.SetClient(log_get_hwnd());
	}

	void gbl_init(void)
	{
		ModifyStyle(WS_THICKFRAME, 0);
		ModifyStyle(WS_MAXIMIZEBOX, 0);

		gFont.CreateFont(16, 8, 0, 0, FW_LIGHT, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
			CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, TEXT("Arial"));

		//gFont.CreatePointFont(80, TEXT("Arial"));

		//CClientDC dc(m_hWnd);
		//dc.SetTextColor(RGB(0, 0, 0));

		json_init();
		port_init();
	}

	void add_button(HWND hwnd)
	{
		CRect rc;
		CButton bt;

		GetClientRect(&rc);

		rc.left = 40;
		rc.top = 40;
		rc.bottom = 80;
		rc.right = 100;
		//https://learn.microsoft.com/zh-cn/windows/win32/controls/button-styles?redirectedfrom=MSDN
		//BS_AUTORADIOBUTTON 单选框
		//BS_CHECKBOX 复选框，右侧带有标题的选择框
		//BS_RADIOBUTTON 单选按钮，右边显示文本
		//BS_GROUPBOX 指定一个组框
		//bt.Create(hwnd, rc, "test", WS_CHILD | WS_VISIBLE, NULL, ID_TEST, NULL);
	}



	//https://www.orcode.com/article/WTL_20113298.html
	void split_init(void)
	{
		CRect rc;

		GetClientRect(&rc);

		//创建垂直分割窗口
		vSplit.Create(m_hWnd, rc, NULL, WS_CHILD | WS_VISIBLE);
		//vSplit.SetSplitterPos(rc.right * 2 / 3);		//wav占水平窗口2/3
		vSplit.SetSplitterPos(rc.right / 2);

		wavPane.Create(vSplit);
		wavPane.SetTitle(_T("wave"));
		wavPane.SetPaneContainerExtendedStyle(PANECNT_NOCLOSEBUTTON);

		vSplit.GetClientRect(&rc);
		hSplit1.Create(vSplit, rc, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
		hSplit1.SetSplitterPos(rc.bottom / 3);			//info占垂直窗口1/3
		
		
		infPane.Create(hSplit1);
		infPane.SetTitle(_T("info"));
		infPane.SetPaneContainerExtendedStyle(PANECNT_NOCLOSEBUTTON);

		/////////////////////////////////////////////////////////////////////
		hSplit1.GetClientRect(&rc);
		hSplit2.Create(hSplit1, rc, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
		hSplit2.SetSplitterPos(rc.bottom / 2);			//ctl和dbg各占剩下垂直窗口1/2

		cmdPane.Create(hSplit2);
		cmdPane.SetTitle(_T("command"));
		cmdPane.SetPaneContainerExtendedStyle(PANECNT_NOCLOSEBUTTON);

		dbgPane.Create(hSplit2);
		dbgPane.SetTitle(_T("debug"));
		dbgPane.SetPaneContainerExtendedStyle(PANECNT_NOCLOSEBUTTON);
		
		vSplit.SetSplitterPanes(wavPane, hSplit1);

		hSplit1.SetSplitterPanes(infPane, hSplit2);
		hSplit2.SetSplitterPanes(cmdPane, dbgPane);

		vSplit.SetSinglePaneMode(SPLIT_PANE_RIGHT);					//暂时不显示wav，待wav部分完成后再放开
		vSplit.SetSplitterExtendedStyle(SPLIT_NONINTERACTIVE);
		hSplit1.SetSplitterExtendedStyle(SPLIT_NONINTERACTIVE);
		hSplit2.SetSplitterExtendedStyle(SPLIT_NONINTERACTIVE);

		//add_button(infPane);
	}

	void pane_init(void)
	{
		inf_pane_init();
		cmd_pane_init();
		dbg_pane_init();
	}

	static int data_recv_callback(handle_t h, void* addr, U32 evt, void* data, int len)
	{
		LOGD("___ mqtt data recved, %d\n", len);
		return 0;
	}

	int set_port(int port)
	{
		if (port >= PORT_MAX) {
			return -1;
		}

		portID = port;
		return 0;
	}

	net_para_t netPara;
	int  prevPortID = portID;

	int get_string(char *src, int src_len, const char *head, const char s_tok, int s_tok_index, const char e_tok, int e_tok_index, char *str, int str_len)
	{
		int ok=0,sl;
		int s_idx = -1, e_idx = -1;
		char *p, *e, *ps=NULL, *pe=NULL;
		
		e = src + src_len;
		p = strstr(src, head);
		if (!p) {
			LOGE("___ %s not found!\n", head);
			return -1;
		}

		p += strlen(head);
		while (p<e) {
			if (*p == s_tok) {
				s_idx++;
			}
			if (*p == e_tok) {
				e_idx++;
			}

			if (s_idx == s_tok_index && !ps) {
				ps = p + 1;
			}
			if (e_idx == e_tok_index && !pe) {
				pe = p - 1;
			}

			if (ps && pe) {
				ok = 1;
				sl = pe - ps + 1;
				break;
			}

			p++;
		}

		if (!ok) {
			LOGE("___ not find the demand string, s_idx: %d, e_idx: %d\n", s_idx, e_idx);
			return -1;
		}

		if (str_len < sl) {
			LOGE("___ str buf len %d is too small, need %d\n", str_len, sl);
			return -1;
		}

		memcpy(str, ps, sl);
		str[sl] = 0;
		
		return 0;
	}


	int load_net_para(net_para_t *para)
	{
		int   r=0,flen;
		char* fbuf,*p;
		const char *path = "app.ini";

		r = load_file(path, &fbuf, &flen);
		if (r) {
			LOGE("___ load_file %s failed\n", path);
			return -1;
		}

		para->mode = 1;
		r = get_string(fbuf, flen, "APP:", '"', 0, '"', 1, para->para.plat.devKey, sizeof(para->para.plat.devKey));
		r = get_string(fbuf, flen, "APP:", '"', 2, '"', 3, para->para.plat.devSecret, sizeof(para->para.plat.devSecret));

		delete[] fbuf;

		return r;
	}

	int port_init()
	{
		handle_t h = comm_init(portID, NULL);
		if (!h) {
			LOGE("___ comm_init failed!\n");
			return -1;
		}
		hand[portID] = h;

		return 0;
	}

	int port_deinit()
	{
		comm_deinit(hand[portID]);
		hand[portID] = NULL;
	}

	int port_open()
	{
		conn_para_t para;

		netPara.mode = 1;
		netPara.para.plat = platPara;
		load_net_para(&netPara);

		para.callback = NULL;
		para.proto = PROTO_MQTT;
		para.para = &netPara;
		conn = comm_open(hand[portID], &para);
		if (!conn) {
			LOGE("___ comm_open failed!\n");
			return -1;
		}
		LOGD("___ comm_open ok!\n");

		prevPortID = portID;

		return 0;
	}
	int port_close(void)
	{
		comm_close(conn);

		return 0;
	}

	int port_read(void* data, int len)
	{
		if (!conn) {
			return -1;
		}

		return comm_recv_data(conn, NULL, data, len);
	}

	int port_write(U8 type, U8 nack, void* data, int len)
	{
		if (!conn) {
			return -1;
		}

		return comm_send_data(conn, NULL, type, nack, data, len);
	}

	int port_pure_write(U8* data, int len)
	{
		if (!conn) {
			return -1;
		}

		return comm_pure_send(conn, NULL, data, len);
	}

	int send_ack(U8 type, U8 err, U8 chkID)
	{
		int len;
		U8 tmp[100];

		len = pkt_pack_ack(type, err, tmp, sizeof(tmp), chkID);
		if (len <= 0) {
			return -1;
		}

		return port_pure_write(tmp, len);
	}

	int data_proc(void* data, int len, U8 chk)
	{
		int r;
		int err = 0;
		node_t nd;
		pkt_hdr_t* hdr = (pkt_hdr_t*)data;

		err = pkt_check_hdr(hdr, len, len, chk);
		if (err == ERROR_NONE) {
			switch (hdr->type) {
			case TYPE_SETT:
			{

			}
			break;

			case TYPE_PARA:
			{
				all_para_t* pa = (all_para_t*)hdr->data;

				if (!para_recved || memcmp(&allPara, pa, sizeof(allPara))) {
					allPara = *pa;
					info_update();

					stop_timer();
					para_recved = 1;
				}
			}
			break;

			case TYPE_ACK:
			{
				ack_t* ack = (ack_t*)hdr->data;
			}
			break;

			case TYPE_CAP:
			{
				if (para_recved) {
					int i, j;
					all_para_t* pa = &allPara;
					ch_data_t* pc = (ch_data_t*)hdr->data;
					ch_para_t* pr = &pa->usr.ch[pc->ch];
					ev_data_t* ev = (ev_data_t*)((U8*)(pc->data) + pc->wavlen);
					const char* ev_str[EV_NUM] = { "rms","amp","asl","ene","ave","min","max" };

					int grps = pc->evlen / (pr->n_ev * sizeof(ev_data_t));
					for (i = 0; i < grps; i++) {
						for (j = 0; j < pr->n_ev; j++) {
							LOGD("%s[%d]: %0.5f\n", ev_str[pr->ev[j]], i, ev[j].data);
						}
					}
					LOGD("\n");
				}
			}
			break;


			case TYPE_CALI:
			{
				cali_sig_t* sig = (cali_sig_t*)hdr->data;

				err = 0;
			}
			break;

			case TYPE_STAT:
			{
				err = 0;
			}
			break;

			case TYPE_HBEAT:
			{
				err = 0;
			}
			break;

			default:
			{
				err = ERROR_PKT_TYPE;
			}
			break;
			}

			if (hdr->askAck || err) {
				send_ack(hdr->type, err, chk);
			}
		}

		return err;
	}


	void start_timer(void)
	{
		if (!para_recved) {
			SetTimer(100, 2000, NULL);
		}
	}

	void stop_timer(void)
	{
		KillTimer(100);
	}

	int load_file(const char *path, char **fbuf, int *flen)
	{
		int r;
		char* buf = NULL;
		struct stat st;

		r = stat(path, &st);
		if (r) {
			return -1;
		}

		FILE* fp = fopen(path, "rt");
		if (!fp) {
			return -1;
		}
		buf = new char[st.st_size];
		if (!buf) {
			fclose(fp);
			return -1;
		}

		r = fread(buf, 1, st.st_size, fp);
		if (fbuf) {
			*fbuf = buf;
		}

		if (flen) {
			*flen = st.st_size;
		}

		fclose(fp);

		return 0;
	}


	LRESULT OnBtnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		int r;
		const char* path = "app.ini";
		CButton btn = hWndCtl;

		if (!dev_opened) {
			if ((wID != ID_OPEN) && (wID != ID_DBG_EN) && (wID != ID_DBG_CLR) && (wID != ID_DBG_SAV)) {
				LOGE("___ dev not opened!\n");
				return -1;
			}
		}
		else {
			if (!para_recved && wID==ID_CONFIG_R) {
				LOGE("___ cfg not recved!\n");
				return -1;
			}
		}

		switch (wID) {
		case ID_OPEN:
		{
			int r;

			if (dev_opened == 0) {
				r = port_open();
				if (r==0) {
					dev_opened = 1;
					start_timer();
					btOpen.SetWindowText("close");
				}
			}
			else {
				stop_timer();

				r = port_close();
				if (r == 0) {
					dev_opened = 0;
					para_recved = 0;

					info_clear();
					cali_cnt = 0;

					btOpen.SetWindowText("open");
				}
			}
		}
		break;

		case ID_CALI_1:
		{
			cali_sig_t sig;
			char* fbuf;
			char tmp[100];
			int r,flen;
			
			r = load_file(path, &fbuf, &flen);
			if (r) {
				LOGE("___ load file %s failed\n", path);
				return -1;
			}

			sig.tms = 1;

			r = get_string(fbuf, flen, "CALI1:", '[', 0, ']', 0, tmp, sizeof(tmp));
			if (r) {
				//MessageBox();
				LOGE("___ CALI1 param is wrong, can't find \"CALI1:\"\n");
				return -1;
			}

			r = sscanf(tmp, "%hd,%f,%d,%f", &sig.ch, &sig.rms, &sig.freq, &sig.bias);
			if (r!=4) {
				LOGE("___ CALI1 param number is wrong!\n");
				return -1;
			}
			r = port_write(TYPE_CALI, 0, &sig, sizeof(sig));

			LOGD("___ CALI-1 ch: %d, rms: %0.5f, freq: %d, bias: %0.5f\n", sig.ch, sig.rms, sig.freq, sig.bias);

			delete[] fbuf;
		}
		break;

		case ID_CALI_2:
		{
			cali_sig_t sig;
			char* fbuf,*p,*p1;
			char tmp[100];
			int r, flen;

			r = load_file(path, &fbuf, &flen);
			if (r) {
				LOGE("___ load file %s failed\n", path);
				return -1;
			}

			if (cali_cnt%2==0) {
				sig.tms = 2;
				
				r = get_string(fbuf, flen, "CALI2:", '[', 0, ']', 0, tmp, sizeof(tmp));
				if (r) {
					//MessageBox();
					return -1;
				}

				r = sscanf(tmp, "%hd,%f,%d,%f]", &sig.ch, &sig.rms, &sig.freq, &sig.bias);
				if (r != 4) {
					LOGE("___ CALI2 param number: %d is wrong!\n", r);
					return -1;
				}

				r = port_write(TYPE_CALI, 0, &sig, sizeof(sig));

				LOGD("___ CALI-2 first, ch: %d, rms: %0.5f, freq: %d, bias: %0.5f\n", sig.ch, sig.rms, sig.freq, sig.bias);
			}
			else {

				r = get_string(fbuf, flen, "CALI2:", '[', 1, ']', 1, tmp, sizeof(tmp));
				if (r) {
					//MessageBox();
					return -1;
				}

				r = sscanf(tmp, "%hd,%f,%d,%f", &sig.ch, &sig.rms, &sig.freq, &sig.bias);
				if (r != 4) {
					LOGE("___ CALI2 param number: %d is wrong!\n", r);
					return -1;
				}

				r = port_write(TYPE_CALI, 0, &sig, sizeof(sig));

				LOGD("___ CALI-2 second, ch: %d, rms: %0.5f, freq: %d, bias: %0.5f\n", sig.ch, sig.rms, sig.freq, sig.bias);
			}

			cali_cnt++;
			delete[] fbuf;
		}
		break;

		case ID_START:
		{
			capture_t cap = {0, !dev_started};

			r = port_write(TYPE_CAP, 0, &cap, sizeof(cap));
			if (r==0) {
				dev_started = dev_started?0:1;

				btStart.SetWindowText(dev_started?"stop":"start");
			}
		}
		break;

		case ID_CONFIG_R:		//读取设备配置文件
		{
			LPCTSTR lpcstrFilter = _T("json Files (*.json)\0*.json\0");
			CFileDialog dlg(TRUE, NULL, _T(""), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, lpcstrFilter);
			if (dlg.DoModal() == IDOK)
			{
				int rlen,jslen = sizeof(usr_para_t)*100;
				char* js = new char[jslen];
				if (js) {
					rlen = json_from(js, jslen, &allPara.usr);
					if (rlen>0) {
						FILE* fp = fopen(dlg.m_ofn.lpstrFile, "wt");
						if (fp) {
							fwrite(js, 1, rlen, fp);
							fclose(fp);
						}
					}
				}
			}
		}
		break;


		case ID_CONFIG_W:		//下发配置文件到设备
		{
			LPCTSTR lpcstrFilter = _T("json Files (*.json)\0*.json\0");
			CFileDialog dlg(TRUE, NULL, _T(""), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, lpcstrFilter);
			if (dlg.DoModal() == IDOK)
			{
				FILE* fp = fopen(dlg.m_ofn.lpstrFile, "rt");
				if (fp) {	//start send upg file
					struct stat st;
					int r = stat(dlg.m_ofn.lpstrFile, &st);
					char* fbuf = new char[st.st_size];
					if (fbuf) {
						usr_para_t usr;
						int rlen = fread(fbuf, 1, st.st_size, fp);
						
						r = json_to(fbuf, &usr);
						if (r == 0) {
							all_para_t all = allPara;

							all.usr = usr;
							r = port_write(TYPE_PARA, 1, &all, sizeof(all));
						}
						delete[] fbuf;
					}
					fclose(fp);
				}
			}
		}
		break;

		case ID_DEFAULT:
		{
			r = port_write(TYPE_DFLT, 0, NULL, 0);
		}
		break;

		case ID_UPGRADE:
		{
			LPCTSTR lpcstrFilter = _T("upg Files (*.upg)\0*.upg\0");
			CFileDialog dlg(TRUE, NULL, _T(""), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, lpcstrFilter);
			if (dlg.DoModal() == IDOK)
			{
				FILE* fp = fopen(dlg.m_ofn.lpstrFile, "r");
				if (fp) {	//start send upg file
					struct stat st;
					int r = stat(dlg.m_ofn.lpstrFile, &st);
					char* fbuf = new char[st.st_size];
					if (fbuf) {
						int rlen = fread(fbuf, 1, st.st_size, fp);

						//send file


						delete [] fbuf;
					}
					fclose(fp);
				}
			}
		}
		break;

		case ID_REBOOT:
		{
			r = port_write(TYPE_REBOOT, 0, NULL, 0);
		}
		break;


		case ID_DBG_EN:
		{
			if (log_is_enable()) {
				log_enable(0);
			}
			else {
				log_enable(1);
			}
		}
		break;

		case ID_DBG_CLR:
		{
			log_clear();
		}
		break;

		case ID_DBG_SAV:
		{
			const char* path = "log.txt";

			int r = log_save(path);
			if (r==0) {
				LOGD("___ %s save ok!\n", path);
			}
		}
		break;

		case ID_DATA_TO:
		{
			static int data_to = 0;
			r = port_write(TYPE_DATATO, 0, NULL, 0);
			if (r==0) {
				data_to = data_to ? 0 : 1;
				btTo.SetWindowText(data_to?"to app":"to ali");
			}
		}
		break;

		}

		
		

		return 0;
	}

	LRESULT OnPortSelect(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
	{
		switch (idCtrl) {
			case ID_PORT:
			break;

			case ID_PORT_LIST:
			break;

		}

		return 0;
	}

	LRESULT OnTabChange(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
	{
		switch (tab.GetCurSel()) {
			case 0:
			pageCmd.ShowWindow(SW_SHOW);
			pageCali.ShowWindow(SW_HIDE);
			pageLog.ShowWindow(SW_HIDE);
			pageSett.ShowWindow(SW_HIDE);
			break;

			case 1:
			pageCmd.ShowWindow(SW_HIDE);
			pageCali.ShowWindow(SW_SHOW);
			pageLog.ShowWindow(SW_HIDE);
			pageSett.ShowWindow(SW_HIDE);
			break;

			case 2:
			pageCmd.ShowWindow(SW_HIDE);
			pageCali.ShowWindow(SW_HIDE);
			pageLog.ShowWindow(SW_SHOW);
			pageSett.ShowWindow(SW_HIDE);
			break;

			case 3:
			pageCmd.ShowWindow(SW_HIDE);
			pageCali.ShowWindow(SW_HIDE);
			pageLog.ShowWindow(SW_HIDE);
			pageSett.ShowWindow(SW_SHOW);
			break;
		}

		return 0;
	}


	LRESULT OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		return 0;
	}

	LRESULT OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		return 0;
	}




	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
		gbl_init();
		split_init();
		pane_init();
		LOGD("___ all init ok!\n");
		
		return 0;
	}

	int t_cnt = 0;
	LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		int r;

		if (dev_opened && !para_recved) {
			r = port_write(TYPE_PARA, 0, NULL, 0);
		}

		return 0;
	}

	LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{

		DestroyWindow();
		PostQuitMessage(0);

		return 0;
	}

	LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		//CPaintDC dc(m_hWnd);
		//CMemoryDC mdc(dc,dc.m_ps.rcPaint);
			
			

		return 0;
	}

	LRESULT DoPaint(CDCHandle dc)
	{
		CRect rc;
		Gdiplus::Point pt1,pt2;
		Gdiplus::Graphics graphics(dc);
		Gdiplus::Pen pen(Gdiplus::Color(255, 0, 0, 255), 1);

		GetClientRect(&rc);

		graphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
		graphics.DrawLine(&pen, pt1, pt2);

		return 0;
	}

	void UpdateLayout(BOOL bResizeBars = TRUE)
	{

	}

	LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		//wavPane.OnSize(uMsg, wParam, lParam, bHandled);
		//infPane.OnSize(uMsg, wParam, lParam, bHandled);
		//ctlPane.OnSize(uMsg, wParam, lParam, bHandled);
		//dbgPane.OnSize(uMsg, wParam, lParam, bHandled);

		//infPane.OnPaint(uMsg, wParam, lParam, bHandled);
		//ctlPane.OnPaint(uMsg, wParam, lParam, bHandled);
		//dbgPane.OnPaint(uMsg, wParam, lParam, bHandled);

		//RedrawWindow();
		//vSplit.OnPaint(uMsg, wParam, lParam, bHandled);

		//
			
		return 0;
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
































