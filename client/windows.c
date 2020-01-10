/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "ui.h"
#include "windows.h"
#include <string.h>

struct UI_WINDOW *window_settings;

static unsigned char foliageCall[5] = { 0, 0, 0, 0, 0 };

static
void toggle_foliage()
{
	void* position;

	if (*((unsigned char*) 0x53C159) == 0x90) {
		memcpy((void*) 0x53C159, foliageCall, 5);
		foliageCall[0] = 0;
	} else {
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
void cb_btn_foliage(struct UI_BUTTON *btn)
{
	toggle_foliage();
}

void wnd_init()
{
	struct UI_BUTTON *btn;

	window_settings = ui_wnd_make(500.0f, 500.0f, "Settings");
	window_settings->columns = 2;
	btn = ui_btn_make(0.0f, 0.0f, "Foliage", cb_btn_foliage);
	ui_wnd_add_child(window_settings, (struct UI_ELEMENT*) btn);
	btn = ui_btn_make(0.0f, 0.0f, "Foliage2", cb_btn_foliage);
	ui_wnd_add_child(window_settings, (struct UI_ELEMENT*) btn);
	btn = ui_btn_make(0.0f, 0.0f, "Foliage3", cb_btn_foliage);
	btn->_parent.span = 2;
	ui_wnd_add_child(window_settings, (struct UI_ELEMENT*) btn);
	btn = ui_btn_make(0.0f, 0.0f, "Foliage4", cb_btn_foliage);
	ui_wnd_add_child(window_settings, (struct UI_ELEMENT*) btn);
}

void wnd_dispose()
{
	ui_wnd_dispose(window_settings);
	if (foliageCall[0]) {
		toggle_foliage();
	}
}
