/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "ui.h"
#include "windows.h"
#include <string.h>

struct UI_WINDOW *window_settings;

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

void wnd_init()
{
	struct RADIOBUTTONGROUP *rdbgroup;
	struct UI_RADIOBUTTON *rdb;
	struct UI_LABEL *lbl;
	struct UI_BUTTON *btn;

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
	btn = ui_btn_make(0.0f, 0.0f, "Hey", (btncb*) ui_elem_dummy_proc);
	btn->_parent.span = 3;
	ui_wnd_add_child(window_settings, btn);
}

void wnd_dispose()
{
	ui_wnd_dispose(window_settings);
	toggle_foliage(1);
}
