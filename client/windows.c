/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ui.h"
#include "vk.h"
#include "windows.h"
#include <string.h>

struct UI_WINDOW *window_settings;
struct UI_WINDOW *window_cpsettings;

static
void toggle_foliage(int should_be_enabled)
{
	static unsigned char foliageCall[5] = { 0, 0, 0, 0, 0 };

	void* position;

	if (*((unsigned char*) 0x53C159) == 0x90) {
		if (should_be_enabled) {
			memcpy((void*) 0x53C159, foliageCall, 5);
			foliageCall[0] = 0;
		}
	} else if (!should_be_enabled) {
		memcpy(foliageCall, (void*) 0x53C159, 5);
		memset((void*) 0x53C159, 0x90, 5);

		/*remove existing foliage*/
		position = (void*) 0xB6F03C; /*_camera.__parent.m_pCoords*/
		if (position == NULL) {
			/*_camera.__parent.placement*/
			position = (void*) 0xB6F02C;
		}
		((void (__cdecl *)(void*,int)) 0x5DC510)(position, 0);
	}
}

static
void cb_rdb_foliage(struct UI_RADIOBUTTON *rdb)
{
	toggle_foliage((int) rdb->_parent._parent.userdata);
}

static
void cb_rdb_keys(struct UI_RADIOBUTTON *rdb)
{
	if ((int) rdb->_parent._parent.userdata) {
		key_w = VK_Z;
		key_a = VK_Q;
	} else {
		key_w = VK_W;
		key_a = VK_A;
	}
}

static
void cb_rdb_movement(struct UI_RADIOBUTTON *rdb)
{
	directional_movement = (int) rdb->_parent._parent.userdata;
}

static
void cb_btn_uifontsize(struct UI_BUTTON *btn)
{
	int data;

	data = (int) btn->_parent.userdata;
	if (data < 0 && fontsize > -25) {
		fontsize--;
		ui_set_fontsize(fontsize, fontratio);
	} else if (data > 0 && fontsize < 25) {
		fontsize++;
		ui_set_fontsize(fontsize, fontratio);
	} else if (data == 0) {
		ui_set_fontsize(UI_DEFAULT_FONT_SIZE, fontratio);
	}
}

static
void cb_btn_uifontratio(struct UI_BUTTON *btn)
{
	int data;

	data = (int) btn->_parent.userdata;
	if (data < 0 && fontratio > -25) {
		fontratio--;
		ui_set_fontsize(fontsize, fontratio);
	} else if (data > 0 && fontratio < 25) {
		fontratio++;
		ui_set_fontsize(fontsize, fontratio);
	} else if (data == 0) {
		ui_set_fontsize(fontsize, UI_DEFAULT_FONT_RATIO);
	}
}

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

	/*            settings*/

	/*TODO: movements stuff*/
	window_settings = ui_wnd_make(500.0f, 500.0f, "Settings");
	window_settings->columns = 4;

	lbl = ui_lbl_make("Foliage:");
	ui_wnd_add_child(window_settings, lbl);
	rdbgroup = ui_rdbgroup_make(cb_rdb_foliage);
	rdb = ui_rdb_make("on", rdbgroup, 1);
	rdb->_parent._parent.userdata = (void*) 1;
	rdb->_parent._parent.span = 2;
	ui_wnd_add_child(window_settings, rdb);
	rdb = ui_rdb_make("off", rdbgroup, 0);
	rdb->_parent._parent.userdata = 0;
	ui_wnd_add_child(window_settings, rdb);

	lbl = ui_lbl_make("Keys:");
	ui_wnd_add_child(window_settings, lbl);
	rdbgroup = ui_rdbgroup_make(cb_rdb_keys);
	rdb = ui_rdb_make("zqsd", rdbgroup, 1);
	rdb->_parent._parent.userdata = (void*) 1;
	rdb->_parent._parent.span = 2;
	ui_wnd_add_child(window_settings, rdb);
	rdb = ui_rdb_make("wasd", rdbgroup, 0);
	rdb->_parent._parent.userdata = 0;
	ui_wnd_add_child(window_settings, rdb);

	lbl = ui_lbl_make("Movement:");
	ui_wnd_add_child(window_settings, lbl);
	rdbgroup = ui_rdbgroup_make(cb_rdb_movement);
	rdb = ui_rdb_make("directional", rdbgroup, 1);
	rdb->_parent._parent.userdata = (void*) 1;
	rdb->_parent._parent.span = 2;
	ui_wnd_add_child(window_settings, rdb);
	rdb = ui_rdb_make("flat", rdbgroup, 0);
	rdb->_parent._parent.userdata = 0;
	ui_wnd_add_child(window_settings, rdb);

	lbl = ui_lbl_make("UI_font_size:");
	ui_wnd_add_child(window_settings, lbl);
	btn = ui_btn_make("-", cb_btn_uifontsize);
	btn->_parent.userdata = (void*) -1;
	ui_wnd_add_child(window_settings, btn);
	btn = ui_btn_make("+", cb_btn_uifontsize);
	btn->_parent.userdata = (void*) 1;
	ui_wnd_add_child(window_settings, btn);
	btn = ui_btn_make("reset", cb_btn_uifontsize);
	btn->_parent.userdata = (void*) 0;
	ui_wnd_add_child(window_settings, btn);

	lbl = ui_lbl_make("UI_font_ratio:");
	ui_wnd_add_child(window_settings, lbl);
	btn = ui_btn_make("-", cb_btn_uifontratio);
	btn->_parent.userdata = (void*) -1;
	ui_wnd_add_child(window_settings, btn);
	btn = ui_btn_make("+", cb_btn_uifontratio);
	btn->_parent.userdata = (void*) 1;
	ui_wnd_add_child(window_settings, btn);
	btn = ui_btn_make("reset", cb_btn_uifontratio);
	btn->_parent.userdata = (void*) 0;
	ui_wnd_add_child(window_settings, btn);

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
	ui_wnd_dispose(window_settings);
	ui_wnd_dispose(window_cpsettings);
	toggle_foliage(1);
}
