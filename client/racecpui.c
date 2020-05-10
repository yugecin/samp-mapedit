/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ui.h"
#include "msgbox.h"
#include "racecp.h"
#include "racecpeditor.h"
#include "racecpui.h"

static struct UI_BUTTON *btn_mainmenu_cplist;
static struct UI_BUTTON *btn_contextmenu_mkracecp;
static struct UI_WINDOW *wnd_cplist;
static struct UI_LIST *lst_checkpoints;
static struct UI_CHECKBOX *chk_snap_camera;
static struct UI_BUTTON *btn_edit;

static
void cb_btn_mkracecp(struct UI_BUTTON *btn)
{
	struct RwV3D posToCreate;

	if (numcheckpoints == MAX_RACECHECKPOINTS) {
		msg_title = "Race_cp";
		msg_message = "Max_race_checkpoints_reached";
		msg_btn1text = "Ok";
		msg_show(NULL);
		return;
	}

	ui_get_clicked_position(&posToCreate);
	checkpointDescriptions[numcheckpoints][0] = 0;
	racecheckpoints[numcheckpoints].used = 0;
	racecheckpoints[numcheckpoints].free = 1;
	racecheckpoints[numcheckpoints].type = 0;
	racecheckpoints[numcheckpoints].pos = posToCreate;
	racecheckpoints[numcheckpoints].fRadius = 2.0f;
	racecheckpoints[numcheckpoints].arrowDirection.x = 1.0f;
	racecheckpoints[numcheckpoints].arrowDirection.y = 0.0f;
	racecheckpoints[numcheckpoints].arrowDirection.z = 0.0f;
	racecheckpoints[numcheckpoints].colABGR = 0xFF0000FF;
	racecpeditor_edit_checkpoint(numcheckpoints);
	numcheckpoints++;
}

static
void cb_btn_cplist(struct UI_BUTTON *btn)
{
	ui_show_window(wnd_cplist);
}

static
void cb_list_item_selected(struct UI_LIST *lst)
{
	btn_edit->enabled = lst->selectedindex != -1;
}

static
void cb_list_hover_update(struct UI_LIST *lst)
{
	int idx;

	idx = ui_lst_get_selected_index(lst);
	if (lst->hoveredindex != -1) {
		idx = lst->hoveredindex;
	}
	if (idx != -1 && chk_snap_camera->checked) {
		center_camera_on(&racecheckpoints[idx].pos);
	}
}

static
void cb_btn_edit(struct UI_BUTTON *btn)
{
	int idx;

	idx = ui_lst_get_selected_index(lst_checkpoints);
	if (idx != -1) {
		racecpeditor_edit_checkpoint(idx);
	}
}

static
void racecpui_update_list_data()
{
	char *unnamed = "<noname>";
	char *names[MAX_RACECHECKPOINTS];
	int i;

	for (i = 0; i < numcheckpoints; i++) {
		if (checkpointDescriptions[i][0]) {
			names[i] = checkpointDescriptions[i];
		} else {
			names[i] = unnamed;
		}
	}
	ui_lst_set_data(lst_checkpoints, names, i);
}

void racecpui_on_active_window_changed(struct UI_WINDOW *wnd)
{
	if (wnd == wnd_cplist) {
		racecpui_update_list_data();
	}
}

void racecpui_init()
{
	struct UI_BUTTON *btn;
	struct UI_LABEL *lbl;

	/*context menu entry*/
	btn = ui_btn_make("Make_race_CP", cb_btn_mkracecp);
	ui_wnd_add_child(context_menu, btn);
	btn->enabled = 0;
	btn_contextmenu_mkracecp = btn;

	/*main menu entry*/
	lbl = ui_lbl_make("=_Race_CPs_=");
	lbl->_parent.span = 2;
	ui_wnd_add_child(main_menu, lbl);
	btn = ui_btn_make("Checkpoints", cb_btn_cplist);
	btn->_parent.span = 2;
	ui_wnd_add_child(main_menu, btn);
	btn->enabled = 0;
	btn_mainmenu_cplist = btn;

	/*vehicle list window*/
	wnd_cplist = ui_wnd_make(9000.0f, 300.0f, "Checkpoints");

	chk_snap_camera = ui_chk_make("Snap_camera", 0, NULL);
	ui_wnd_add_child(wnd_cplist, chk_snap_camera);
	lst_checkpoints = ui_lst_make(35, cb_list_item_selected);
	lst_checkpoints->hovercb = cb_list_hover_update;
	ui_wnd_add_child(wnd_cplist, lst_checkpoints);
	btn_edit = ui_btn_make("Edit", cb_btn_edit);
	ui_wnd_add_child(wnd_cplist, btn_edit);
}

void racecpui_dispose()
{
	ui_wnd_dispose(wnd_cplist);
}

void racecpui_prj_postload()
{
	btn_mainmenu_cplist->enabled = 1;
	btn_contextmenu_mkracecp->enabled = 1;
}
