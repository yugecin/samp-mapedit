/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ui.h"
#include "vk.h"
#include <string.h>

static struct UI_WINDOW *window_settings;
static char is_changed;

static
void btn_save_update(struct UI_BUTTON *btn)
{
	float angle;
	int col;

	angle = (*timeInGame % 2500) / 2500.0f;
	col = 0xFF000000;
	col |= hue(angle, HUE_COMP_R);
	col |= hue(angle, HUE_COMP_G) << 8;
	col |= hue(angle, HUE_COMP_B) << 16;
	btn->foregroundABGR = col;
	((ui_method*) btn->_parent.userdata)(btn);
}

static
void cb_btn_save(struct UI_BUTTON *btn)
{
	ui_wnd_remove_child(window_settings, btn);
	ui_btn_dispose(btn);
	is_changed = 0;
}

static
void settings_changed()
{
	struct UI_BUTTON *btn;

	if (!is_changed) {  
		is_changed = 1;
		btn = ui_btn_make("save_settings", cb_btn_save);
		btn->foregroundABGR = 0xFFFF0000;
		btn->_parent.span = window_settings->columns;
		btn->_parent.userdata = btn->_parent.proc_update;
		btn->_parent.proc_update = (ui_method*) btn_save_update;
		ui_wnd_add_child(window_settings, btn);
	}
}

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
	settings_changed();
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
	settings_changed();
}

static
void cb_rdb_movement(struct UI_RADIOBUTTON *rdb)
{
	directional_movement = (int) rdb->_parent._parent.userdata;
	settings_changed();
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
	settings_changed();
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
	settings_changed();
}

static
void cb_btn_settings(struct UI_BUTTON *btn)
{
	ui_show_window(window_settings);
}

void settings_init()
{
	struct UI_BUTTON *btn;
	struct UI_RADIOBUTTON *rdb;
	struct RADIOBUTTONGROUP *rdbgroup;
	struct UI_LABEL *lbl;

	is_changed = 0;

	btn = ui_btn_make("Settings", cb_btn_settings);
	btn->_parent.x = 10.0f;
	btn->_parent.y = 550.0f;
	ui_cnt_add_child(background_element, btn);
	
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
}

void settings_dispose()
{
	ui_wnd_dispose(window_settings);
	toggle_foliage(1);
}
