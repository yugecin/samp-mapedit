/* vim: set filetype=c ts=8 noexpandtab: */

#include "bulkedit.h"
#include "bulkeditui.h"
#include "common.h"
#include "ui.h"
#include <string.h>

struct UI_WINDOW *bulkeditui_wnd;
char bulkeditui_shown;

static
void cb_posrdbgroup_changed(struct UI_RADIOBUTTON *rdb)
{
	bulkedit_pos_update_method = rdb->_parent._parent.userdata;
	bulkedit_update();
}

static
void cb_rotrdbgroup_changed(struct UI_RADIOBUTTON *rdb)
{
	bulkedit_rot_update_method = rdb->_parent._parent.userdata;
	bulkedit_update();
}

static
void cb_directionrdbgroup_changed(struct UI_RADIOBUTTON *rdb)
{
	bulkedit_direction_add_90 = rdb->_parent._parent.userdata == (void*) 1;
	bulkedit_direction_remove_90 = rdb->_parent._parent.userdata == (void*) 2;
	bulkedit_direction_add_180 = rdb->_parent._parent.userdata == (void*) 3;
	bulkedit_update();
}

static
void cb_btn_commit(struct UI_BUTTON *btn)
{
	bulkedit_commit();
}

static
void cb_btn_disband(struct UI_BUTTON *btn)
{
	bulkedit_revert();
	bulkedit_reset();
	bulkeditui_hide();
}

static
void cb_btn_revert(struct UI_BUTTON *btn)
{
	bulkedit_revert();
}

void bulkeditui_toggle()
{
	bulkeditui_shown = !bulkeditui_shown;
}

void bulkeditui_hide()
{
	bulkeditui_shown = 0;
}

void bulkeditui_init()
{
	struct UI_LABEL *lbl;
	struct UI_BUTTON *btn;
	struct UI_RADIOBUTTON *rdb;
	struct RADIOBUTTONGROUP *posrdbgroup, *rotrdbgroup, *directionrdbgroup;

	bulkeditui_wnd = ui_wnd_make(200.0f, 20.0f, "Bulkedit");
	bulkeditui_wnd->columns = 3;
	bulkeditui_wnd->closeable = 0;

	posrdbgroup = ui_rdbgroup_make(cb_posrdbgroup_changed);
	rotrdbgroup = ui_rdbgroup_make(cb_rotrdbgroup_changed);
	directionrdbgroup = ui_rdbgroup_make(cb_directionrdbgroup_changed);
	ui_wnd_add_child(bulkeditui_wnd, ui_lbl_make("position"));
	ui_wnd_add_child(bulkeditui_wnd, lbl = ui_lbl_make("rotation"));
	lbl->_parent.span = 2;
	rdb = ui_rdb_make("Ignore", posrdbgroup, 0);
	rdb->_parent._parent.userdata = (void*) bulkedit_update_nop;
	ui_wnd_add_child(bulkeditui_wnd, rdb);
	rdb = ui_rdb_make("Ignore", rotrdbgroup, 0);
	rdb->_parent._parent.span = 2;
	rdb->_parent._parent.userdata = (void*) bulkedit_update_nop;
	ui_wnd_add_child(bulkeditui_wnd, rdb);
	rdb = ui_rdb_make("Sync_movement", posrdbgroup, 1);
	rdb->_parent._parent.userdata = (void*) bulkedit_update_pos_sync;
	ui_wnd_add_child(bulkeditui_wnd, rdb);
	rdb = ui_rdb_make("Sync_movement", rotrdbgroup, 1);
	rdb->_parent._parent.span = 2;
	rdb->_parent._parent.userdata = (void*) bulkedit_update_rot_sync;
	ui_wnd_add_child(bulkeditui_wnd, rdb);
	rdb = ui_rdb_make("Spread", posrdbgroup, 0);
	rdb->_parent._parent.userdata = (void*) bulkedit_update_pos_spread;
	ui_wnd_add_child(bulkeditui_wnd, rdb);
	rdb = ui_rdb_make("Spread", rotrdbgroup, 0);
	rdb->_parent._parent.span = 2;
	rdb->_parent._parent.userdata = (void*) bulkedit_update_rot_spread;
	ui_wnd_add_child(bulkeditui_wnd, rdb);
	rdb = ui_rdb_make("Copy_x", posrdbgroup, 0);
	rdb->_parent._parent.userdata = (void*) bulkedit_update_pos_copyx;
	ui_wnd_add_child(bulkeditui_wnd, rdb);
	rdb = ui_rdb_make("Object_direction", rotrdbgroup, 0);
	rdb->_parent._parent.span = 2;
	rdb->_parent._parent.userdata = (void*) bulkedit_update_rot_object_direction;
	ui_wnd_add_child(bulkeditui_wnd, rdb);
	rdb = ui_rdb_make("Copy_y", posrdbgroup, 0);
	rdb->_parent._parent.userdata = (void*) bulkedit_update_pos_copyy;
	ui_wnd_add_child(bulkeditui_wnd, rdb);
	ui_wnd_add_child(bulkeditui_wnd, NULL);
	rdb = ui_rdb_make("+0", directionrdbgroup, 1);
	rdb->_parent._parent.userdata = (void*) 0;
	ui_wnd_add_child(bulkeditui_wnd, rdb);
	rdb = ui_rdb_make("Copy_z", posrdbgroup, 0);
	rdb->_parent._parent.userdata = (void*) bulkedit_update_pos_copyz;
	ui_wnd_add_child(bulkeditui_wnd, rdb);
	ui_wnd_add_child(bulkeditui_wnd, NULL);
	rdb = ui_rdb_make("+90", directionrdbgroup, 0);
	rdb->_parent._parent.userdata = (void*) 1;
	ui_wnd_add_child(bulkeditui_wnd, rdb);
	ui_wnd_add_child(bulkeditui_wnd, NULL);
	ui_wnd_add_child(bulkeditui_wnd, NULL);
	rdb = ui_rdb_make("-90", directionrdbgroup, 0);
	rdb->_parent._parent.userdata = (void*) 2;
	ui_wnd_add_child(bulkeditui_wnd, rdb);
	ui_wnd_add_child(bulkeditui_wnd, NULL);
	ui_wnd_add_child(bulkeditui_wnd, NULL);
	rdb = ui_rdb_make("+180", directionrdbgroup, 0);
	rdb->_parent._parent.userdata = (void*) 3;
	ui_wnd_add_child(bulkeditui_wnd, rdb);

	btn = ui_btn_make("Commit_positions", cb_btn_commit);
	btn->_parent.span = 3;
	ui_wnd_add_child(bulkeditui_wnd, btn);
	btn = ui_btn_make("Disband_bulk_edit", cb_btn_disband);
	btn->_parent.span = 3;
	ui_wnd_add_child(bulkeditui_wnd, btn);
	btn = ui_btn_make("Revert_bulk_edit", cb_btn_revert);
	btn->_parent.span = 3;
	ui_wnd_add_child(bulkeditui_wnd, btn);
}

void bulkeditui_dispose()
{
	ui_wnd_dispose(bulkeditui_wnd);
}