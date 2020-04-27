/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "sockets.h"
#include "ui.h"
#include "vehicles.h"
#include "vehiclesui.h"
#include "../shared/serverlink.h"

static struct UI_BUTTON *btn_contextmenu_mkvehicle;
static struct UI_BUTTON *btn_mainmenu_vehicles;

static
void cb_btn_contextmenu_mkvehicle(struct UI_BUTTON *btn)
{
	struct RwV3D pos;

	if (clicked_entity) {
		pos = clicked_colpoint.pos;
		pos.z += 1.0f;
	} else {
		game_ScreenToWorld(&pos, fresx / 2.0f, fresy / 2.0f, 40.0f);
	}

	vehicles_create(411, &pos);
}

static
void cb_btn_mainmenu_vehiclelist(struct UI_BUTTON *btn)
{
}

void vehiclesui_init()
{
	struct UI_LABEL *lbl;
	struct UI_BUTTON *btn;

	/*context menu entry*/
	btn = ui_btn_make("Make_vehicle", cb_btn_contextmenu_mkvehicle);
	ui_wnd_add_child(context_menu, btn);
	btn->enabled = 0;
	btn_contextmenu_mkvehicle = btn;

	/*main menu entry*/
	lbl = ui_lbl_make("=_Vehicles_=");
	lbl->_parent.span = 2;
	ui_wnd_add_child(main_menu, lbl);
	btn = ui_btn_make("Vehicles", cb_btn_mainmenu_vehiclelist);
	btn->_parent.span = 2;
	ui_wnd_add_child(main_menu, btn);
	btn->enabled = 0;
	btn_mainmenu_vehicles = btn;
}

void vehiclesui_dispose()
{
}

void vehiclesui_prj_postload()
{
	btn_mainmenu_vehicles->enabled = 1;
	btn_contextmenu_mkvehicle->enabled = 1;
}
