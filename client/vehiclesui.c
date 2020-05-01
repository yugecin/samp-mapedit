/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "sockets.h"
#include "ui.h"
#include "vehicles.h"
#include "vehicleseditor.h"
#include "vehicleselector.h"
#include "vehiclesui.h"
#include "../shared/serverlink.h"

static struct UI_BUTTON *btn_contextmenu_mkvehicle;
static struct UI_BUTTON *btn_contextmenu_editvehicle;
static struct UI_BUTTON *btn_mainmenu_vehicles;

static struct RwV3D posToCreate;

static
void cb_vehsel_create(short model)
{
	vehedit_show(vehicles_create(model, &posToCreate));
}

static
void cb_btn_contextmenu_mkvehicle(struct UI_BUTTON *btn)
{
	float x, y;

	if (clicked_entity) {
		posToCreate = clicked_colpoint.pos;
		posToCreate.z += 1.0f;
	} else {
		x = fresx / 2.0f;
		y = fresy / 2.0f;
		game_ScreenToWorld(&posToCreate, x, y, 40.0f);
	}

	vehsel_show(cb_vehsel_create);
}

static
void cb_btn_contextmenu_editvehicle(struct UI_BUTTON *btn)
{
	vehedit_show(vehicles_from_entity(clicked_entity));
}

static
void cb_btn_mainmenu_vehiclelist(struct UI_BUTTON *btn)
{
}

int vehiclesui_on_background_element_just_clicked()
{
	btn_contextmenu_editvehicle->enabled =
		clicked_entity != NULL &&
		vehicles_from_entity(clicked_entity) != NULL;

	return 1;
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
	btn = ui_btn_make("Edit_vehicle", cb_btn_contextmenu_editvehicle);
	ui_wnd_add_child(context_menu, btn);
	btn->enabled = 0;
	btn_contextmenu_editvehicle = btn;

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
