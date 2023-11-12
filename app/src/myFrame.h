
#pragma once

#include "wtl.h"
#include "log.h"
#include "pkt.h"
#include "comm.h"
#include "mySerial.h"
#include "myMqtt.h"

enum {

	ID_MENU_HOME=0x8000,
	ID_MENU_SETUP,
	ID_MENU_UPGRADE,

	ID_PORT=0x8010,
	ID_PORT_LIST,

	ID_OPEN,
    ID_CALI_1,
	ID_CALI_2,
	ID_START,

	ID_CONFIG_R,
	ID_CONFIG_W,

	ID_UPGRADE,
	ID_REBOOT,

	ID_LOG_EN,
	ID_LOG_CLR,
	ID_LOG_SAV,

	ID_TAB,
	ID_PAGE_CMD,
	ID_PAGE_SETT,
	ID_PAGE_CALI,
};



#define PROD_KEY		"izenceucjUD"
#define PROD_SECRET		"xWYqWMNG3XQe7aEZ"
#define DEV_NAME		"PCTOOL"
#define DEV_SECRET		"5e6c2795ee8c04552a84c39f304fa4d5"

#define HOST_URL        "iot-06z00cq4vq4hkzx.mqtt.iothub.aliyuncs.com"
#define HOST_PORT       1883

static meta_data_t meta = {
	PROD_KEY,
	PROD_SECRET,
	DEV_NAME,
	DEV_SECRET,

	HOST_URL,
	HOST_PORT,
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
static myMqtt mMqtt;


class myFrame : public CFrameWindowImpl<myFrame>,
	public CUpdateUI<myFrame>,
	public CMessageFilter,
	public CIdleHandler
{

public:
	myFrame();
	~myFrame();

	CFont   gFont;
	CTabCtrl tab;
	CEdit    info;
	CPagerCtrl pageCmd,pageSett,pageCali;
	CComboBox  port,portList;

	void* hconn;

	CSplitterWindow    vSplit;		//主窗口，垂直分割
	CHorSplitterWindow hSplit1;		//右窗口，水平分割
	CHorSplitterWindow hSplit2;
	CPaneContainer     wavPane, infPane, cmdPane, dbgPane;
	CButton btOpen, btCali, btStart;
	CButton btEna, btSav, btClr;
	CButton btCfgr, btCfgw, btUpg,btBoot;
	CButton btCali1, btCali2;

	handle_t hand[PORT_MAX] = {NULL};
	int  portID = 0;
	mySerial mSerial;

	int dev_opened=0;
	int dev_started=0;

	int log_started=0;
	int log_enabled=0;

	int proto = 1;		//0: tcp   1: mqtt

	BEGIN_MSG_MAP(myFrame)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_CLOSE, OnClose)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)

		MESSAGE_HANDLER(WM_SIZING, OnSize)
		//MESSAGE_HANDLER(WM_PAINT, OnPaint)
		//MSG_WM_CLOSE(OnClose)

		//MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
		//MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)

		//COMMAND_RANGE_CODE_HANDLER(ID_OPEN, ID_LOG_SAV, BN_CLICKED, OnBtnClicked)

		COMMAND_HANDLER(ID_OPEN, BN_CLICKED, OnBtnClicked)
		//COMMAND_ID_HANDLER(ID_OPEN, OnBtnClicked)

		//NOTIFY_HANDLER(IDC_TABCTRLDOWN, TCN_SELCHANGE, OnSelChangeTab)
		
		NOTIFY_RANGE_CODE_HANDLER(ID_PORT, ID_PORT_LIST, CBN_SELENDOK, OnPortSelect)

		NOTIFY_HANDLER(ID_TAB, TCN_SELCHANGE, OnTabChange)

		//FORWARD_NOTIFICATIONS()
		//REFLECT_NOTIFICATIONS()

		CHAIN_MSG_MAP(CUpdateUI<myFrame>)
		CHAIN_MSG_MAP(CFrameWindowImpl<myFrame>)

	END_MSG_MAP()

	BEGIN_UPDATE_UI_MAP(myFrame)
		UPDATE_ELEMENT(ID_OPEN, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_START, UPDUI_RADIO)
	END_UPDATE_UI_MAP()



public:

	virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
		//if (CFrameWindowImpl<myFrame>::PreTranslateMessage(pMsg))
		//	return TRUE;

		//return m_view.PreTranslateMessage(pMsg);//传递给其他窗口函数。
		return FALSE;
	}

	virtual BOOL OnIdle()
	{
		UIUpdateToolBar();

		return FALSE;
	}

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

	void port_init(void)
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

	void infPaneInit(void)
	{
		CRect rc;
	
		infPane.GetClientRect(&rc);
		info.Create(infPane.m_hWnd, rc, NULL, WS_CHILD | ES_LEFT | ES_NOHIDESEL | WS_TABSTOP | ES_MULTILINE | ES_WANTRETURN | WS_VSCROLL | ES_AUTOVSCROLL);
		infPane.SetClient(info.m_hWnd);
		//::SetWindowPos(h, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

		//info.SetReadOnly(1);
		//info.LimitText(5000);
		//info.ShowScrollBar(SB_VERT);
		//info.SetFont(gFont);

		info.AppendText("9999999999999\r\n");


		//infPaneUpdate();
	}

	
	void infPaneUpdate(void)
	{
		int i;
		TCHAR tmp[100];
		
		info.Clear();

		extern all_para_t allPara;
		sys_para_t* sys = &allPara.sys;
		sprintf(tmp, "fw.version: %s\n", sys->para.fwInfo.version);	info.AppendText(tmp);
		sprintf(tmp, "fw.bldtime: %s\n", sys->para.fwInfo.bldtime);	info.AppendText(tmp);
		sprintf(tmp, "fw.length: %d\n", sys->para.fwInfo.length);	info.AppendText(tmp);
		sprintf(tmp, "fw.length: %d\n", sys->para.fwInfo.length);	info.AppendText(tmp);
		sprintf(tmp, "sys.mode: %d\n", sys->stat.mode);				info.AppendText(tmp);
		sprintf(tmp, "sys.state: %d\n", sys->stat.state);			info.AppendText(tmp);

		usr_para_t* usr = &allPara.usr;
		sprintf(tmp, "card.apn: %s\n", usr->card.apn);				info.AppendText(tmp);
		sprintf(tmp, "card.type: %d\n", usr->card.type);			info.AppendText(tmp);
		sprintf(tmp, "card.auth: %d\n", usr->card.auth);			info.AppendText(tmp);
		if (usr->net.mode == 0) {
			sprintf(tmp, "net.ip: %s\n", usr->net.para.tcp.ip);				info.AppendText(tmp);
			sprintf(tmp, "net.port: %d\n", usr->net.para.tcp.port);				info.AppendText(tmp);
		}
		else {
			sprintf(tmp, "net.host: %s\n", usr->net.para.plat.host);				info.AppendText(tmp);
			sprintf(tmp, "net.port: %d\n", usr->net.para.plat.port);				info.AppendText(tmp);
			sprintf(tmp, "net.devKey: %s\n", usr->net.para.plat.devKey);			info.AppendText(tmp);
			sprintf(tmp, "net.devSec: %s\n", usr->net.para.plat.devSecret);		info.AppendText(tmp);
			sprintf(tmp, "net.prdKey: %s\n", usr->net.para.plat.prdKey);			info.AppendText(tmp);
			sprintf(tmp, "net.prdSec: %s\n", usr->net.para.plat.prdSecret);		info.AppendText(tmp);
		}
		sprintf(tmp, "smp.smp_mode: %d\n", usr->smp.smp_mode);		info.AppendText(tmp);
		sprintf(tmp, "smp.smp_period: %ds\n", usr->smp.smp_period);	info.AppendText(tmp);
		
		for (i = 0; i < CH_MAX; i++) {
			sprintf(tmp, "ch[%d].upway: %d\n",		i, usr->ch[i].upway);       info.AppendText(tmp);
			sprintf(tmp, "ch[%d].upwav: %d\n",		i, usr->ch[i].upwav);       info.AppendText(tmp);
			sprintf(tmp, "ch[%d].smpFreq: %d\n",	i, usr->ch[i].smpFreq);     info.AppendText(tmp);
			sprintf(tmp, "ch[%d].smpTime: %d\n",	i, usr->ch[i].smpTime);     info.AppendText(tmp);
			sprintf(tmp, "ch[%d].n_ev: %d\n",		i, usr->ch[i].n_ev);        info.AppendText(tmp);
			sprintf(tmp, "ch[%d].evCalcCnt: %d\n",	i, usr->ch[i].evCalcCnt);   info.AppendText(tmp);
			sprintf(tmp, "ch[%d].coef.a: %f\n",		i, usr->ch[i].coef.a);      info.AppendText(tmp);
			sprintf(tmp, "ch[%d].coef.b: %f\n",		i, usr->ch[i].coef.b);      info.AppendText(tmp);
		}
	}

	void ctlPaneInit(void)
	{
		CRect rc;

		cmdPane.GetClientRect(&rc);
		tab.Create(cmdPane.m_hWnd, rc, NULL, WS_CHILD | WS_VISIBLE, NULL, ID_TAB, NULL);
		cmdPane.SetClient(tab.m_hWnd);

		tab.SetFont(gFont);
		tab.InsertItem(0, "command");
		tab.InsertItem(1, "calibration");
		tab.InsertItem(2, "setting");

		tab.GetClientRect(&rc);
		pageCmd.Create(tab.m_hWnd,  rc, NULL, WS_CHILD | WS_VISIBLE, NULL, ID_PAGE_CMD, NULL);
		pageSett.Create(tab.m_hWnd, rc, NULL, WS_CHILD | WS_VISIBLE, NULL, ID_PAGE_SETT, NULL);
		pageCali.Create(tab.m_hWnd, rc, NULL, WS_CHILD | WS_VISIBLE, NULL, ID_PAGE_CALI, NULL);

		rc.top += 21;
		rc.bottom -= 1;
		rc.left += 1;
		rc.right -= 1;

		tab.SetCurSel(0);

		pageCmd.MoveWindow(rc);
		pageCmd.ShowWindow(SW_SHOW);

		pageSett.MoveWindow(rc);
		pageSett.ShowWindow(SW_HIDE);

		pageCali.MoveWindow(rc);
		pageCali.ShowWindow(SW_HIDE);

#if 1
		
#define BTN_WIDTH   80
#define BTN_HEIGHT  30

#define BTN_WGAP    10
#define BTN_HGAP    20

		HWND hwnd = pageCmd.m_hWnd;
		pageCmd.GetClientRect(&rc);

		//command buttons
		rc.left = 20;
		rc.right = rc.left + BTN_WIDTH;
		rc.top = 20;
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

		//log control button
		rc.left = 20;
		rc.top = rc.bottom + BTN_HGAP;
		rc.right = rc.left + BTN_WIDTH;
		rc.bottom = rc.top + BTN_HEIGHT;
		btEna.Create(hwnd, rc, "log off", WS_CHILD | WS_VISIBLE, NULL, ID_LOG_EN, NULL);
		btEna.SetFont(gFont);

		rc.left = rc.right + BTN_WGAP;
		rc.right = rc.left + BTN_WIDTH;
		btClr.Create(hwnd, rc, "log clr", WS_CHILD | WS_VISIBLE, NULL, ID_LOG_CLR, NULL);
		btClr.SetFont(gFont);

		rc.left = rc.right + BTN_WGAP;
		rc.right = rc.left + BTN_WIDTH;
		btSav.Create(hwnd, rc, "log save", WS_CHILD | WS_VISIBLE, NULL, ID_LOG_SAV, NULL);
		btSav.SetFont(gFont);

		rc.left = rc.right + BTN_WGAP;
		rc.right = rc.left + BTN_WIDTH;
		btUpg.Create(hwnd, rc, "upgrade", WS_CHILD | WS_VISIBLE, NULL, ID_UPGRADE, NULL);
		btUpg.SetFont(gFont);

		rc.left = rc.right + BTN_WGAP;
		rc.right = rc.left + BTN_WIDTH;
		btBoot.Create(hwnd, rc, "reboot", WS_CHILD | WS_VISIBLE, NULL, ID_REBOOT, NULL);
		btBoot.SetFont(gFont);


		hwnd = pageCali.m_hWnd;
		pageCali.GetClientRect(&rc);

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

		hwnd = pageSett.m_hWnd;
		pageSett.GetClientRect(&rc);

		rc.left = 20;
		rc.right = rc.left + PORT_WIDTH;
		rc.top = 40;
		rc.bottom = rc.top + PORT_HEIGHT;
		port.Create(hwnd, rc, NULL, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_HASSTRINGS, NULL, ID_PORT, NULL);
		port_init();

		rc.left = rc.right + 20;
		rc.right = rc.left + PORT_WIDTH;
		portList.Create(hwnd, rc, NULL, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_HASSTRINGS, NULL, ID_PORT_LIST, NULL);
		port_list_refresh();





#endif
	}

	void dbgPaneInit(void)
	{
		CRect rc;

		dbgPane.GetClientRect(&rc);
		log_init(dbgPane.m_hWnd, rc, gFont);
		dbgPane.SetClient(log_get_hwnd());
	}

	void gbl_init(void)
	{
		ModifyStyle(WS_THICKFRAME, 0);
		ModifyStyle(WS_MAXIMIZEBOX, 0);

		gFont.CreateFont(16, 8, 0, 0, FW_THIN, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
			CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, TEXT("Arial"));

		//gFont.CreatePointFont(80, TEXT("Arial"));

		//CClientDC dc(m_hWnd);
		//dc.SetTextColor(RGB(0, 0, 0));

		

	}

	void button_test(void)
	{
		CRect rc;
		CButton bt;

		GetClientRect(&rc);

		rc.left = 20;
		rc.top = 20;
		rc.bottom = 60;
		rc.right = 60;
		//https://learn.microsoft.com/zh-cn/windows/win32/controls/button-styles?redirectedfrom=MSDN
		//BS_AUTORADIOBUTTON 单选框
		//BS_CHECKBOX 复选框，右侧带有标题的选择框
		//BS_RADIOBUTTON 单选按钮，右边显示文本
		//BS_GROUPBOX 指定一个组框
		bt.Create(m_hWnd, rc, NULL, WS_CHILD | WS_VISIBLE, NULL, ID_OPEN, NULL);
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

		vSplit.SetSinglePaneMode(SPLIT_PANE_RIGHT);		//暂时不显示wav，待wav部分完成后再放开

		wavPane.Create(vSplit.m_hWnd);
		wavPane.SetTitle(_T("wave"));
		wavPane.SetPaneContainerExtendedStyle(PANECNT_NOCLOSEBUTTON);

		vSplit.GetClientRect(&rc);
		hSplit1.Create(vSplit.m_hWnd, rc, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
		hSplit1.SetSplitterPos(rc.bottom / 3);			//info占垂直窗口1/3

		vSplit.SetSplitterPanes(wavPane, hSplit1);
		
		infPane.Create(hSplit1.m_hWnd);
		infPane.SetTitle(_T("info"));
		infPane.SetPaneContainerExtendedStyle(PANECNT_NOCLOSEBUTTON);

		/////////////////////////////////////////////////////////////////////
		hSplit1.GetClientRect(&rc);
		hSplit2.Create(hSplit1.m_hWnd, rc, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
		hSplit2.SetSplitterPos(rc.bottom / 2);			//ctl和dbg各占剩下垂直窗口1/2

		hSplit1.SetSplitterPanes(infPane, hSplit2);

		cmdPane.Create(hSplit2.m_hWnd);
		cmdPane.SetTitle(_T("command"));
		cmdPane.SetPaneContainerExtendedStyle(PANECNT_NOCLOSEBUTTON);

		dbgPane.Create(hSplit2.m_hWnd);
		dbgPane.SetTitle(_T("debug"));
		dbgPane.SetPaneContainerExtendedStyle(PANECNT_NOCLOSEBUTTON);
		
		hSplit2.SetSplitterPanes(cmdPane, dbgPane);
	}

	void pane_init(void)
	{
		infPaneInit();
		ctlPaneInit();
		dbgPaneInit();
	}

	void mqtt_open(void)
	{
		hconn = mMqtt.conn(&meta);
		if (!hconn) {
			LOGE("___ mqtt conn failed!\n");
			return;
		}
		LOGE("___ mqtt conn ok!\n");

		mqtt_sub();
	}
	void mqtt_close(void)
	{
		mMqtt.disconn(hconn);
	}
	void mqtt_sub(void)
	{
		char tmp[200];

		snprintf(tmp, sizeof(tmp), "/%s/%s/user/get", PROD_KEY, DEV_NAME);
		mMqtt.subscribe(hconn, tmp);

		snprintf(tmp, sizeof(tmp), "/%s/%s/user/set", PROD_KEY, DEV_NAME);
		mMqtt.subscribe(hconn, tmp);
	}

	int mqtt_read(void* buf, int buflen)
	{
		return mMqtt.read(hconn, buf, buflen);
	}
	int mqtt_write(U8 type, U8 nAck, void* data, int len)
	{
		int r;
		char topic[200];
		char tmp[1000];

		//pkt_pack();
		sprintf(topic, "/%s/%s/user/set", PROD_KEY, DEV_NAME);
		r = mMqtt.publish(hconn, topic, tmp, 1);

		return r;
	}

	int set_port(int port)
	{
		if (port >= PORT_MAX) {
			return -1;
		}

		portID = port;
		return 0;
	}

	int port_open()
	{
		if (portID ==PORT_MQTT) {
			mqtt_open();
		}
		else {
			if (!hand[portID]) {
				hand[portID] = comm_init(PORT_UART, &portID,  NULL);
			}
		}

		return 0;
	}
	int port_close(void)
	{
		if (!hand[portID]) {
			return -1;
		}
		
		comm_deinit(hand[portID]);
		hand[portID] = NULL;

		return 0;
	}

	int port_read(void* data, int len)
	{
		if (portID == PORT_MQTT) {
			return mqtt_read(data, len);
		}
		else {
			return comm_recv_data(hand[portID], NULL, data, len);
		}
	}

	int port_write(U8 type, U8 nack, void* data, int len)
	{
		if (portID==PORT_MQTT) {
			return mqtt_write(type, nack, data, len);
		}
		else {
			return comm_send_data(hand[portID], NULL, type, nack, data, len);
		}
	}

	//SetTimer(88, 200, NULL);

	LRESULT OnBtnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		int r;
		CButton btn = hWndCtl;

		switch (wID) {
		case ID_OPEN:
		{
			if (dev_opened == 0) {
				port_open();
			}
			else {
				port_close();
			}
			dev_opened = dev_opened?0:1;
			btOpen.SetWindowText(dev_opened?"close":"open");
		}
		break;

		case ID_CALI_1:
		case ID_CALI_2:
		{
			cali_sig_t sig;

			sig.ch = 0;
			sig.rms = 40.0f;
			sig.freq = 40000;
			sig.bias = 0.0f;
			r = port_write(TYPE_CALI, 0, &sig, sizeof(sig));
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

		case ID_CONFIG_R:
		{
			LPCTSTR lpcstrFilter = _T("json Files (*.json)\0*.json\0");
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


						delete[] fbuf;
					}
					fclose(fp);
				}
			}
		}
		break;


		case ID_CONFIG_W:
		{
			LPCTSTR lpcstrFilter = _T("json Files (*.json)\0*.json\0");
			CFileDialog dlg(FALSE, NULL, _T(""), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, lpcstrFilter);
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


						delete[] fbuf;
					}
					fclose(fp);
				}
			}
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
			//comm_send();
		}
		break;


		case ID_LOG_EN:
		{
			if (log_is_enable()) {
				log_enable(0);
			}
			else {
				log_enable(1);
			}
		}
		break;

		case ID_LOG_CLR:
		{
			log_clear();
		}
		break;

		case ID_LOG_SAV:
		{
			//extern void writeFile(U8 ch, void* data, int len);
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
			pageSett.ShowWindow(SW_HIDE);
			break;

			case 1:
			pageCmd.ShowWindow(SW_HIDE);
			pageCali.ShowWindow(SW_SHOW);
			pageSett.ShowWindow(SW_HIDE);
			break;

			case 2:
			pageCmd.ShowWindow(SW_HIDE);
			pageCali.ShowWindow(SW_HIDE);
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
		extern CAppModule appModule;
		CMessageLoop* pLoop = appModule.GetMessageLoop();

		ATLASSERT(pLoop != NULL);

		//HWND hWndToolBar = CreateSimpleToolBarCtrl(m_hWnd, 2000, FALSE, ATL_SIMPLE_TOOLBAR_PANE_STYLE);
		//CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
		//AddSimpleReBarBand(hWndToolBar, NULL, TRUE);
		//UpdateLayout();
		//UIAddToolBar(hWndToolBar);

		UISetCheck(ID_OPEN, TRUE);

		pLoop->AddMessageFilter(this);
		pLoop->AddIdleHandler(this);

		gbl_init();
		split_init();
		pane_init();

		//mqtt_open();

		LOGD("___ all init ok!\n");

		return 0;
	}

	int t_cnt = 0;
	LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{


		LOGD("____cnt: %d\n", t_cnt++);

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



myFrame::myFrame()
{
	ackEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
}


myFrame::~myFrame()
{
	port_close();
	CloseHandle(ackEvent);
}































