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
static struct UI_BUTTON *btn_delete;
static ui_method *proc_draw_window_cplist;

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
	btn_edit->enabled = btn_delete->enabled = lst->selectedindex != -1;
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
void cb_msg_confirm_delete(int choice)
{
	int idx, i;

	if (choice == MSGBOX_RESULT_1) {
		idx = ui_lst_get_selected_index(lst_checkpoints);
		for (i = idx; i < numcheckpoints - 1; i++) {
			memcpy(checkpointDescriptions + i,
				checkpointDescriptions + i + 1,
				sizeof(checkpointDescriptions[0]));
			racecheckpoints[i] = racecheckpoints[i + 1];
		}
		for (i = idx; i < numcheckpoints; i++) {
			racecheckpoints[i].free = 1;
			racecheckpoints[i].used = 0;
		}
		numcheckpoints--;
	}
	ui_show_window(wnd_cplist);
}

static
void cb_btn_delete(struct UI_BUTTON *btn)
{
	msg_title = "Checkpoints";
	msg_message = "Delete_checkpoint?";
	msg_message2 = "This_cannot_be_undone!";
	msg_btn1text = "Yes";
	msg_btn2text = "No";
	msg_show(cb_msg_confirm_delete);
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

static
int draw_window_cplist(struct UI_ELEMENT *wnd)
{
	struct RwV3D out;
	char *noname = "<noname>";
	char *text;
	int i;

	game_TextSetAlign(CENTER);
	for (i = 0; i < numcheckpoints; i++) {
		game_WorldToScreen(&out, &racecheckpoints[i].pos);
		if (out.z > 0.0f) {
			text = checkpointDescriptions[i];
			if (!text[0]) {
				text = noname;
			}
			game_TextPrintString(out.x, out.y, text);
		}
	}
	return proc_draw_window_cplist(wnd);
}

void racecpui_show_window()
{
	ui_show_window(wnd_cplist);
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
	proc_draw_window_cplist = wnd_cplist->_parent._parent.proc_draw;
	wnd_cplist->_parent._parent.proc_draw = draw_window_cplist;

	chk_snap_camera = ui_chk_make("Snap_camera", 0, NULL);
	ui_wnd_add_child(wnd_cplist, chk_snap_camera);
	lst_checkpoints = ui_lst_make(35, cb_list_item_selected);
	lst_checkpoints->hovercb = cb_list_hover_update;
	ui_wnd_add_child(wnd_cplist, lst_checkpoints);
	btn_edit = ui_btn_make("Edit", cb_btn_edit);
	ui_wnd_add_child(wnd_cplist, btn_edit);
	btn_delete = ui_btn_make("Delete", cb_btn_delete);
	ui_wnd_add_child(wnd_cplist, btn_delete);
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
