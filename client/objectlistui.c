/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "entity.h"
#include "game.h"
#include "ide.h"
#include "objects.h"
#include "objectsui.h"
#include "objectlistui.h"
#include "objectseditor.h"
#include "player.h"
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
static struct UI_BUTTON *btn_tp_to_cam;
static struct UI_BUTTON *btn_delete;
static struct UI_BUTTON *btn_clone;
static struct UI_BUTTON *btn_edit;

static char txt_currentlayer[100];
static char txt_numobjects[20];
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
struct OBJECT *objlistui_index_to_object(int idx)
{
	if (idx >= 0 && fromLayers) {
		if (idx < active_layer->numobjects) {
			return &active_layer->objects[idx];
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

	TRACE("objlistui_refresh_list_from_layer");
	fromLayers = 1;
	btn_delete->enabled = 0;
	btn_move->enabled = 0;
	btn_edit->enabled = 0;

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
	sprintf(txt_numobjects, "%d_objects", active_layer->numobjects);
}

static
void objlistui_refresh_list_from_nearby()
{
	char *unknown = "unknown";
	char *names[MAX_OBJECTS];
	int i;

	TRACE("objlistui_refresh_list_from_nearby");
	fromLayers = 0;
	btn_delete->enabled = 0;
	btn_move->enabled = 0;
	btn_edit->enabled = 0;

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
	sprintf(txt_numobjects, "%d_objects", numNearbyEntities);
}

static
void cb_btn_mainmenu_objects(struct UI_BUTTON *btn)
{
	if (active_layer == NULL) {
		objui_show_select_layer_first_msg();
		return;
	}

	strcpy(txt_currentlayer + 15, active_layer->name);
	objlistui_refresh_list_from_layer();
	ui_show_window(wnd);
}

static
void cb_btn_mainmenu_nearby(struct UI_BUTTON *btn)
{
	if (active_layer == NULL) {
		objui_show_select_layer_first_msg();
		return;
	}

	strcpy(txt_currentlayer + 15, active_layer->name);
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
		nc.nc = NC_EditObject;
		nc.params.asint[1] = 0;
		nc.params.asint[2] = obj->samp_objectid;
		sockets_send(&nc, sizeof(nc));
	}
}

static
void cb_btn_tp_to_cam(struct UI_BUTTON *btn)
{
	struct CEntity *entity;
	struct OBJECT *obj;
	struct MSG_NC nc;

	entity = objlistui_index_to_entity(lst->selectedindex);
	if (entity != NULL) {
		obj = objects_find_by_sa_object(entity);
		if (obj != NULL) {
			nc._parent.id = MAPEDIT_MSG_NATIVECALL;
			nc.nc = NC_SetObjectPos;
			nc.params.asint[1] = obj->samp_objectid;
			nc.params.asflt[2] = camera->position.x;
			nc.params.asflt[3] = camera->position.y;
			nc.params.asflt[4] = camera->position.z;
			sockets_send(&nc, sizeof(nc));
		}
	}
}

static
void cb_msg_deleteconfirm(int opt)
{
	struct CEntity *entity;
	struct OBJECT *obj;

	if (opt != MSGBOX_RESULT_1) {
		goto ret;
	}

	entity = objlistui_index_to_entity(lst->selectedindex);
	if (entity == NULL) {
		goto ret;
	}

	obj = objects_find_by_sa_object(entity);
	if (obj == NULL) {
		goto ret;
	}

	objects_delete_obj(obj);
	if (fromLayers) {
		objlistui_refresh_list_from_layer();
	} else {
		objlistui_refresh_list_from_nearby();
	}
ret:
	ui_show_window(wnd);
}

static
void cb_btn_center(struct UI_BUTTON *btn)
{
	struct RwV3D pos;
	struct CEntity *entity;

	entity = objlistui_index_to_entity(lst->selectedindex);
	if (entity != NULL) {
		game_ObjectGetPos(entity, &pos);
		center_camera_on(&pos);
	}
}

static
void cb_objlistui_btn_delete(struct UI_BUTTON *btn)
{
	struct CEntity *entity;
	struct OBJECT *obj;

	entity = objlistui_index_to_entity(lst->selectedindex);
	if (entity == NULL) {
		return;
	}

	obj = objects_find_by_sa_object(entity);
	if (obj != NULL) {
		objui_show_delete_confirm_msg(cb_msg_deleteconfirm);
		return;
	}

	rbe_show_for_entity(entity, objui_cb_rb_save_new);
}

static
void cb_btn_objlistui_clone(struct UI_BUTTON *btn)
{
	struct CEntity *entity;

	entity = objlistui_index_to_entity(lst->selectedindex);
	if (entity != NULL) {
		lst->selectedindex = -1;
		objects_clone(objlistui_index_to_object(lst->selectedindex), entity);
	}
}

static
void cb_btn_objlistui_edit(struct UI_BUTTON *btn)
{
	struct CEntity *entity;
	struct OBJECT *obj;

	entity = objlistui_index_to_entity(lst->selectedindex);
	if (entity != NULL) {
		obj = objects_find_by_sa_object(entity);
		if (obj != NULL) {
			objedit_show(obj);
			return;
		}
	}
}

static
void cb_btn_jmp2objcenter(struct UI_BUTTON *btn)
{
	struct OBJECTLAYER *layer;
	struct OBJECT *obj;
	int i, j, n;

	n = 0;
	player_position.x = player_position.y = player_position.z = 0.0f;
	for (i = 0, layer = layers; i < numlayers; i++, layer++) {
		for (j = 0, obj = layer->objects; j < layer->numobjects; j++, obj++) {
			player_position.x += obj->pos.x;
			player_position.y += obj->pos.y;
			player_position.z += obj->pos.z;
			n++;
		}
	}
	player_position.x /= n;
	player_position.y /= n;
	player_position.z /= n;
	ui_place_camera_behind_player();
}

static
void cb_btn_jmp2sacenter(struct UI_BUTTON *btn)
{
	player_position.x = player_position.y = 0.0f;
	player_position.z = 10.0f;
	ui_place_camera_behind_player();
}

static
void cb_list_item_selected(struct UI_LIST *lst)
{
	struct CEntity *entity;
	struct OBJECT *obj;

	entity = objlistui_index_to_entity(lst->selectedindex);
	if (entity == NULL) {
		btn_center->enabled =
			btn_move->enabled =
			btn_tp_to_cam->enabled =
			btn_delete->enabled =
			btn_clone->enabled =
			btn_edit->enabled = 0;
	} else {
		btn_center->enabled =
			btn_delete->enabled =
			btn_clone->enabled = 1;
		obj = objects_find_by_sa_object(entity);
		btn_move->enabled =
			btn_tp_to_cam->enabled =
			btn_edit->enabled = obj != NULL;
	}
}

void objlistui_init()
{
	struct UI_BUTTON *btn;

	btn = ui_btn_make("Objects", cb_btn_mainmenu_objects);
	ui_wnd_add_child(main_menu, btn);
	btn = ui_btn_make("Nearby", cb_btn_mainmenu_nearby);
	ui_wnd_add_child(main_menu, btn);
	btn = ui_btn_make("jmp2sacenter", cb_btn_jmp2sacenter);
	btn->_parent.span = 2;
	ui_wnd_add_child(main_menu, btn);
	btn = ui_btn_make("jmp2objcenter", cb_btn_jmp2objcenter);
	btn->_parent.span = 2;
	ui_wnd_add_child(main_menu, btn);

	wnd = ui_wnd_make(9000.0f, 300.0f, "Objects");

	strcpy(txt_currentlayer, "Current_layer:_");
	ui_wnd_add_child(wnd, ui_lbl_make(txt_currentlayer));
	ui_wnd_add_child(wnd, ui_lbl_make(txt_numobjects));
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
	btn_tp_to_cam = ui_btn_make("TP_Object_to_camera", cb_btn_tp_to_cam);
	ui_wnd_add_child(wnd, btn_tp_to_cam);
	btn_delete = ui_btn_make("Delete", cb_objlistui_btn_delete);
	ui_wnd_add_child(wnd, btn_delete);
	btn_clone = ui_btn_make("Clone", cb_btn_objlistui_clone);
	ui_wnd_add_child(wnd, btn_clone);
	btn_edit = ui_btn_make("Edit", cb_btn_objlistui_edit);
	ui_wnd_add_child(wnd, btn_edit);
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
	int idx;

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
		entity_draw_bound_rect(entity, 0xFF);
	}

	if (exclusiveEntity == lastHoveredEntity) {
		return;
	}

	lastHoveredEntity = exclusiveEntity;
	if (exclusiveEntity == NULL || chk_isolate_element->checked) {
		entity_render_exclusively(exclusiveEntity);
	}
	if (exclusiveEntity != NULL && chk_snap_camera->checked) {
		game_ObjectGetPos(exclusiveEntity, &pos);
		center_camera_on(&pos);
	}
}

void objlistui_on_active_window_changed(struct UI_WINDOW *new_wnd)
{
	if (new_wnd == wnd) {
		isactive = 1;
		lastHoveredEntity = NULL;
		/*updates buttons*/
		lst->selectedindex = -1;
		cb_list_item_selected(lst);
	} else if (isactive) {
		if (lastHoveredEntity != NULL) {
			entity_render_exclusively(NULL);
		}
		isactive = 0;
	}
}
