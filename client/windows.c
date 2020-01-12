/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ui.h"
#include "windows.h"
#include <string.h>

struct UI_WINDOW *window_cpsettings;

static
void cb_rdbgroup_cptype(struct UI_RADIOBUTTON *rdb)
{
	racecheckpoints[0].type = (char) rdb->_parent._parent.userdata;
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
	racecheckpoints[0].pos.z = z;
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
}

void wnd_init()
{
	struct RADIOBUTTONGROUP *rdbgroup;
	struct UI_RADIOBUTTON *rdb;
	struct UI_LABEL *lbl;
	struct UI_BUTTON *btn;
	struct UI_COLORPICKER *colpick;

	/*            checkpoint settings*/

	window_cpsettings = ui_wnd_make(500.0f, 500.0f, "Checkpoint");
	window_cpsettings->columns = 4;

	lbl = ui_lbl_make("Type:");
	ui_wnd_add_child(window_cpsettings, lbl);
	rdbgroup = ui_rdbgroup_make(cb_rdbgroup_cptype);
	rdb = ui_rdb_make("0._Arrow", rdbgroup, 1);
	rdb->_parent._parent.span = 3;
	rdb->_parent._parent.userdata = 0;
	ui_wnd_add_child(window_cpsettings, rdb);
	ui_wnd_add_child(window_cpsettings, NULL);
	rdb = ui_rdb_make("1._Finish", rdbgroup, 0);
	rdb->_parent._parent.span = 3;
	rdb->_parent._parent.userdata = (void*) 1;
	ui_wnd_add_child(window_cpsettings, rdb);
	ui_wnd_add_child(window_cpsettings, NULL);
	rdb = ui_rdb_make("2._Normal", rdbgroup, 0);
	rdb->_parent._parent.span = 3;
	rdb->_parent._parent.userdata = (void*) 2;
	ui_wnd_add_child(window_cpsettings, rdb);
	ui_wnd_add_child(window_cpsettings, NULL);
	rdb = ui_rdb_make("3._Air_normal", rdbgroup, 0);
	rdb->_parent._parent.span = 3;
	rdb->_parent._parent.userdata = (void*) 3;
	ui_wnd_add_child(window_cpsettings, rdb);
	ui_wnd_add_child(window_cpsettings, NULL);
	rdb = ui_rdb_make("4._Air_finish", rdbgroup, 0);
	rdb->_parent._parent.span = 3;
	rdb->_parent._parent.userdata = (void*) 4;
	ui_wnd_add_child(window_cpsettings, rdb);
	ui_wnd_add_child(window_cpsettings, NULL);
	rdb = ui_rdb_make("5._Air_rotate", rdbgroup, 0);
	rdb->_parent._parent.span = 3;
	rdb->_parent._parent.userdata = (void*) 5;
	ui_wnd_add_child(window_cpsettings, rdb);
	ui_wnd_add_child(window_cpsettings, NULL);
	rdb = ui_rdb_make("6._Air_up_down_nothing", rdbgroup, 0);
	rdb->_parent._parent.span = 3;
	rdb->_parent._parent.userdata = (void*) 6;
	ui_wnd_add_child(window_cpsettings, rdb);
	ui_wnd_add_child(window_cpsettings, NULL);
	rdb = ui_rdb_make("7._Air_up_down_1", rdbgroup, 0);
	rdb->_parent._parent.span = 3;
	rdb->_parent._parent.userdata = (void*) 7;
	ui_wnd_add_child(window_cpsettings, rdb);
	ui_wnd_add_child(window_cpsettings, NULL);
	rdb = ui_rdb_make("8._Air_up_down_2", rdbgroup, 0);
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

void wnd_dispose()
{
	ui_wnd_dispose(window_cpsettings);
}
