/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ui.h"
#include "msgbox.h"
#include "racecp.h"
#include "racecpui.h"
#include <stdio.h>

struct UI_WINDOW *window_cpsettings;

static struct UI_BUTTON *btn_mainmenu_cplist;
static struct UI_BUTTON *btn_contextmenu_mkracecp;
static struct RADIOBUTTONGROUP *rdbgroup_cptype;
static struct UI_COLORPICKER *cp_colpick;

static int editingCheckpoint;

static
void cb_btn_mkracecp(struct UI_BUTTON *btn)
{
	struct RwV3D posToCreate;
	float x, y;

	if (numcheckpoints == MAX_RACECHECKPOINTS) {
		msg_title = "Race_cp";
		msg_message = "Max_race_checkpoints_reached";
		msg_btn1text = "Ok";
		msg_show(NULL);
		return;
	}

	if (clicked_entity) {
		posToCreate = clicked_colpoint.pos;
	} else {
		x = fresx / 2.0f;
		y = fresy / 2.0f;
		game_ScreenToWorld(&posToCreate, x, y, 40.0f);
	}

	editingCheckpoint = numcheckpoints;
	racecheckpoints[editingCheckpoint].used = 1;
	racecheckpoints[editingCheckpoint].free = 0;
	racecheckpoints[editingCheckpoint].pos = posToCreate;
	racecheckpoints[editingCheckpoint].arrowDirection.x = 10.0f;
	racecheckpoints[editingCheckpoint].arrowDirection.y = 0.0f;
	racecheckpoints[editingCheckpoint].arrowDirection.z = 0.0f;
	racecheckpoints[editingCheckpoint].colABGR = cp_colpick->last_selected_colorABGR;
	ui_rdb_click_match_userdata(rdbgroup_cptype, (void*) 0);
	numcheckpoints++;

	ui_show_window(window_cpsettings);
}

static
void cb_btn_cplist(struct UI_BUTTON *btn)
{
}

static
void cb_rdbgroup_cptype(struct UI_RADIOBUTTON *rdb)
{
	racecheckpoints[editingCheckpoint].type = (char) rdb->_parent._parent.userdata;
}

static
void cb_cp_cpcol(struct UI_COLORPICKER *colpick)
{
	racecheckpoints[editingCheckpoint].colABGR = colpick->last_selected_colorABGR;
	racecheckpoints[editingCheckpoint].used = 0;
	racecheckpoints[editingCheckpoint].free = 1;
}

static
void cb_btn_cpreset(struct UI_BUTTON *btn)
{
	struct RwV3D pos;

	/*TODO: this does horrible things to the arrow etc*/
	pos = racecheckpoints[editingCheckpoint].pos;
	memset(racecheckpoints + editingCheckpoint, 0, sizeof(struct CRaceCheckpoint));
	racecheckpoints[editingCheckpoint].pos = pos;
	racecheckpoints[editingCheckpoint].arrowDirection.x = 0.0f;
	racecheckpoints[editingCheckpoint].arrowDirection.y = 0.0f;
	racecheckpoints[editingCheckpoint].arrowDirection.z = 0.0f;
	racecheckpoints[editingCheckpoint].colABGR = 0xFFFF0000;
	racecheckpoints[editingCheckpoint].used = 1;
	racecheckpoints[editingCheckpoint].free = 0;
	racecheckpoints[editingCheckpoint].fRadius = 10.0f;
	ui_rdb_click_match_userdata(rdbgroup_cptype, (void*) 0);
}

void racecpui_init()
{
	struct UI_BUTTON *btn;
	struct UI_LABEL *lbl;
	struct UI_RADIOBUTTON *rdb;

	/*context menu entry*/
	btn = ui_btn_make("Make_race_CP", cb_btn_mkracecp);
	ui_wnd_add_child(context_menu, btn);
	btn->enabled = 0;
	btn_contextmenu_mkracecp = btn;

	/*main menu entry*/
	lbl = ui_lbl_make("=_Race_CPs_=");
	lbl->_parent.span = 2;
	ui_wnd_add_child(main_menu, lbl);
	btn = ui_btn_make("List", cb_btn_cplist);
	btn->_parent.span = 2;
	ui_wnd_add_child(main_menu, btn);
	btn->enabled = 0;
	btn_mainmenu_cplist = btn;

	/*checkpoint settings window*/
	window_cpsettings = ui_wnd_make(9000.0f, 500.0f, "Checkpoint");
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
	cp_colpick = ui_colpick_make(100.0f, cb_cp_cpcol);
	cp_colpick->_parent.span = 3;
	ui_wnd_add_child(window_cpsettings, cp_colpick);
	btn = ui_btn_make("Reset", cb_btn_cpreset);
	btn->_parent.span = 4;
	ui_wnd_add_child(window_cpsettings, btn);
}

void racecpui_dispose()
{
	ui_wnd_dispose(window_cpsettings);
}

void racecpui_prj_postload()
{
	btn_mainmenu_cplist->enabled = 1;
	btn_contextmenu_mkracecp->enabled = 1;
}
