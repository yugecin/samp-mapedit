/* vim: set filetype=c ts=8 noexpandtab: */

#include "bulkedit.h"
#include "bulkeditui.h"
#include "common.h"
#include "game.h"
#include "entity.h"
#include "ide.h"
#include "objects.h"
#include "objectseditor.h"
#include "ui.h"
#include "msgbox.h"
#include <string.h>

struct UI_WINDOW *bulkeditui_wnd;
char bulkeditui_shown;

static struct UI_CHECKBOX *chk_plant_objects;
static struct UI_CHECKBOX *chk_select_from_active_layer;
static struct UI_CHECKBOX *chk_captureclicks;
static struct UI_CHECKBOX *chk_dragselect;
static ui_method *bulkeditui_original_proc_wnd_draw;
static struct OBJECT *plantObj;

static char txt_bulk_selected_objects[70];
static struct UI_LABEL *lbl_bulk;
static struct UI_LABEL *lbl_bulk_filter;
static struct UI_INPUT *in_bulk_filter;
static struct UI_LIST *lst_bulk_filter;
static struct UI_BUTTON *btn_bulk_addall;
static struct UI_BUTTON *btn_layer[MAX_LAYERS];

static char is_dragselecting;
static float dragselect_start_x, dragselect_start_y;

#define MAX_LABELS 5
struct FLOATING_LABEL {
	float x, y;
	char txt[80];
	int timeout;
};

static struct FLOATING_LABEL labels[MAX_LABELS];
static int label_next_idx;

struct ON_SCREEN_OBJECT {
	float x, y;
	struct OBJECT *obj;
	int selected;
	int passes_filter;
};

static struct ON_SCREEN_OBJECT on_screen_objects[MAX_OBJECTS * 10];
static int num_on_screen_objects;

static
void cb_posrdbgroup_changed(struct UI_RADIOBUTTON *rdb)
{
	bulkedit_pos_update_method = rdb->_parent._parent.userdata;
	bulkedit_update();
}

static
void cb_rotrdbgroup_changed(struct UI_RADIOBUTTON *rdb)
{
	bulkedit_rot_update_method = rdb->_parent._parent.userdata;
	bulkedit_update();
}

static
void cb_directionrdbgroup_changed(struct UI_RADIOBUTTON *rdb)
{
	bulkedit_direction_add_90 = rdb->_parent._parent.userdata == (void*) 1;
	bulkedit_direction_remove_90 = rdb->_parent._parent.userdata == (void*) 2;
	bulkedit_direction_add_180 = rdb->_parent._parent.userdata == (void*) 3;
	bulkedit_update();
}

static
void cb_btn_clone_all(struct UI_BUTTON *btn)
{
	bulkedit_clone_all();
}

static
void cb_msg_delete_confirm(int opt)
{
	if (opt == MSGBOX_RESULT_1) {
		bulkedit_delete_objects();
	}
}

static
void cb_btn_delete_all(struct UI_BUTTON *btn)
{
	msg_title = "Bulk_delete";
	msg_message = "Delete_objects?";
	msg_message2 = "This_cannot_be_undone!";
	msg_btn1text = "Yes";
	msg_btn2text = "No";
	msg_show(cb_msg_delete_confirm);
}

static
void cb_btn_commit(struct UI_BUTTON *btn)
{
	bulkedit_commit();
}

static
void cb_btn_disband(struct UI_BUTTON *btn)
{
	bulkedit_revert();
	bulkedit_reset();
}

static
void cb_btn_revert(struct UI_BUTTON *btn)
{
	bulkedit_revert();
}

static
void cb_btn_mainmenu_bulkedit(struct UI_BUTTON *btn)
{
	struct OBJECT *obj;

	bulkeditui_shown = 1;
	bulkeditui_wnd->_parent._parent.proc_recalc_size(bulkeditui_wnd);
	obj = objedit_get_editing_object();
	if (obj) {
		bulkedit_begin(obj);
	}
}

static
void bulkeditui_update_list_data()
{
	int i;
	char *lst_items[1000];
	int num_lst_items;
	char *name;

	num_lst_items = 0;
	for (i = 0; i < num_on_screen_objects; i++) {
		if (on_screen_objects[i].selected) {
			name = modelNames[on_screen_objects[i].obj->model];
			if (!in_bulk_filter->valuelen || stristr(name, in_bulk_filter->value) != NULL) {
				on_screen_objects[i].passes_filter = 1;
				lst_items[num_lst_items++] = name;
			} else {
				on_screen_objects[i].passes_filter = 0;
			}
		}
	}
	sprintf(txt_bulk_selected_objects, "Selected_objects_(%d/%d):", num_lst_items, num_on_screen_objects);
	ui_lst_set_data(lst_bulk_filter, lst_items, num_lst_items);
}

void cb_chk_dragselect(struct UI_CHECKBOX *chk);

static
void cb_chk_captureclicks(struct UI_CHECKBOX *chk)
{
	if (chk->checked && chk_dragselect->checked) {
		chk_dragselect->checked = 0;
		ui_chk_updatetext(chk_dragselect);
		cb_chk_dragselect(chk_dragselect);
	}
}

static
void cb_chk_dragselect(struct UI_CHECKBOX *chk)
{
	if (chk->checked) {
		if (chk_captureclicks->checked) {
			chk_captureclicks->checked = 0;
			ui_chk_updatetext(chk_captureclicks);
			cb_chk_captureclicks(chk_captureclicks);
		}
		ui_wnd_add_child(bulkeditui_wnd, lbl_bulk);
		ui_wnd_add_child(bulkeditui_wnd, lbl_bulk_filter);
		ui_wnd_add_child(bulkeditui_wnd, in_bulk_filter);
		ui_wnd_add_child(bulkeditui_wnd, lst_bulk_filter);
		ui_wnd_add_child(bulkeditui_wnd, btn_bulk_addall);
	} else {
		num_on_screen_objects = 0;
		bulkeditui_update_list_data();
		ui_wnd_remove_child(bulkeditui_wnd, lbl_bulk);
		ui_wnd_remove_child(bulkeditui_wnd, lbl_bulk_filter);
		ui_wnd_remove_child(bulkeditui_wnd, in_bulk_filter);
		ui_wnd_remove_child(bulkeditui_wnd, lst_bulk_filter);
		ui_wnd_remove_child(bulkeditui_wnd, btn_bulk_addall);
	}
}

static
void cb_in_bulkfilter(struct UI_INPUT *in)
{
	bulkeditui_update_list_data();
}

static
void cb_btn_bulkaddall(struct UI_BUTTON *btn)
{
	int i;

	for (i = 0; i < num_on_screen_objects; i++) {
		if (on_screen_objects[i].passes_filter) {
			bulkedit_add(on_screen_objects[i].obj);
		}
	}

	num_on_screen_objects = 0;
	bulkeditui_update_list_data();
}

static
void cb_btn_move_all_to_layer(struct UI_BUTTON *btn)
{
	int i;

	if (!btn_layer[0]->_parent.parent) {
		for (i = 0; i < numlayers; i++) {
			btn_layer[i]->text = layers[i].name;
			ui_wnd_add_child(bulkeditui_wnd, btn_layer[i]);
		}
	} else {
		for (i = 0; i < MAX_LAYERS; i++) {
			if (btn_layer[i]->_parent.parent) {
				ui_wnd_remove_child(bulkeditui_wnd, btn_layer[i]);
			}
		}
	}
}

static
void cb_btn_movelayer(struct UI_BUTTON *btn)
{
	int i;

	ui_hide_window();
	bulkedit_move_layer(layers + (int) btn->_parent.userdata);
	for (i = 0; i < MAX_LAYERS; i++) {
		if (btn_layer[i]->_parent.parent) {
			ui_wnd_remove_child(bulkeditui_wnd, btn_layer[i]);
		}
	}
}

int bulkeditui_proc_close(struct UI_WINDOW *wnd)
{
	is_dragselecting = 0;
	bulkeditui_shown = 0;
	return 1;
}

int bulkeditui_on_background_element_just_clicked(struct OBJECTLAYER *real_active_layer)
{
	struct OBJECT *obj;
	struct FLOATING_LABEL *lbl;

	if (is_dragselecting && bulkeditui_shown) {
		is_dragselecting = 0;
		if (real_active_layer != active_layer) {
			objects_activate_layer(real_active_layer - layers);
		}
		return 0;
	}

	if (bulkeditui_shown) {
		obj = objects_find_by_sa_object(clicked_entity);
		if (real_active_layer != active_layer) {
			objects_activate_layer(real_active_layer - layers);
		}
		if (chk_plant_objects->checked) {
			if (active_layer->numobjects < 950) {
				plantObj = active_layer->objects + active_layer->numobjects++;
				if (clicked_entity) {
					plantObj->pos = clicked_colpoint.pos;
				} else {
					game_ScreenToWorld(&plantObj->pos, cursorx, cursory, 30.0f);
				}
				plantObj->rot.x = plantObj->rot.y = plantObj->rot.z = 0.0f;
				plantObj->model = lastMadeObjectModel;
				plantObj->num_materials = 0;
				objects_mkobject(plantObj);
				return 0;
			}
		}
		if (obj != NULL && chk_captureclicks->checked) {
			if (!chk_select_from_active_layer->checked ||
				objects_layer_for_object(obj) == active_layer)
			{
				lbl = labels + label_next_idx;
				label_next_idx++;
				if (label_next_idx >= MAX_LABELS) {
					label_next_idx = 0;
				}
				if (bulkedit_is_in_list(obj)) {
					bulkedit_remove(obj);
					sprintf(lbl->txt, "~r~del_~w~%s", modelNames[obj->model]);
				} else {
					bulkedit_add(obj);
					sprintf(lbl->txt, "~g~add_~w~%s", modelNames[obj->model]);
				}
				lbl->timeout = *timeInGame + 800;
				lbl->x = cursorx;
				lbl->y = cursory + 10.0f;
			}
			return 0;
		}
	}
	return 1;
}

static
int bulkeditui_proc_wnd_draw(void *wnd)
{
	struct RwV3D screenpos;
	int val, i, j;
	float x, y;
	float minx, miny, maxx, maxy;

	if (plantObj != NULL && plantObj->sa_object) {
		bulkedit_add(plantObj);
		plantObj = NULL;
	}

	val = bulkeditui_original_proc_wnd_draw(wnd);
	for (i = 0; i < MAX_LABELS; i++) {
		if (*timeInGame < labels[i].timeout) {
			game_TextSetAlign(CENTER);
			game_TextPrintString(labels[i].x, labels[i].y, labels[i].txt);
		}
	}

	if (!is_dragselecting && ui_mouse_is_just_down &&
		chk_dragselect->checked &&
		!ui_is_cursor_hovering_any_window())
	{
		is_dragselecting = 1;
		dragselect_start_x = cursorx;
		dragselect_start_y = cursory;
		num_on_screen_objects = 0;
		for (i = 0; i < numlayers; i++) {
			if ((!chk_select_from_active_layer->checked || layers + i == active_layer) && layers[i].show) {
				for (j = 0; j < layers[i].numobjects; j++) {
					game_WorldToScreen(&screenpos, &layers[i].objects[j].pos);
					if (screenpos.z > 0.0f) {
						on_screen_objects[num_on_screen_objects].x = screenpos.x;
						on_screen_objects[num_on_screen_objects].y = screenpos.y;
						on_screen_objects[num_on_screen_objects].obj = layers[i].objects + j;
						num_on_screen_objects++;
					}
				}
			}
		}
	}

	if (is_dragselecting) {
		if (dragselect_start_x < cursorx) {
			minx = dragselect_start_x;
			maxx = cursorx;
		} else {
			minx = cursorx;
			maxx = dragselect_start_x;
		}
		if (dragselect_start_y < cursory) {
			miny = dragselect_start_y;
			maxy = cursory;
		} else {
			miny = cursory;
			maxy = dragselect_start_y;
		}
		for (i = 0; i < num_on_screen_objects; i++) {
			x = on_screen_objects[i].x;
			y = on_screen_objects[i].y;
			if (minx < x && x < maxx && miny < y && y < maxy) {
				on_screen_objects[i].selected = 1;
			} else {
				on_screen_objects[i].selected = 0;
				on_screen_objects[i].passes_filter = 0;
			}
		}
		/*So this sets lits data every frame... not very efficient but good enough for now*/
		bulkeditui_update_list_data();
		game_DrawRect(
			dragselect_start_x, dragselect_start_y,
			cursorx - dragselect_start_x, cursory - dragselect_start_y,
			0x4400cc00);
	}

	for (i = 0; i < num_on_screen_objects; i++) {
		if (on_screen_objects[i].passes_filter) {
			entity_draw_bound_rect(on_screen_objects[i].obj->sa_object, 0x22FFFFFF);
		}
	}
	return val;
}

void bulkeditui_init()
{
	struct UI_LABEL *lbl;
	struct UI_BUTTON *btn;
	struct UI_RADIOBUTTON *rdb;
	struct RADIOBUTTONGROUP *posrdbgroup, *rotrdbgroup, *directionrdbgroup;
	int i;

	btn = ui_btn_make("Bulkedit", cb_btn_mainmenu_bulkedit);
	btn->_parent.span = 2;
	ui_wnd_add_child(main_menu, btn);

	bulkeditui_wnd = ui_wnd_make(300.0f, 20.0f, "Bulkedit");
	bulkeditui_wnd->columns = 3;
	bulkeditui_wnd->proc_close = bulkeditui_proc_close;
	bulkeditui_original_proc_wnd_draw = bulkeditui_wnd->_parent._parent.proc_draw;
	bulkeditui_wnd->_parent._parent.proc_draw = bulkeditui_proc_wnd_draw;

	posrdbgroup = ui_rdbgroup_make(cb_posrdbgroup_changed);
	rotrdbgroup = ui_rdbgroup_make(cb_rotrdbgroup_changed);
	directionrdbgroup = ui_rdbgroup_make(cb_directionrdbgroup_changed);
	ui_wnd_add_child(bulkeditui_wnd, ui_lbl_make("position"));
	ui_wnd_add_child(bulkeditui_wnd, lbl = ui_lbl_make("rotation"));
	lbl->_parent.span = 2;
	rdb = ui_rdb_make("Ignore", posrdbgroup, 0);
	rdb->_parent._parent.userdata = (void*) bulkedit_update_nop;
	ui_wnd_add_child(bulkeditui_wnd, rdb);
	rdb = ui_rdb_make("Ignore", rotrdbgroup, 0);
	rdb->_parent._parent.span = 2;
	rdb->_parent._parent.userdata = (void*) bulkedit_update_nop;
	ui_wnd_add_child(bulkeditui_wnd, rdb);
	rdb = ui_rdb_make("Sync_movement", posrdbgroup, 1);
	rdb->_parent._parent.userdata = (void*) bulkedit_update_pos_sync;
	ui_wnd_add_child(bulkeditui_wnd, rdb);
	rdb = ui_rdb_make("Sync_movement", rotrdbgroup, 1);
	rdb->_parent._parent.span = 2;
	rdb->_parent._parent.userdata = (void*) bulkedit_update_rot_sync;
	ui_wnd_add_child(bulkeditui_wnd, rdb);
	rdb = ui_rdb_make("Spread", posrdbgroup, 0);
	rdb->_parent._parent.userdata = (void*) bulkedit_update_pos_spread;
	ui_wnd_add_child(bulkeditui_wnd, rdb);
	rdb = ui_rdb_make("Spread", rotrdbgroup, 0);
	rdb->_parent._parent.span = 2;
	rdb->_parent._parent.userdata = (void*) bulkedit_update_rot_spread;
	ui_wnd_add_child(bulkeditui_wnd, rdb);
	rdb = ui_rdb_make("Copy_x", posrdbgroup, 0);
	rdb->_parent._parent.userdata = (void*) bulkedit_update_pos_copyx;
	ui_wnd_add_child(bulkeditui_wnd, rdb);
	rdb = ui_rdb_make("Object_direction", rotrdbgroup, 0);
	rdb->_parent._parent.span = 2;
	rdb->_parent._parent.userdata = (void*) bulkedit_update_rot_object_direction;
	ui_wnd_add_child(bulkeditui_wnd, rdb);
	rdb = ui_rdb_make("Copy_y", posrdbgroup, 0);
	rdb->_parent._parent.userdata = (void*) bulkedit_update_pos_copyy;
	ui_wnd_add_child(bulkeditui_wnd, rdb);
	ui_wnd_add_child(bulkeditui_wnd, NULL);
	rdb = ui_rdb_make("+0", directionrdbgroup, 1);
	rdb->_parent._parent.userdata = (void*) 0;
	ui_wnd_add_child(bulkeditui_wnd, rdb);
	rdb = ui_rdb_make("Copy_z", posrdbgroup, 0);
	rdb->_parent._parent.userdata = (void*) bulkedit_update_pos_copyz;
	ui_wnd_add_child(bulkeditui_wnd, rdb);
	ui_wnd_add_child(bulkeditui_wnd, NULL);
	rdb = ui_rdb_make("+90", directionrdbgroup, 0);
	rdb->_parent._parent.userdata = (void*) 1;
	ui_wnd_add_child(bulkeditui_wnd, rdb);
	rdb = ui_rdb_make("Sync_around_z", posrdbgroup, 0);
	rdb->_parent._parent.userdata = (void*) bulkedit_update_pos_rotaroundz;
	ui_wnd_add_child(bulkeditui_wnd, rdb);
	ui_wnd_add_child(bulkeditui_wnd, NULL);
	rdb = ui_rdb_make("-90", directionrdbgroup, 0);
	rdb->_parent._parent.userdata = (void*) 2;
	ui_wnd_add_child(bulkeditui_wnd, rdb);
	ui_wnd_add_child(bulkeditui_wnd, NULL);
	ui_wnd_add_child(bulkeditui_wnd, NULL);
	rdb = ui_rdb_make("+180", directionrdbgroup, 0);
	rdb->_parent._parent.userdata = (void*) 3;
	ui_wnd_add_child(bulkeditui_wnd, rdb);

	btn = ui_btn_make("Clone_all_objects", cb_btn_clone_all);
	btn->_parent.span = 3;
	ui_wnd_add_child(bulkeditui_wnd, btn);
	btn = ui_btn_make("Delete_all_objects", cb_btn_delete_all);
	btn->_parent.span = 3;
	ui_wnd_add_child(bulkeditui_wnd, btn);
	btn = ui_btn_make("Commit_positions", cb_btn_commit);
	btn->_parent.span = 3;
	ui_wnd_add_child(bulkeditui_wnd, btn);
	btn = ui_btn_make("Disband_bulk_edit", cb_btn_disband);
	btn->_parent.span = 3;
	ui_wnd_add_child(bulkeditui_wnd, btn);
	btn = ui_btn_make("Revert_bulk_edit", cb_btn_revert);
	btn->_parent.span = 3;
	ui_wnd_add_child(bulkeditui_wnd, btn);
	chk_plant_objects = ui_chk_make("Plant_objects", 0, NULL);
	chk_plant_objects->_parent._parent.span = 3;
	ui_wnd_add_child(bulkeditui_wnd, chk_plant_objects);
	chk_select_from_active_layer = ui_chk_make("Select_from_active_layer_only", 0, NULL);
	chk_select_from_active_layer->_parent._parent.span = 3;
	ui_wnd_add_child(bulkeditui_wnd, chk_select_from_active_layer);
	chk_captureclicks = ui_chk_make("Select_objects_for_bulkedit", 0, cb_chk_captureclicks);
	chk_captureclicks->_parent._parent.span = 3;
	ui_wnd_add_child(bulkeditui_wnd, chk_captureclicks);
	chk_dragselect = ui_chk_make("Bulk_drag_select", 0, cb_chk_dragselect);
	chk_dragselect->_parent._parent.span = 3;
	ui_wnd_add_child(bulkeditui_wnd, chk_dragselect);

	btn = ui_btn_make("Move_all_to_layer", cb_btn_move_all_to_layer);
	btn->_parent.span = 3;
	ui_wnd_add_child(bulkeditui_wnd, btn);
	for (i = 0; i < MAX_LAYERS; i++) {
		btn = btn_layer[i] = ui_btn_make("", cb_btn_movelayer);
		btn->_parent.userdata = (void*) i;
		btn->_parent.span = (i % 2) + 1;
	}

	lbl_bulk = ui_lbl_make(txt_bulk_selected_objects);
	lbl_bulk->_parent.span = 3;
	lbl_bulk_filter = ui_lbl_make("Filter:");
	in_bulk_filter = ui_in_make(cb_in_bulkfilter);
	in_bulk_filter->_parent.span = 2;
	lst_bulk_filter = ui_lst_make(20, NULL);
	lst_bulk_filter->_parent.span = 3;
	btn_bulk_addall = ui_btn_make("Add_above_to_bulk_edit", cb_btn_bulkaddall);
	btn_bulk_addall->_parent.span = 3;
}

void bulkeditui_dispose()
{
	int i;

	if (!lbl_bulk->_parent.parent) {
		UIPROC(lbl_bulk, proc_dispose);
	}
	if (!lbl_bulk_filter->_parent.parent) {
		UIPROC(lbl_bulk_filter, proc_dispose);
	}
	if (!in_bulk_filter->_parent.parent) {
		UIPROC(in_bulk_filter, proc_dispose);
	}
	if (!lst_bulk_filter->_parent.parent) {
		UIPROC(lst_bulk_filter, proc_dispose);
	}
	if (!btn_bulk_addall->_parent.parent) {
		UIPROC(btn_bulk_addall, proc_dispose);
	}
	for (i = 0; i < MAX_LAYERS; i++) {
		if (!btn_layer[i]->_parent.parent) {
			UIPROC(btn_bulk_addall, proc_dispose);
		}
	}
	ui_wnd_dispose(bulkeditui_wnd);
}
