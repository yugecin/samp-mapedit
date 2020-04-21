/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "objbase.h"
#include "objects.h"
#include "msgbox.h"
#include "removebuildingeditor.h"
#include "removedbuildings.h"
#include "removedbuildingsui.h"
#include "ui.h"
#include <stdio.h>

static struct UI_WINDOW *wnd;
static struct UI_LIST *lst;
static struct UI_BUTTON *btn_edit;
static struct UI_BUTTON *btn_delete;

static char txt_currentlayer[100];

static
void cb_list_item_selected(struct UI_LIST *lst)
{
	btn_delete->enabled = btn_edit->enabled = lst->selectedindex != -1;
}

static
void cb_rbe_save(struct REMOVEDBUILDING *remove)
{
	if (remove != NULL &&
		active_layer != NULL &&
		0 <= lst->selectedindex &&
		lst->selectedindex < active_layer->numremoves)
	{
		active_layer->removes[lst->selectedindex] = *remove;
		rbui_refresh_list();
	}
}

static
void cb_btn_edit(struct UI_BUTTON *btn)
{
	int idx;

	idx = lst->selectedindex;
	if (active_layer != NULL &&
		0 <= idx &&
		idx < active_layer->numremoves)
	{
		rbe_show_for_remove(&active_layer->removes[idx], cb_rbe_save);
	}
}

static
void cb_msg_deleteconfirm(int opt)
{
	int idx;

	idx = lst->selectedindex;
	if (opt != MSGBOX_RESULT_1 ||
		active_layer == NULL ||
		idx < 0 ||
		active_layer->numremoves <= idx)
	{
		return;
	}

	rb_undo_all();
	active_layer->numremoves--;
	if (active_layer->removes[idx].description != NULL) {
		free(active_layer->removes[idx].description);
	}
	if (active_layer->numremoves > 0) {
		active_layer->removes[idx] =
			active_layer->removes[active_layer->numremoves];
	}
	rbui_refresh_list();
	rb_do_all();
}

static
void cb_btn_delete(struct UI_BUTTON *btn)
{
	msg_title = "Remove";
	msg_message = "Delete_remove?";
	msg_message2 = "This_cannot_be_undone!";
	msg_btn1text = "Yes";
	msg_btn2text = "No";
	msg_show(cb_msg_deleteconfirm);
}

static
void cb_btn_close(struct UI_BUTTON *btn)
{
	ui_hide_window();
}

static
void cb_btn_mainmenu_removes(struct UI_BUTTON *btn)
{
	if (active_layer == NULL) {
		objects_show_select_layer_first_msg();
	} else {
		ui_show_window(wnd);
	}
}

static
void cb_btn_mainmenu_refresh_removes(struct UI_BUTTON *btn)
{
	rb_undo_all();
	rb_do_all();
}

void rbui_init()
{
	static struct UI_BUTTON *btn;

	btn = ui_btn_make("Removes", cb_btn_mainmenu_removes);
	btn->_parent.span = 2;
	ui_wnd_add_child(main_menu, btn);
	/*because it's not working properly yet*/
	btn = ui_btn_make("Refresh_removes", cb_btn_mainmenu_refresh_removes);
	btn->_parent.span = 2;
	ui_wnd_add_child(main_menu, btn);

	wnd = ui_wnd_make(9000.0f, 300.0f, "Removes");

	strcpy(txt_currentlayer, "Current_layer:_");
	ui_wnd_add_child(wnd, ui_lbl_make(txt_currentlayer));
	lst = ui_lst_make(15, cb_list_item_selected);
	lst->_parent.pref_width = 700.0f;
	ui_wnd_add_child(wnd, lst);
	btn_edit = ui_btn_make("Edit_selected", cb_btn_edit);
	ui_wnd_add_child(wnd, btn_edit);
	btn_delete = ui_btn_make("Delete_selected", cb_btn_delete);
	ui_wnd_add_child(wnd, btn_delete);
	btn = ui_btn_make("Close", cb_btn_close);
	ui_wnd_add_child(wnd, btn);
}

void rbui_dispose()
{
	ui_wnd_dispose(wnd);
}

void rbui_refresh_list()
{
	char unnamedpool[MAX_REMOVES * 20];
	char *n = unnamedpool;
	char *names[MAX_REMOVES];
	int i;
	struct REMOVEDBUILDING *remove;

	btn_delete->enabled = 0;
	btn_edit->enabled = 0;

	if (active_layer == NULL) {
		strcpy(txt_currentlayer + 15, "<none>");
		ui_lst_set_data(lst, NULL, 0);
		return;
	}

	strcpy(txt_currentlayer + 15, active_layer->name);
	remove = active_layer->removes;
	for (i = 0; i < active_layer->numremoves; i++) {
		if (remove->description != NULL) {
			names[i] = remove->description;
		} else if (n - unnamedpool < sizeof(unnamedpool) - 20) {
			names[i] = n;
			n += 1 + sprintf(n, "mod %hd rad %.0f",
				remove->model,
				(float) sqrt(remove->radiussq));
		} else {
			break;
		}
		remove++;
	}
	ui_lst_set_data(lst, names, i);
}
