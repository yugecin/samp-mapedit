/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "entity.h"
#include "game.h"
#include "sockets.h"
#include "ui.h"
#include "vehicles.h"
#include "vehicleseditor.h"
#include "vehicleselector.h"
#include "vehiclesui.h"
#include "vehnames.h"
#include "../shared/serverlink.h"

static struct UI_BUTTON *btn_contextmenu_mkvehicle;
static struct UI_BUTTON *btn_contextmenu_editvehicle;
static struct UI_BUTTON *btn_mainmenu_vehicles;

static struct UI_WINDOW *wnd_vehlist;
static struct UI_CHECKBOX *chk_snap_camera;
static struct UI_LIST *lst_vehicles;
static struct UI_BUTTON *btn_edit;

static struct RwV3D posToCreate;
static char vehlistactive = 0;
static struct CEntity *highlightedVehicle;

static
void cb_vehsel_create(short model)
{
	vehedit_show(vehicles_create(model, &posToCreate), NULL);
}

static
void cb_btn_contextmenu_mkvehicle(struct UI_BUTTON *btn)
{
	ui_get_clicked_position(&posToCreate);
	posToCreate.z += 1.0f;
	vehsel_show(cb_vehsel_create);
}

static
void cb_btn_contextmenu_editvehicle(struct UI_BUTTON *btn)
{
	vehedit_show(vehicles_from_entity(clicked_entity), NULL);
}

static
void cb_btn_mainmenu_vehiclelist(struct UI_BUTTON *btn)
{
	ui_show_window(wnd_vehlist);
}

static
void cb_list_item_selected(struct UI_LIST *lst)
{
	btn_edit->enabled = lst->selectedindex != -1;
}

static
void cb_list_hover_update(struct UI_LIST *lst)
{
	struct RwV3D pos;
	int idx;

	idx = ui_lst_get_selected_index(lst);
	if (lst->hoveredindex != -1) {
		idx = lst->hoveredindex;
	}
	highlightedVehicle = NULL;
	if (idx == -1) {
		return;
	}
	highlightedVehicle = vehicles[idx].sa_vehicle;
	if (chk_snap_camera->checked) {
		game_ObjectGetPos(highlightedVehicle, &pos);
		center_camera_on(&pos);
	}
}

static
void cb_vehedit()
{
	ui_show_window(wnd_vehlist);
}

static
void cb_btn_edit(struct UI_BUTTON *btn)
{
	vehedit_show(vehicles + lst_vehicles->selectedindex, cb_vehedit);
}

static
void vehiclesui_update_list_data()
{
	char *names[MAX_VEHICLES];
	int i;

	for (i = 0; i < numvehicles; i++) {
		names[i] = vehnames[vehicles[i].model - 400];
	}
	ui_lst_set_data(lst_vehicles, names, i);
}

void vehiclesui_frame_update()
{
	if (vehlistactive && highlightedVehicle != NULL) {
		entity_draw_bound_rect(highlightedVehicle, 0xFF);
	}
}

void vehiclesui_on_active_window_changed(struct UI_WINDOW *wnd)
{
	if (wnd == wnd_vehlist) {
		vehlistactive = 1;
		// update list
		highlightedVehicle = NULL;
		btn_edit->enabled = 0;
		vehiclesui_update_list_data();
	} else {
		vehlistactive = 0;
	}
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

	/*vehicle list window*/
	wnd_vehlist = ui_wnd_make(9000.0f, 300.0f, "Vehicles");

	chk_snap_camera = ui_chk_make("Snap_camera", 0, NULL);
	ui_wnd_add_child(wnd_vehlist, chk_snap_camera);
	lst_vehicles = ui_lst_make(35, cb_list_item_selected);
	lst_vehicles->hovercb = cb_list_hover_update;
	ui_wnd_add_child(wnd_vehlist, lst_vehicles);
	btn_edit = ui_btn_make("Edit", cb_btn_edit);
	ui_wnd_add_child(wnd_vehlist, btn_edit);
}

void vehiclesui_dispose()
{
}

void vehiclesui_prj_postload()
{
	btn_mainmenu_vehicles->enabled = 1;
	btn_contextmenu_mkvehicle->enabled = 1;
}
