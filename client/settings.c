/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "client.h"
#include "game.h"
#include "samp.h"
#include "ui.h"
#include "vk.h"
#include <stdio.h>
#include <string.h>
#include <windows.h>

#define KEYS_ZQSD ((void*) 1)
#define KEYS_WASD ((void*) 0)

#define FOLIAGE_ON ((void*) 1)
#define FOLIAGE_OFF ((void*) 0)

#define MOVEMENT_DIR ((void*) 0)
#define MOVEMENT_NONDIR ((void*) 1)

static struct UI_WINDOW *window_settings;
static struct UI_BUTTON *btn_save_null_when_unchanged;
static struct RADIOBUTTONGROUP *rdbgroup_movement;
static struct RADIOBUTTONGROUP *rdbgroup_foliage = NULL;
static struct RADIOBUTTONGROUP *rdbgroup_keys;

static int saved_fontsize, saved_fontratio;
static int saved_foliage, saved_dirmov, saved_zqsd;

static unsigned char foliageCall[5] = { 0, 0, 0, 0, 0 };

static
void settings_remove_save_button()
{
	ui_wnd_remove_child(window_settings, btn_save_null_when_unchanged);
	ui_btn_dispose(btn_save_null_when_unchanged);
	btn_save_null_when_unchanged = NULL;
}

static
void cb_btn_save_settngs(struct UI_BUTTON *btn)
{
	FILE *ini;
	int len;
	char buf[80];

	settings_remove_save_button();
	if (rdbgroup_foliage != NULL) {
		/*it's not present when in samp*/
		saved_foliage =
			(int) ui_rdbgroup_selected_data(rdbgroup_foliage);
	}
	saved_zqsd = (int) ui_rdbgroup_selected_data(rdbgroup_keys);
	saved_dirmov = (int) ui_rdbgroup_selected_data(rdbgroup_movement);
	saved_fontsize = fontsize;
	saved_fontratio = fontratio;

	ini = fopen("samp-mapedit\\settings.txt", "w");
	if (ini != NULL) {
		len = sprintf(buf, "fontsize %d\n", fontsize);
		fwrite(buf, sizeof(char), len, ini);
		len = sprintf(buf, "fontratio %d\n", fontratio);
		fwrite(buf, sizeof(char), len, ini);
		len = sprintf(buf, "foliage %d\n", saved_foliage);
		fwrite(buf, sizeof(char), len, ini);
		len = sprintf(buf, "zqsd %d\n", saved_zqsd);
		fwrite(buf, sizeof(char), len, ini);
		len = sprintf(buf, "dirmov %d\n", saved_dirmov);
		fwrite(buf, sizeof(char), len, ini);
		fclose(ini);
	} else {
		sprintf(debugstring, "~r~failed to write settings");
		ui_push_debug_string();
	}
}

static
int settings_are_changed()
{
	return
		/*foliage is not available when in samp*/
		(rdbgroup_foliage != NULL && (saved_foliage ^
			(int) ui_rdbgroup_selected_data(rdbgroup_foliage))) ||
		(saved_zqsd ^
			(int) ui_rdbgroup_selected_data(rdbgroup_keys)) ||
		(saved_dirmov ^
			(int) ui_rdbgroup_selected_data(rdbgroup_movement)) ||
		saved_fontsize != fontsize ||
		saved_fontratio != fontratio;
}

static
void settings_changed()
{
	struct UI_BUTTON *btn;

	if (settings_are_changed()) {
		if (btn_save_null_when_unchanged == NULL) {
			btn = ui_btn_make("save_settings", cb_btn_save_settngs);
			btn->_parent.span = window_settings->columns;
			btn->_parent.userdata = btn->_parent.proc_update;
			ui_wnd_add_child(window_settings, btn);
			btn_save_null_when_unchanged = btn;
		}
	} else if (btn_save_null_when_unchanged != NULL) {
		settings_remove_save_button();
	}
}

static
void toggle_foliage(int should_be_enabled)
{
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

static
void cb_btn_reload_settings(struct UI_BUTTON *btn)
{
	FILE *ini;
	int pos, i;
	char buf[80];

	saved_fontsize = UI_DEFAULT_FONT_SIZE;
	saved_fontratio = UI_DEFAULT_FONT_RATIO;
	saved_foliage = (int) FOLIAGE_ON;
	saved_dirmov = (int) MOVEMENT_DIR;
	saved_zqsd = (int) KEYS_ZQSD;

	ini = fopen("samp-mapedit\\settings.txt", "r");
	if (ini != NULL) {
		pos = 0;
nextline:
		memset(buf, 0, sizeof(buf));
		fseek(ini, pos, SEEK_SET);
		if (fread(buf, 1, sizeof(buf) - 1, ini) == 0) {
			goto done;
		}
		i = 0;
		if (strncmp("fontsize ", buf, 9) == 0) {
			saved_fontsize = atoi(buf + (i = 9));
		} else if (strncmp("fontratio ", buf, 10) == 0) {
			saved_fontratio = atoi(buf + (i = 10));
		} else if (strncmp("foliage ", buf, 8) == 0) {
			saved_foliage = atoi(buf + (i = 8));
		} else if (strncmp("dirmov ", buf, 7) == 0) {
			saved_dirmov = atoi(buf + (i = 7));
		} else if (strncmp("zqsd ", buf, 5) == 0) {
			saved_zqsd = atoi(buf + (i = 5));
		} else {
			i = 0;
		}
		while (buf[i] != 0 && buf[i] != '\n') {
			i++;
		}
		pos += i + 2;
		goto nextline;
done:
		fclose(ini);
	} else {
		sprintf(debugstring, "~r~failed to read settings");
		ui_push_debug_string();
	}

	if (rdbgroup_foliage != NULL) {
		/*it's not present when in samp*/
		ui_rdb_click_match_userdata(rdbgroup_foliage,
			(void*) saved_foliage);
	}
	ui_rdb_click_match_userdata(rdbgroup_keys, (void*) saved_zqsd);
	ui_rdb_click_match_userdata(rdbgroup_movement, (void*) saved_dirmov);
	ui_set_fontsize(saved_fontsize, saved_fontratio);
	settings_changed();
}

void settings_init()
{
	struct UI_BUTTON *btn;
	struct UI_RADIOBUTTON *rdb;
	struct UI_LABEL *lbl;

	btn = ui_btn_make("Settings", cb_btn_settings);
	btn->_parent.span = 2;
	ui_wnd_add_child(main_menu, btn);

	window_settings = ui_wnd_make(500.0f, 500.0f, "Settings");
	window_settings->columns = 4;

	if (samp_handle) {
		lbl = ui_lbl_make("Foliage_is_always_off_in_samp");
		lbl->_parent.span = 4;
		ui_wnd_add_child(window_settings, lbl);
	} else {
		lbl = ui_lbl_make("Foliage:");
		ui_wnd_add_child(window_settings, lbl);
		rdbgroup_foliage = ui_rdbgroup_make(cb_rdb_foliage);
		rdb = ui_rdb_make("on", rdbgroup_foliage, 1);
		rdb->_parent._parent.userdata = FOLIAGE_ON;
		rdb->_parent._parent.span = 2;
		ui_wnd_add_child(window_settings, rdb);
		rdb = ui_rdb_make("off", rdbgroup_foliage, 0);
		rdb->_parent._parent.userdata = FOLIAGE_OFF;
		ui_wnd_add_child(window_settings, rdb);
	}

	lbl = ui_lbl_make("Keys:");
	ui_wnd_add_child(window_settings, lbl);
	rdbgroup_keys = ui_rdbgroup_make(cb_rdb_keys);
	rdb = ui_rdb_make("zqsd", rdbgroup_keys, 1);
	rdb->_parent._parent.userdata = KEYS_ZQSD;
	rdb->_parent._parent.span = 2;
	ui_wnd_add_child(window_settings, rdb);
	rdb = ui_rdb_make("wasd", rdbgroup_keys, 0);
	rdb->_parent._parent.userdata = KEYS_WASD;
	ui_wnd_add_child(window_settings, rdb);

	lbl = ui_lbl_make("Movement:");
	ui_wnd_add_child(window_settings, lbl);
	rdbgroup_movement = ui_rdbgroup_make(cb_rdb_movement);
	rdb = ui_rdb_make("directional", rdbgroup_movement, 1);
	rdb->_parent._parent.userdata = MOVEMENT_DIR;
	rdb->_parent._parent.span = 2;
	ui_wnd_add_child(window_settings, rdb);
	rdb = ui_rdb_make("flat", rdbgroup_movement, 0);
	rdb->_parent._parent.userdata = MOVEMENT_NONDIR;
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

	ui_wnd_add_child(window_settings, NULL);
	btn = ui_btn_make("reload_saved_settings", cb_btn_reload_settings);
	btn->_parent.span = 3;
	ui_wnd_add_child(window_settings, btn);

	btn_save_null_when_unchanged = NULL;
	cb_btn_reload_settings(NULL); /*loads saved settings*/
}

void settings_dispose()
{
	TRACE("settings_dispose");
	ui_wnd_dispose(window_settings);
	if (!samp_handle) {
		toggle_foliage(1);
	}
}
