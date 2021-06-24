#include <windows.h>
#include "win.h"
#include "port.h"
#include "log.h"
#include "task.h"
#include "icfg.h"
#include "com.h"
#include "paras.h"

#pragma comment  (lib, "libui.lib")

int port_opened = 0;
extern setts_t  curSetts;
extern sett_t* pCurSett;
static U8 valve_open = 0;
extern int exit_flag;
static grp_t     grp;
static int       mcuSim;
uiMultilineEntry* logEntry = NULL;

static int onClosing(uiWindow* w, void* data)
{
	uiQuit();
	return 1;
}
static int onShouldQuit(void* data)
{
	uiWindow* win = uiWindow(data);

	uiControlDestroy(uiControl(win));
	return 1;
}


////////////////////////////////////////////////////
static label_btn_t* label_btn_new(const char* label, const char* btn_txt, button_clik_t fn)
{
	label_btn_t* lb = (label_btn_t*)malloc(sizeof(label_btn_t));

	if (!lb) {
		return NULL;
	}

	lb->grid = uiNewGrid(); uiGridSetPadded(lb->grid, 1);
	lb->label = uiNewLabel(label);

	lb->button = uiNewButton(btn_txt);
	uiButtonOnClicked(lb->button, fn, NULL);

	uiGridAppend(lb->grid, uiControl(lb->label),  0, 0, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
	uiGridAppend(lb->grid, uiControl(lb->button), 1, 0, 1, 1, 0, uiAlignFill, 0, uiAlignFill);

	return lb;
}
static label_cmb_t* label_comb_new(const char* label, combo_sel_t fn, void* user_data, const char **item, int dft)
{
	int i = 0;
	label_cmb_t* lc = (label_cmb_t*)malloc(sizeof(label_cmb_t));

	if (!lc) {
		return NULL;
	}

	lc->grid = uiNewGrid(); uiGridSetPadded(lc->grid, 1);
	lc->label = uiNewLabel(label);
	lc->combo = uiNewCombobox();
	while(item[i]) { 
		uiComboboxAppend(lc->combo, item[i]); i++;
	}
	uiComboboxSetSelected(lc->combo, dft);
	uiComboboxOnSelected(lc->combo, fn, NULL);

	uiGridAppend(lc->grid, uiControl(lc->label), 1, 0, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
	uiGridAppend(lc->grid, uiControl(lc->combo), 2, 0, 1, 1, 0, uiAlignFill, 0, uiAlignFill);

	return lc;
}




static label_input_t* label_input_new(const char* label, const char *value, const char* unit)
{
	label_input_t* li = (label_input_t*)malloc(sizeof(label_input_t));

	if (!li) {
		return NULL;
	}

	li->grid = uiNewGrid(); uiGridSetPadded(li->grid, 1);
	li->label = uiNewLabel(label); 
	li->entry = uiNewEntry();			uiEntrySetText(li->entry, value);
	li->unit = uiNewLabel(unit);

	uiGridAppend(li->grid, uiControl(li->label),  0, 0, 1, 1, 0, uiAlignEnd, 0, uiAlignFill);
	uiGridAppend(li->grid, uiControl(li->entry),  1, 0, 1, 1, 0, uiAlignEnd, 0, uiAlignFill);
	uiGridAppend(li->grid, uiControl(li->unit),   2, 0, 1, 1, 0, uiAlignEnd, 0, uiAlignFill);

	return li;
}
//////////////////////////////////////////////////////
static void on_port_combo_sel_fn(uiCombobox* c, void* data)
{
	uiWindow* win = uiWindow(data);
	
}
static void on_port_open_btn_fn(uiButton* b, void* data)
{
	int r=0;
	uiCombobox* c = uiCombobox(data);
	int port_id = uiComboboxSelected(c)+1;

	if (port_opened) {
		uiButtonSetText(b, "Open");
		port_close();
		port_opened = 0;
		r = 0;
	}
	else {
		r = port_open(port_id);
		if (r) {
			uiMsgBoxError(grp.win, "Open Failed", "Please Check The Port And Try Again.");
		}
		else {
			port_opened = 1;
			com_send_paras(0);
			uiButtonSetText(b, "Close");
		}
	}
}
static void on_port_chkbox_fn(uiCheckbox* c, void* data)
{
	if (uiCheckboxChecked(c)) {
		task_app_stop();
		task_dev_start();
	}
	else {
		task_dev_stop();
		task_app_start();
	}
}
static int port_grp_init(uiWindow* win, port_grp_t* port)
{
	int i;
	char tmp[50];

	port->grid = uiNewGrid();					uiGridSetPadded(port->grid, 1);
	port->grp = uiNewGroup("Port Select");
	port->hbox = uiNewHorizontalBox();
	////////////////
	port->g = uiNewGrid();					uiGridSetPadded(port->g, 1);
	port->port = uiNewCombobox();
	for (i = 1; i <= MAX_PORT; i++) {
		snprintf(tmp, sizeof(tmp), "COM%d", i);
		uiComboboxAppend(port->port, tmp);
		uiComboboxOnSelected(port->port, on_port_combo_sel_fn, NULL);
	}
	uiComboboxSetSelected(port->port, DEF_PORT-1);

	port->open = uiNewButton("Open");			uiButtonOnClicked(port->open, on_port_open_btn_fn, port->port);
	port->mode = uiNewCheckbox("MCU Simulate"); uiCheckboxOnToggled(port->mode, on_port_chkbox_fn, NULL); uiCheckboxSetChecked(port->mode, mcuSim);
	if (mcuSim) {
		task_dev_start();
	}
	else {
		task_app_start();
	}
	                                            
	uiGridAppend(port->g, uiControl(port->port), 0, 0, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
	uiGridAppend(port->g, uiControl(port->open), 1, 0, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
	uiGridAppend(port->g, uiControl(port->mode), 2, 0, 1, 1, 0, uiAlignFill, 0, uiAlignFill);

	uiBoxAppend(port->hbox, uiControl(port->g), 1);
	uiGroupSetChild(port->grp, uiControl(port->hbox));
	uiGridAppend(port->grid, uiControl(port->grp), 0, 0, 1, 1, 1, uiAlignFill, 0, uiAlignFill);

	return 0;
}



static void on_sett_set_fn(uiButton* b, void* data)
{
	setts_t *setts=&curParas.setts;
	if (!com_get_paras_flag()) {
		uiMsgBoxError(grp.win, "Set Failed", "Paras not received!");
		return;
	}
	com_send_data(TYPE_SETT, 1, setts, sizeof(setts_t));
}
static void on_sett_valve_fn(uiButton* b, void* data)
{
	para_grp_t* para = (para_grp_t*)data;

	valve_open = !valve_open;
	cmd_t cmd = { CMD_VALVE_SET , valve_open };
	com_send_data(TYPE_CMD, 0, &cmd, sizeof(cmd));

	uiButtonSetText(para->sett.valve, valve_open?"Valve Open":"Valve Close");
	uiButtonSetText(para->sett.start, "Start");
}

static void on_sett_start_btn_fn(uiButton* b, void* data)
{
	U8 err;
	cmd_t cmd;

	if(strcmp(uiButtonText(b), "Start")==0) {
		cmd.cmd = CMD_PUMP_START;cmd.para = 0;
		err = com_send_data(TYPE_CMD, 0, &cmd, sizeof(cmd));
		if (err == 0) {
			uiButtonSetText(b, "Stop");
			LOG("start the pump...\n");
		}
	}
	else if (strcmp(uiButtonText(b), "Stop") == 0) {
		cmd.cmd = CMD_PUMP_STOP;cmd.para = 0;
		err = com_send_data(TYPE_CMD, 0, &cmd, sizeof(cmd));
		if (err == 0) {
			uiButtonSetText(b, "Start");
			LOG("stop the pump...\n");
		}
	}
}
static int para_grp_init(uiWindow* win, para_grp_t* para)
{
	const char* valveStr[] = {"ON", "OFF", NULL};

	para->grid      = uiNewGrid(); uiGridSetPadded(para->grid, 1);
	para->sett.grp  = uiNewGroup("Pump Setting"); 
	para->sett.vbox = uiNewVerticalBox();uiBoxSetPadded(para->sett.vbox, 1);
	para->sett.grid = uiNewGrid();       uiGridSetPadded(para->sett.grid, 1);

	const char* modStr[] = {
		"CONTINUS",
		"INTERVAL",
		"FIXED TIME",
		"FIXED VOLUME",
		NULL
	};
	para->sett.mode = label_comb_new("Mode", NULL, NULL, modStr, 0);
	para->sett.w_time = label_input_new("wTime", "0", "s");
	para->sett.s_time = label_input_new("sTime", "0", "s");
	para->sett.t_time = label_input_new("tTime", "0", "s");
	para->sett.vacuum = label_input_new("Pres ", "0", "kpa");
	para->sett.maxVol = label_input_new("Vol  ", "0", "ml");
	para->sett.set    = uiNewButton("Set");   uiButtonOnClicked(para->sett.set, on_sett_set_fn, NULL);
	para->sett.valve  = uiNewButton("Valve Close"); uiButtonOnClicked(para->sett.valve, on_sett_valve_fn, para);
	para->sett.start  = uiNewButton("Start"); uiButtonOnClicked(para->sett.start, on_sett_start_btn_fn, NULL);

	uiGridAppend(para->sett.grid, uiControl(para->sett.mode->grid),   0, 0, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
	uiGridAppend(para->sett.grid, uiControl(para->sett.w_time->grid), 0, 1, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
	uiGridAppend(para->sett.grid, uiControl(para->sett.s_time->grid), 0, 2, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
	uiGridAppend(para->sett.grid, uiControl(para->sett.t_time->grid), 0, 3, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
	uiGridAppend(para->sett.grid, uiControl(para->sett.vacuum->grid), 0, 4, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
	uiGridAppend(para->sett.grid, uiControl(para->sett.maxVol->grid), 0, 5, 1, 1, 0, uiAlignFill, 0, uiAlignFill);

	uiGridAppend(para->sett.grid, uiControl(para->sett.set),		  3, 3, 1, 1, 1, uiAlignCenter, 0, uiAlignFill);
	uiGridAppend(para->sett.grid, uiControl(para->sett.valve),        3, 4, 1, 1, 1, uiAlignCenter, 0, uiAlignFill);
	uiGridAppend(para->sett.grid, uiControl(para->sett.start),        0, 6, 4, 1, 1, uiAlignFill, 0, uiAlignFill);

	uiBoxAppend(para->sett.vbox, uiControl(para->sett.grid), 0);
	uiGroupSetChild(para->sett.grp, uiControl(para->sett.vbox));

	////////////////////////////////////////////////////////////
	para->stat.grp    = uiNewGroup("Pump State");
	para->stat.vbox   = uiNewHorizontalBox();    uiBoxSetPadded(para->stat.vbox, 1);
	para->stat.grid   = uiNewGrid();             uiGridSetPadded(para->stat.grid, 1);

	para->stat.stat   = uiNewLabel("stat:");
	para->stat.vacuum = uiNewLabel("Vacuum:");
	para->stat.pressure = uiNewLabel("Pressure:");
	para->stat.tempure  = uiNewLabel("Tempure:");
	para->stat.speed    = uiNewLabel("Speed:");
	para->stat.current  = uiNewLabel("Current:");

	uiGridAppend(para->stat.grid, uiControl(para->stat.stat),    0, 0, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
	uiGridAppend(para->stat.grid, uiControl(para->stat.vacuum),    0, 2, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
	uiGridAppend(para->stat.grid, uiControl(para->stat.pressure),   0, 3, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
	uiGridAppend(para->stat.grid, uiControl(para->stat.tempure),   0, 4, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
	uiGridAppend(para->stat.grid, uiControl(para->stat.speed),  0, 5, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
	uiGridAppend(para->stat.grid, uiControl(para->stat.current), 0, 6, 1, 1, 0, uiAlignFill, 0, uiAlignFill);

	uiBoxAppend(para->stat.vbox, uiControl(para->stat.grid), 1);
	uiGroupSetChild(para->stat.grp, uiControl(para->stat.vbox));
	//					left   top     hspan   vspan   hexpand    halign         vexpand     valign
	uiGridAppend(para->grid, uiControl(para->sett.grp), 0, 0, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
	uiGridAppend(para->grid, uiControl(para->stat.grp), 1, 0, 1, 1, 1, uiAlignFill, 0, uiAlignFill);

	return 0;
}

extern node_t upg_node;
static void on_upg_open_btn_fn(uiButton* b, void* data)
{
	char* filename;
	uiWindow* win = uiWindow(data);

	filename = uiOpenFile(win);
	if (filename) {
		//uiEntrySetText(entry, "");
		return;
	}
}

static void on_upg_start_btn_fn(uiButton* b, void* data)
{

}
static int upg_grp_init(uiWindow* win, upg_grp_t* upg)
{
	upg->grid = uiNewGrid();					uiGridSetPadded(upg->grid, 1);
	upg->grp  = uiNewGroup("Firmware Upgrade");
	upg->vbox = uiNewVerticalBox();
	////////////////
	upg->g    = uiNewGrid();					uiGridSetPadded(upg->g, 1);

	upg->fwinfo = uiNewLabel("fw info: ");
	upg->path = uiNewEntry();					uiEntrySetReadOnly(upg->path, 1);
	upg->open = uiNewButton("Open");			uiButtonOnClicked(upg->open, on_upg_open_btn_fn, win);
	upg->pgbar = uiNewProgressBar();			uiProgressBarSetValue(upg->pgbar, 0);
	upg->start = uiNewButton("Start");			uiButtonOnClicked(upg->start, on_upg_start_btn_fn, NULL);
	
	uiGridAppend(upg->g, uiControl(upg->fwinfo), 0, 0, 1, 1, 1, uiAlignFill,   0, uiAlignFill);
	uiGridAppend(upg->g, uiControl(upg->path),   0, 1, 1, 1, 1, uiAlignFill,   0, uiAlignFill);
	uiGridAppend(upg->g, uiControl(upg->open),   1, 1, 1, 1, 0, uiAlignFill,   0, uiAlignFill);
	uiGridAppend(upg->g, uiControl(upg->pgbar),  0, 2, 2, 1, 1, uiAlignFill,   0, uiAlignFill);
	uiGridAppend(upg->g, uiControl(upg->start),  0, 3, 1, 1, 0, uiAlignCenter, 0, uiAlignFill);

	uiBoxAppend(upg->vbox, uiControl(upg->g), 1);
	uiGroupSetChild(upg->grp, uiControl(upg->vbox));
	uiGridAppend(upg->grid, uiControl(upg->grp), 0, 0, 1, 1, 1, uiAlignFill, 0, uiAlignFill);

	return 0;
}


static void on_log_enable_btn_fn(uiButton* b, void* data)
{
	if (logx_get()==0) {
		logx_en(1);
		uiButtonSetText(b, "Enable");
	}
	else {
		logx_en(0);
		uiButtonSetText(b, "Disable");
	}
}
static void on_log_clear_btn_fn(uiButton* b, void* data)
{
	logx_clr();
}
static void on_log_save_btn_fn(uiButton* b, void* data)
{
	char* filename;
	uiWindow* win = uiWindow(data);

	filename = uiSaveFile(win);
	if (filename) {
		logx_save(filename);
		return;
	}
}

static int log_grp_init(uiWindow* win, log_grp_t* log)
{
	log->grid = uiNewGrid();	 uiGridSetPadded(log->grid, 1);
	log->grp  = uiNewGroup("Log Infomation");
	log->vbox = uiNewVerticalBox();

	log->g = uiNewGrid();		uiGridSetPadded(log->g, 1);
	log->enable = uiNewButton("Disable");uiButtonOnClicked(log->enable, on_log_enable_btn_fn, win);
	log->clear  = uiNewButton("Clear"); uiButtonOnClicked(log->clear,  on_log_clear_btn_fn, win);
	log->save   = uiNewButton("Save");  uiButtonOnClicked(log->save,   on_log_save_btn_fn, win);
	log->entry  = uiNewMultilineEntry();uiMultilineEntrySetReadOnly(log->entry, 1);
	logEntry    = log->entry;

	//                                              x  y  xs ys hex halign     vex  valign
	uiGridAppend(log->g, uiControl(log->enable), 0, 0, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
	uiGridAppend(log->g, uiControl(log->clear),  1, 0, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
	uiGridAppend(log->g, uiControl(log->save),   2, 0, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
	uiGridAppend(log->g, uiControl(log->entry),  0, 1, 3, 1, 1, uiAlignFill, 1, uiAlignFill);

	uiBoxAppend(log->vbox, uiControl(log->g), 1);
	uiGroupSetChild(log->grp, uiControl(log->vbox));
	uiGridAppend(log->grid, uiControl(log->grp), 0, 1, 1, 1, 1, uiAlignFill, 1, uiAlignFill);

	return 0;
}



static void ui_init(void)
{
	const char* err;
	uiInitOptions options;

	memset(&options, 0, sizeof(options));
	err = uiInit(&options);
	if (err != NULL) {
		uiFreeInitError(err);
		return;
	}

	grp.win = uiNewWindow("winSim", SCR_WIDTH, SCR_HEIGHT, 1);
	uiWindowOnClosing(grp.win, onClosing, grp.win);
	uiOnShouldQuit(onShouldQuit, grp.win);

	grp.grid = uiNewGrid();	 uiGridSetPadded(grp.grid, 1);
	uiWindowSetChild(grp.win, uiControl(grp.grid));

	port_grp_init(grp.win, &grp.port);
	para_grp_init(grp.win, &grp.para);
	upg_grp_init(grp.win, &grp.upg);
	log_grp_init(grp.win, &grp.log);

	uiGridAppend(grp.grid, uiControl(grp.port.grid), 0, 0, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
	uiGridAppend(grp.grid, uiControl(grp.para.grid), 0, 1, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
	uiGridAppend(grp.grid, uiControl(grp.upg.grid),  0, 2, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
	uiGridAppend(grp.grid, uiControl(grp.log.grid),  0, 3, 1, 1, 1, uiAlignFill, 1, uiAlignFill);

	uiControlShow(uiControl(grp.win));
}


static DWORD WINAPI uiThread(LPVOID lpParam)
{
	ui_init();
	uiMain();
	uiUninit();
	port_close();
	logEntry = NULL;
	task_quit();

	return 0;
}

void win_init(int mcu)
{
	mcuSim = mcu;
	CreateThread(NULL, 0, uiThread, NULL, 0, NULL);
}


const char* statString[STAT_MAX] = {
	"stop",
	"running",
	"upgrade",
};
const char* modeString[MODE_MAX] = {
	"continus",
	"interval",
	"fixed-time",
	"fixed-volume",
};


void win_paras_update(paras_t* paras)
{
	char tmp[100];
#if 1
	if (!paras) {
		grp.para.sett.mode = label_comb_new("Mode", NULL, NULL, modeString, 0);
		grp.para.sett.w_time = label_input_new("wTime", "0", "s");
		grp.para.sett.s_time = label_input_new("sTime", "0", "s");
		grp.para.sett.t_time = label_input_new("tTime", "0", "s");
		grp.para.sett.vacuum = label_input_new("Pres ", "0", "kpa");
		grp.para.sett.maxVol = label_input_new("Vol  ", "0", "ml");
	}
	else {
		snprintf(tmp, sizeof(tmp), "%d", paras->setts.mode);
		uiComboboxSetSelected(grp.para.sett.mode->combo, tmp);

		snprintf(tmp, sizeof(tmp), "%d", paras->setts.sett[paras->setts.mode].time.work_time);
		uiEntrySetText(grp.para.sett.w_time->entry, tmp);

		snprintf(tmp, sizeof(tmp), "%d", paras->setts.sett[paras->setts.mode].time.stop_time);
		uiEntrySetText(grp.para.sett.s_time->entry, tmp);

		snprintf(tmp, sizeof(tmp), "%2.1f", paras->setts.sett[paras->setts.mode].pres);
		uiEntrySetText(grp.para.sett.vacuum->entry, tmp);

		snprintf(tmp, sizeof(tmp), "%d", (int)paras->setts.sett[paras->setts.mode].maxVol);
		uiEntrySetText(grp.para.sett.maxVol->entry, tmp);

		snprintf(tmp, sizeof(tmp), "fw info: %s, %s", paras->fwInfo.version, paras->fwInfo.bldtime);
		uiLabelSetText(grp.upg.fwinfo, (const char*)tmp);
	}
#endif
}

void win_stat_update(stat_t *stat)
{
	char tmp[100];

	snprintf(tmp, sizeof(tmp), "State: %s", statString[stat->sysState]);
	uiLabelSetText(grp.para.stat.stat, (const char*)tmp);

	snprintf(tmp, sizeof(tmp), "Pressure: %.1f", stat->aPres);
	uiLabelSetText(grp.para.stat.vacuum, (const char*)tmp);

	snprintf(tmp, sizeof(tmp), "Vacuum: %.1f", stat->dPres);
	uiLabelSetText(grp.para.stat.pressure, (const char*)tmp);

	snprintf(tmp, sizeof(tmp), "Tempure: %.1f", stat->temp);
	uiLabelSetText(grp.para.stat.tempure, (const char*)tmp);

	snprintf(tmp, sizeof(tmp), "Speed: %d", stat->pump.speed);
	uiLabelSetText(grp.para.stat.speed, (const char*)tmp);

	snprintf(tmp, sizeof(tmp), "Current: %.1f", stat->pump.current);
	uiLabelSetText(grp.para.stat.current, (const char*)tmp);

	uiButtonSetText(grp.para.sett.start, (stat->sysState==STAT_STOP)?"Start": "Stop");
}





