#ifndef __WIN_Hx__
#define __WIN_Hx__

#include "ui.h"
#include "data.h"

typedef void (*button_clik_t)(uiButton* b, void* data);
typedef void (*combo_sel_t)(uiCombobox* c, void* data);

typedef struct {
	uiGrid*				grid;
	uiLabel*			label;
	uiButton*			button;
}label_btn_t;

typedef struct {
	uiGrid*				grid;
	uiLabel*			label;
	uiCombobox*			combo;
}label_cmb_t;

typedef struct {
	uiGrid*				grid;
	uiLabel*			label;
	uiEntry*			entry;
	uiLabel*			unit;
}label_input_t;
//////////////////////////////////
typedef struct {
	uiGrid*				grid;

	uiGroup*			grp;
	uiBox*				hbox;
	uiGrid*				g;
	uiCombobox*			port;
	uiButton*			open;
	uiCheckbox*		    mode;
}port_grp_t;

typedef struct {
	uiGrid*				grid;

	struct {
		uiGroup*		grp;
		uiBox*          vbox;
		uiGrid*			grid;

		label_cmb_t*	mode;
		label_input_t*	w_time;
		label_input_t*	s_time;
		label_input_t*	t_time;
		label_input_t*	vacuum;
		label_input_t*	maxVol;

		uiButton*		set;

		uiButton*		start;
	}sett;

	struct {
		uiGroup*		grp;
		uiBox*			vbox;
		uiGrid*			grid;

		uiLabel*		mode;
		uiLabel*		temp;
		uiLabel*		speed;
		uiLabel*		valve;
		uiLabel*		vacuum;
		uiLabel*		current;
	}stat;
}para_grp_t;

typedef struct {
	uiGrid*				grid;

	uiGroup*			grp;
	uiBox*				vbox;
	uiGrid*				g;

	uiEntry*			path;
	uiButton*			open;

	uiProgressBar*		pgbar;
	uiButton*			start;
}upg_grp_t;

typedef struct {
	uiGrid*				grid;

	uiGroup*			grp;
	uiBox*				vbox;
	uiGrid*				g;
	uiButton*			enable;
	uiButton*			clear;
	uiButton*			save;

	uiMultilineEntry*   entry;
}log_grp_t;

typedef struct {
	uiWindow*			win;
	uiGrid*				grid;

	port_grp_t			port;
	para_grp_t			para;
	upg_grp_t			upg;
	log_grp_t			log;
}grp_t;


void win_init(int mcu);
void win_stat_update(stat_t* stat);

#endif
