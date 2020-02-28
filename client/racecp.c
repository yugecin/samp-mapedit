/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ui.h"
#include <stdio.h>

struct UI_WINDOW *window_cpsettings;

static struct RADIOBUTTONGROUP *rdbgroup_cptype;

static
void cb_btn_mkracecp(struct UI_BUTTON *btn)
{
	ui_show_window(window_cpsettings);
}

static
void cb_btn_cplist(struct UI_BUTTON *btn)
{
}

static
void cb_rdbgroup_cptype(struct UI_RADIOBUTTON *rdb)
{
	racecheckpoints[0].type = (char) rdb->_parent._parent.userdata;
	racecheckpoints[0].used = 1;
	racecheckpoints[0].free = 0;
}

static
void cb_cp_cpcol(struct UI_COLORPICKER *colpick)
{
	racecheckpoints[0].colABGR = colpick->last_selected_colorABGR;
	racecheckpoints[0].used = 1;
	racecheckpoints[0].free = 3;
}

static
void cb_btn_cpreset(struct UI_BUTTON *btn)
{
	float x, y, z;

	/*TODO: this does horrible things to the arrow etc*/
	x = racecheckpoints[0].pos.x;
	y = racecheckpoints[0].pos.y;
	z = racecheckpoints[0].pos.z;
	memset(racecheckpoints, 0, sizeof(struct CRaceCheckpoint));
	racecheckpoints[0].pos.x = x;
	racecheckpoints[0].pos.y = y;
	racecheckpoints[0].pos.z = z;
	racecheckpoints[0].arrowDirection.x = x + 100.0f;
	racecheckpoints[0].arrowDirection.y = y;
	racecheckpoints[0].arrowDirection.z = z;
	racecheckpoints[0].colABGR = 0xFFFF0000;
	racecheckpoints[0].used = 1;
	racecheckpoints[0].free = 3;
	racecheckpoints[0].type = 0;
	racecheckpoints[0].fRadius = 10.0f;
	ui_rdb_click_match_userdata(rdbgroup_cptype, (void*) 0);
}

void racecp_init()
{
	struct UI_BUTTON *btn;
	struct UI_LABEL *lbl;
	struct UI_RADIOBUTTON *rdb;
	struct UI_COLORPICKER *colpick;

	/*context menu entry*/
	btn = ui_btn_make("Make_race_CP", cb_btn_mkracecp);
	ui_wnd_add_child(context_menu, btn);

	/*main menu entry*/
	lbl = ui_lbl_make("=_Race_CPs_=");
	lbl->_parent.span = 2;
	ui_wnd_add_child(main_menu, lbl);
	btn = ui_btn_make("List", cb_btn_cplist);
	btn->_parent.span = 2;
	ui_wnd_add_child(main_menu, btn);

	/*checkpoint settings window*/
	window_cpsettings = ui_wnd_make(500.0f, 500.0f, "Checkpoint");
	window_cpsettings->columns = 4;
	lbl = ui_lbl_make("Type:");
	ui_wnd_add_child(window_cpsettings, lbl);
	rdbgroup_cptype = ui_rdbgroup_make(cb_rdbgroup_cptype);
	rdb = ui_rdb_make("0._Arrow", rdbgroup_cptype, 1);
	rdb->_parent._parent.span = 3;
	rdb->_parent._parent.userdata = 0;
	ui_wnd_add_child(window_cpsettings, rdb);
	ui_wnd_add_child(window_cpsettings, NULL);
	rdb = ui_rdb_make("1._Finish", rdbgroup_cptype, 0);
	rdb->_parent._parent.span = 3;
	rdb->_parent._parent.userdata = (void*) 1;
	ui_wnd_add_child(window_cpsettings, rdb);
	ui_wnd_add_child(window_cpsettings, NULL);
	rdb = ui_rdb_make("2._Normal", rdbgroup_cptype, 0);
	rdb->_parent._parent.span = 3;
	rdb->_parent._parent.userdata = (void*) 2;
	ui_wnd_add_child(window_cpsettings, rdb);
	ui_wnd_add_child(window_cpsettings, NULL);
	rdb = ui_rdb_make("3._Air_normal", rdbgroup_cptype, 0);
	rdb->_parent._parent.span = 3;
	rdb->_parent._parent.userdata = (void*) 3;
	ui_wnd_add_child(window_cpsettings, rdb);
	ui_wnd_add_child(window_cpsettings, NULL);
	rdb = ui_rdb_make("4._Air_finish", rdbgroup_cptype, 0);
	rdb->_parent._parent.span = 3;
	rdb->_parent._parent.userdata = (void*) 4;
	ui_wnd_add_child(window_cpsettings, rdb);
	ui_wnd_add_child(window_cpsettings, NULL);
	rdb = ui_rdb_make("5._Air_rotate", rdbgroup_cptype, 0);
	rdb->_parent._parent.span = 3;
	rdb->_parent._parent.userdata = (void*) 5;
	ui_wnd_add_child(window_cpsettings, rdb);
	ui_wnd_add_child(window_cpsettings, NULL);
	rdb = ui_rdb_make("6._Air_up_down_nothing", rdbgroup_cptype, 0);
	rdb->_parent._parent.span = 3;
	rdb->_parent._parent.userdata = (void*) 6;
	ui_wnd_add_child(window_cpsettings, rdb);
	ui_wnd_add_child(window_cpsettings, NULL);
	rdb = ui_rdb_make("7._Air_up_down_1", rdbgroup_cptype, 0);
	rdb->_parent._parent.span = 3;
	rdb->_parent._parent.userdata = (void*) 7;
	ui_wnd_add_child(window_cpsettings, rdb);
	ui_wnd_add_child(window_cpsettings, NULL);
	rdb = ui_rdb_make("8._Air_up_down_2", rdbgroup_cptype, 0);
	rdb->_parent._parent.span = 3;
	rdb->_parent._parent.userdata = (void*) 8;
	ui_wnd_add_child(window_cpsettings, rdb);
	lbl = ui_lbl_make("Color:");
	ui_wnd_add_child(window_cpsettings, lbl);
	colpick = ui_colpick_make(100.0f, cb_cp_cpcol);
	colpick->_parent.span = 3;
	ui_wnd_add_child(window_cpsettings, colpick);
	btn = ui_btn_make("Reset", cb_btn_cpreset);
	btn->_parent.span = 4;
	ui_wnd_add_child(window_cpsettings, btn);
}

void racecp_dispose()
{
	ui_wnd_dispose(window_cpsettings);
}

void racecp_prj_save(FILE *f, char *buf)
{
}

int racecp_prj_load_line(char *buf)
{
	return 0;
}

void racecp_prj_postload()
{
}
