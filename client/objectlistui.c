/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ide.h"
#include "objbase.h"
#include "objects.h"
#include "objectlistui.h"
#include "msgbox.h"
#include "removebuildingeditor.h"
#include "removedbuildings.h"
#include "sockets.h"
#include "ui.h"
#include "../shared/serverlink.h"
#include <stdio.h>

static struct UI_WINDOW *wnd;
static struct UI_LIST *lst;
static struct UI_BUTTON *btn_edit;
static struct UI_BUTTON *btn_delete;

static char txt_currentlayer[100];

static
void cb_btn_mainmenu_objects(struct UI_BUTTON *btn)
{
	if (active_layer == NULL) {
		objects_show_select_layer_first_msg();
	} else {
		ui_show_window(wnd);
	}
}

static
void cb_btn_edit(struct UI_BUTTON *btn)
{
	struct OBJECT *obj;
	struct MSG_NC nc;

	if (active_layer != NULL &&
		0 <= lst->selectedindex &&
		lst->selectedindex < active_layer->numobjects)
	{
		obj = active_layer->objects + lst->selectedindex;
		nc._parent.id = MAPEDIT_MSG_NATIVECALL;
		nc._parent.data = 0;
		nc.nc = NC_EditObject;
		nc.params.asint[1] = 0;
		nc.params.asint[2] = obj->samp_objectid;
		sockets_send(&nc, sizeof(nc));
	}
}

static
void cb_msg_deleteconfirm(int opt)
{
	struct MSG_NC nc;
	int idx;

	idx = lst->selectedindex;
	if (opt != MSGBOX_RESULT_1 ||
		active_layer == NULL ||
		idx < 0 ||
		active_layer->numobjects <= idx)
	{
		goto ret;
	}

	nc._parent.id = MAPEDIT_MSG_NATIVECALL;
	nc._parent.data = 0;
	nc.nc = NC_DestroyObject;
	nc.params.asint[1] = active_layer->objects[idx].samp_objectid;
	sockets_send(&nc, sizeof(nc));

	active_layer->numobjects--;
	if (active_layer->numobjects > 0) {
		active_layer->objects[idx] =
			active_layer->objects[active_layer->numobjects];
	}
	objlistui_refresh_list();

ret:
	ui_show_window(wnd);
}

static
void cb_btn_delete(struct UI_BUTTON *btn)
{
	msg_title = "Object";
	msg_message = "Delete_object?";
	msg_message2 = "This_cannot_be_undone!";
	msg_btn1text = "Yes";
	msg_btn2text = "No";
	msg_show(cb_msg_deleteconfirm);
}

static
void cb_list_item_selected(struct UI_LIST *lst)
{
	btn_edit->enabled = btn_delete->enabled = lst->selectedindex != -1;
}

void objlistui_init()
{
	struct UI_BUTTON *btn;

	btn = ui_btn_make("Objects", cb_btn_mainmenu_objects);
	btn->_parent.span = 2;
	ui_wnd_add_child(main_menu, btn);

	wnd = ui_wnd_make(9000.0f, 300.0f, "Objects");

	strcpy(txt_currentlayer, "Current_layer:_");
	ui_wnd_add_child(wnd, ui_lbl_make(txt_currentlayer));
	lst = ui_lst_make(35, cb_list_item_selected);
	ui_wnd_add_child(wnd, lst);
	btn_edit = ui_btn_make("Edit", cb_btn_edit);
	ui_wnd_add_child(wnd, btn_edit);
	btn_delete = ui_btn_make("Delete", cb_btn_delete);
	ui_wnd_add_child(wnd, btn_delete);
}

void objlistui_dispose()
{
}

void objlistui_refresh_list()
{
	char *unknown = "unknown";
	char *names[MAX_REMOVES];
	int i;
	struct OBJECT *obj;

	btn_delete->enabled = 0;
	btn_edit->enabled = 0;

	if (active_layer == NULL) {
		strcpy(txt_currentlayer + 15, "<none>");
		ui_lst_set_data(lst, NULL, 0);
		return;
	}

	strcpy(txt_currentlayer + 15, active_layer->name);
	obj = active_layer->objects;
	for (i = 0; i < active_layer->numobjects; i++) {
		if (modelNames[obj->model]) {
			names[i] = modelNames[obj->model];
		} else {
			names[i] = unknown;
		}
		obj++;
	}
	ui_lst_set_data(lst, names, i);
}
