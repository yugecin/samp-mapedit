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
#include <math.h>
#include <stdio.h>

static struct UI_WINDOW *wnd;
static struct UI_CHECKBOX *chk_snap_camera;
static struct UI_CHECKBOX *chk_isolate_element;
static struct UI_LIST *lst;
static struct UI_BUTTON *btn_center;
static struct UI_BUTTON *btn_move;
static struct UI_BUTTON *btn_delete;

static char txt_currentlayer[100];
static char isactive;
static struct CEntity *lastHoveredEntity;
static struct CEntity *nearbyEntities[MAX_OBJECTS];
static short numNearbyEntities;
static char fromLayers;

static
struct CEntity *objlistui_index_to_entity(int idx)
{
	if (idx < 0) {
		return NULL;
	}

	if (fromLayers) {
		if (idx < active_layer->numobjects) {
			return active_layer->objects[idx].sa_object;
		}
	} else {
		if (idx < numNearbyEntities) {
			return nearbyEntities[idx];
		}
	}
	return NULL;
}

static
void objlistui_refresh_list_from_layer()
{
	char *unknown = "unknown";
	char *names[MAX_OBJECTS];
	int i;
	struct OBJECT *obj;

	btn_delete->enabled = 0;
	btn_move->enabled = 0;

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

static
void objlistui_refresh_list_from_nearby()
{
	char *unknown = "unknown";
	char *names[MAX_OBJECTS];
	int i;

	btn_delete->enabled = 0;
	btn_move->enabled = 0;

	game_WorldFindObjectsInRange(
		&camera->position,
		100.0f,
		0,
		&numNearbyEntities,
		MAX_OBJECTS,
		nearbyEntities,
		1,
		0,
		0,
		1,
		1);

	for (i = 0; i < numNearbyEntities; i++) {
		if (modelNames[nearbyEntities[i]->model]) {
			names[i] = modelNames[nearbyEntities[i]->model];
		} else {
			names[i] = unknown;
		}
	}
	ui_lst_set_data(lst, names, i);
}

static
void cb_btn_mainmenu_objects(struct UI_BUTTON *btn)
{
	if (active_layer == NULL) {
		objects_show_select_layer_first_msg();
		return;
	}

	strcpy(txt_currentlayer + 15, active_layer->name);
	fromLayers = 1;
	objlistui_refresh_list_from_layer();
	ui_show_window(wnd);
}

static
void cb_btn_mainmenu_nearby(struct UI_BUTTON *btn)
{
	if (active_layer == NULL) {
		objects_show_select_layer_first_msg();
		return;
	}

	strcpy(txt_currentlayer + 15, active_layer->name);
	fromLayers = 0;
	objlistui_refresh_list_from_nearby();
	ui_show_window(wnd);
}

static
void cb_btn_move(struct UI_BUTTON *btn)
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
	objlistui_refresh_list_from_layer();

ret:
	ui_show_window(wnd);
}

static
void cb_btn_center(struct UI_BUTTON *btn)
{
	struct RwV3D pos;
	int idx;

	idx = lst->selectedindex;
	if (active_layer != NULL &&
		0 <= idx &&
		idx < active_layer->numobjects)
	{
		game_ObjectGetPos(active_layer->objects[idx].sa_object, &pos);
		center_camera_on(&pos);
	}
}

static
void cb_btn_delete(struct UI_BUTTON *btn)
{
	struct CEntity *entity;
	struct OBJECT *obj;

	entity = objlistui_index_to_entity(lst->selectedindex);
	if (entity == NULL) {
		return;
	}

	obj = objects_find_by_sa_object(entity);
	if (obj != NULL) {
		msg_title = "Object";
		msg_message = "Delete_object?";
		msg_message2 = "This_cannot_be_undone!";
		msg_btn1text = "Yes";
		msg_btn2text = "No";
		msg_show(cb_msg_deleteconfirm);
		return;
	}

	rbe_show_for_entity(entity, objects_cb_rb_save_new);
}

static
void cb_list_item_selected(struct UI_LIST *lst)
{
	struct CEntity *entity;

	entity = objlistui_index_to_entity(lst->selectedindex);
	if (entity == NULL) {
		btn_center->enabled =
			btn_move->enabled = btn_delete->enabled = 0;
	} else {
		btn_center->enabled = btn_delete->enabled = 1;
		if (objects_find_by_sa_object(entity) != NULL) {
			btn_move->enabled = 1;
		}
	}
}

void objlistui_init()
{
	struct UI_BUTTON *btn;

	btn = ui_btn_make("Objects", cb_btn_mainmenu_objects);
	btn->_parent.span = 2;
	ui_wnd_add_child(main_menu, btn);
	btn = ui_btn_make("Nearby", cb_btn_mainmenu_nearby);
	btn->_parent.span = 2;
	ui_wnd_add_child(main_menu, btn);

	wnd = ui_wnd_make(9000.0f, 300.0f, "Objects");

	strcpy(txt_currentlayer, "Current_layer:_");
	ui_wnd_add_child(wnd, ui_lbl_make(txt_currentlayer));
	chk_snap_camera = ui_chk_make("Snap_camera", 0, NULL);
	ui_wnd_add_child(wnd, chk_snap_camera);
	chk_isolate_element = ui_chk_make("Isolate_object", 1, NULL);
	ui_wnd_add_child(wnd, chk_isolate_element);
	lst = ui_lst_make(35, cb_list_item_selected);
	ui_wnd_add_child(wnd, lst);
	btn_center = ui_btn_make("Center_on_screen", cb_btn_center);
	ui_wnd_add_child(wnd, btn_center);
	btn_move = ui_btn_make("Move", cb_btn_move);
	ui_wnd_add_child(wnd, btn_move);
	btn_delete = ui_btn_make("Delete", cb_btn_delete);
	ui_wnd_add_child(wnd, btn_delete);
}

void objlistui_dispose()
{
	ui_wnd_dispose(wnd);
}

void objlistui_frame_update()
{
	struct CEntity *exclusiveEntity;
	struct CEntity *entity;
	struct RwV3D pos;
	int idx, col;

	if (!isactive) {
		return;
	}

	exclusiveEntity = NULL;
	idx = lst->hoveredindex;
	if (idx < 0) {
		idx = lst->selectedindex;
	} else {
		entity = objlistui_index_to_entity(idx);
		exclusiveEntity = entity;
		goto hasentity;
	}
	entity = objlistui_index_to_entity(idx);
	if (entity != NULL) {
hasentity:
		col = (BBOX_ALPHA_ANIM_VALUE << 24) | 0xFF;
		objbase_draw_entity_bound_rect(entity, col);
	}

	if (exclusiveEntity != lastHoveredEntity) {
		lastHoveredEntity = exclusiveEntity;
		objbase_set_entity_to_render_exclusively(exclusiveEntity);
		if (exclusiveEntity != NULL && chk_snap_camera->checked) {
			game_ObjectGetPos(exclusiveEntity, &pos);
			center_camera_on(&pos);
		}
	}
}

void objlistui_on_active_window_changed(struct UI_WINDOW *new_wnd)
{
	if (new_wnd == wnd) {
		isactive = 1;
		lastHoveredEntity = NULL;
	} else if (isactive) {
		if (lastHoveredEntity != NULL) {
			objbase_set_entity_to_render_exclusively(NULL);
		}
		isactive = 0;
	}
}
