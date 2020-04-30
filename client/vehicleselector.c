/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ui.h"
#include "vehicles.h"
#include "vehicleselector.h"
#include "vehiclesui.h"
#include "vehnames.h"
#include "../shared/serverlink.h"

static struct UI_WINDOW *wnd;
static struct UI_LIST *lst_vehicles;
static struct UI_BUTTON *btn_create;

static
void cb_in_filter_updated(struct UI_INPUT *in)
{
	ui_lst_recalculate_filter(lst_vehicles);
}

static
void cb_lst_vehicles_selected(struct UI_LIST *lst)
{
	btn_create->enabled = lst->selectedindex != -1;
}

static
void cb_btn_create(struct UI_BUTTON *btn)
{
	int idx;

	idx = ui_lst_get_selected_index(lst_vehicles);
	vehiclesui_create(400 + idx);
}

void vehsel_show()
{
	ui_show_window(wnd);
}

void vehsel_init()
{
	struct UI_INPUT *filter;

	wnd = ui_wnd_make(fresx / 2.0f, 200.0f, "Vehicle_selector");

	filter = ui_in_make(cb_in_filter_updated);
	ui_wnd_add_child(wnd, filter);
	lst_vehicles = ui_lst_make(35, cb_lst_vehicles_selected);
	ui_wnd_add_child(wnd, lst_vehicles);
	lst_vehicles->filter = filter->value;
	ui_lst_set_data(lst_vehicles, vehnames, VEHICLE_MODEL_TOTAL);
	btn_create = ui_btn_make("Create", cb_btn_create);
	ui_wnd_add_child(wnd, btn_create);
}

void vehsel_dispose()
{
	ui_wnd_dispose(wnd);
}