/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ui.h"
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
void cb_rdb_foliage(struct RADIOBUTTONGROUP *grp, void *data)
{
	toggle_foliage((int) data);
}

static
void cb_btn_uifontsup(struct UI_BUTTON *btn)
{
	if (fontsize < 25) {
		fontsize++;
		ui_set_fontsize(fontsize, fontratio);
	}
}

static
void cb_btn_uifontsdown(struct UI_BUTTON *btn)
{
	if (fontsize > -25) {
		fontsize--;
		ui_set_fontsize(fontsize, fontratio);
	}
}

static
void cb_btn_uifontrup(struct UI_BUTTON *btn)
{
	if (fontratio < 25) {
		fontratio++;
		ui_set_fontsize(fontsize, fontratio);
	}
}

static
void cb_btn_uifontrdown(struct UI_BUTTON *btn)
{
	if (fontratio > -25) {
		fontratio--;
		ui_set_fontsize(fontsize, fontratio);
	}
}

static
void cb_btn_cptype(struct UI_BUTTON *btn)
{
	racecheckpoints[0].type = (char) btn->_parent.userdata;
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

	/*TODO: movements stuff (keys & mode)*/
	window_settings = ui_wnd_make(500.0f, 500.0f, "Settings");
	window_settings->columns = 3;

	lbl = ui_lbl_make(0.0f, 0.0f, "Foliage:");
	ui_wnd_add_child(window_settings, lbl);
	rdbgroup = ui_rdbgroup_make(cb_rdb_foliage);
	rdb = ui_rdb_make(0.0f, 0.0f, "on", rdbgroup, 1);
	rdb->data = (void*) 1;
	ui_wnd_add_child(window_settings, rdb);
	rdb = ui_rdb_make(0.0f, 0.0f, "off", rdbgroup, 0);
	rdb->data = 0;
	ui_wnd_add_child(window_settings, rdb);

	lbl = ui_lbl_make(0.0f, 0.0f, "UI_font_size:");
	ui_wnd_add_child(window_settings, lbl);
	btn = ui_btn_make(0.0f, 0.0f, "-", cb_btn_uifontsdown);
	ui_wnd_add_child(window_settings, btn);
	btn = ui_btn_make(0.0f, 0.0f, "+", cb_btn_uifontsup);
	ui_wnd_add_child(window_settings, btn);

	lbl = ui_lbl_make(0.0f, 0.0f, "UI_font_ratio:");
	ui_wnd_add_child(window_settings, lbl);
	btn = ui_btn_make(0.0f, 0.0f, "-", cb_btn_uifontrdown);
	ui_wnd_add_child(window_settings, btn);
	btn = ui_btn_make(0.0f, 0.0f, "+", cb_btn_uifontrup);
	ui_wnd_add_child(window_settings, btn);

	/*            checkpoint settings*/

	window_cpsettings = ui_wnd_make(500.0f, 500.0f, "Checkpoint");
	window_cpsettings->columns = 4;

	lbl = ui_lbl_make(0.0f, 0.0f, "Type:");
	ui_wnd_add_child(window_cpsettings, lbl);
	btn = ui_btn_make(0.0f, 0.0f, "0_Arrow", cb_btn_cptype);
	btn->_parent.span = 3;
	btn->_parent.userdata = 0;
	ui_wnd_add_child(window_cpsettings, btn);
	ui_wnd_add_child(window_cpsettings, NULL);
	btn = ui_btn_make(0.0f, 0.0f, "1_Finish", cb_btn_cptype);
	btn->_parent.span = 3;
	btn->_parent.userdata = (void*) 1;
	ui_wnd_add_child(window_cpsettings, btn);
	ui_wnd_add_child(window_cpsettings, NULL);
	btn = ui_btn_make(0.0f, 0.0f, "2_Normal", cb_btn_cptype);
	btn->_parent.span = 3;
	btn->_parent.userdata = (void*) 2;
	ui_wnd_add_child(window_cpsettings, btn);
	ui_wnd_add_child(window_cpsettings, NULL);
	btn = ui_btn_make(0.0f, 0.0f, "3_Air_normal", cb_btn_cptype);
	btn->_parent.span = 3;
	btn->_parent.userdata = (void*) 3;
	ui_wnd_add_child(window_cpsettings, btn);
	ui_wnd_add_child(window_cpsettings, NULL);
	btn = ui_btn_make(0.0f, 0.0f, "4_Air_finish", cb_btn_cptype);
	btn->_parent.span = 3;
	btn->_parent.userdata = (void*) 4;
	ui_wnd_add_child(window_cpsettings, btn);
	ui_wnd_add_child(window_cpsettings, NULL);
	btn = ui_btn_make(0.0f, 0.0f, "5_Air_rotate", cb_btn_cptype);
	btn->_parent.span = 3;
	btn->_parent.userdata = (void*) 5;
	ui_wnd_add_child(window_cpsettings, btn);
	ui_wnd_add_child(window_cpsettings, NULL);
	btn = ui_btn_make(0.0f, 0.0f, "6_Air_up_down_nothing", cb_btn_cptype);
	btn->_parent.span = 3;
	btn->_parent.userdata = (void*) 6;
	ui_wnd_add_child(window_cpsettings, btn);
	ui_wnd_add_child(window_cpsettings, NULL);
	btn = ui_btn_make(0.0f, 0.0f, "7_Air_up_down_1", cb_btn_cptype);
	btn->_parent.span = 3;
	btn->_parent.userdata = (void*) 7;
	ui_wnd_add_child(window_cpsettings, btn);
	ui_wnd_add_child(window_cpsettings, NULL);
	btn = ui_btn_make(0.0f, 0.0f, "8_Air_up_down_2", cb_btn_cptype);
	btn->_parent.span = 3;
	btn->_parent.userdata = (void*) 8;
	ui_wnd_add_child(window_cpsettings, btn);

	lbl = ui_lbl_make(0.0f, 0.0f, "Color:");
	ui_wnd_add_child(window_cpsettings, lbl);
	colpick = ui_colpick_make(0.0f, 0.0f, 100.0f, cb_cp_cpcol);
	colpick->_parent.span = 3;
	ui_wnd_add_child(window_cpsettings, colpick);

	btn = ui_btn_make(0.0f, 0.0f, "Reset", cb_btn_cpreset);
	btn->_parent.span = 4;
	ui_wnd_add_child(window_cpsettings, btn);
}

void wnd_dispose()
{
	ui_wnd_dispose(window_settings);
	ui_wnd_dispose(window_cpsettings);
	toggle_foliage(1);
}
