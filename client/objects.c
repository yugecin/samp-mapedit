/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ide.h"
#include "msgbox.h"
#include "ui.h"
#include "objbase.h"
#include "objects.h"
#include "objbrowser.h"
#include "objectstorage.h"
#include "persistence.h"
#include "player.h"
#include "project.h"
#include "removedbuildings.h"
#include "removebuildingeditor.h"
#include "removedbuildingsui.h"
#include "sockets.h"
#include "../shared/serverlink.h"
#include <string.h>
#include <stdio.h>
#include <windows.h>

static struct UI_WINDOW *window_layers;
static struct UI_LABEL *lbl_layer;

static struct UI_BUTTON *btn_mainmenu_layers, *btn_mainmenu_selectobject;
static struct UI_BUTTON *btn_contextmenu_mkobject;
static struct UI_LIST *lst_layers;
static struct UI_INPUT *in_layername;

static struct UI_WINDOW *window_objinfo;
static struct UI_LABEL *lbl_objentity;
static struct UI_LABEL *lbl_objflags;
static struct UI_LABEL *lbl_objmodel;
static struct UI_LABEL *lbl_objlodmodel;
static struct UI_BUTTON *btn_remove_building;
static struct UI_BUTTON *btn_objclone;
static struct UI_BUTTON *btn_move_obj;
static struct UI_BUTTON *btn_tp_obj_to_cam;
static struct UI_BUTTON *btn_delete_obj;

static char txt_objentity[9];
static char txt_objmodel[45];
static char txt_objtype[9];
static char txt_objflags[9];
static char txt_objlodentity[9];
static char txt_objlodmodel[45];
static char txt_objlodflags[9];

static struct CEntity *selected_entity;
static struct OBJECT *selected_object;
static struct OBJECT cloning_object;
static struct RwV3D cloning_object_rot;
static int activelayeridx = 0;
static struct RwV3D nextObjectPosition;
static struct RwV3D player_pos_before_selecting;
static int is_selecting_object = 0;

struct OBJECTLAYER layers[MAX_LAYERS];
struct OBJECTLAYER *active_layer = NULL;
int numlayers = 0;

static
void ensure_layer_name_unique(struct OBJECTLAYER *layer)
{
	char newname[INPUT_TEXTLEN + 1];
	int i, try;

	try = 0;
	strcpy(newname, layer->name);
	for (i = 0; i < numlayers; i++) {
		if (layers + i != layer) {
			if (!strcmp(newname, layers[i].name)) {
				try++;
				sprintf(newname, "%s%d", layer->name, try);
			}
		}
	}
	if (try != 0) {
		strcpy(layer->name, newname);
	}
}

static
void cb_msg_openlayers(int choice)
{
	ui_show_window(window_layers);
}

static
void cb_btn_mkobject(struct UI_BUTTON *btn)
{
	if (active_layer == NULL) {
		msg_message = "Create_and_select_an_object_layer_first!";
		msg_title = "Objects";
		msg_btn1text = "Ok";
		msg_show(cb_msg_openlayers);
	} else if (active_layer->numobjects == MAX_OBJECTS) {
		msg_message = "Layer_object_limit_reached.";
		msg_title = "Objects";
		msg_btn1text = "Ok";
		msg_show(NULL);
	} else {
		objbrowser_show(&nextObjectPosition);
	}
}

static
void layer_activate(int idx)
{
	if (0 <= idx && idx < numlayers) {
		activelayeridx = idx;
		active_layer = layers + idx;
		lbl_layer->text = active_layer->name;
		ui_lbl_recalc_size(lbl_layer);
		ui_in_set_text(in_layername, active_layer->name);
		ui_lst_set_selected_index(lst_layers, activelayeridx);
		persistence_set_object_layerid(activelayeridx);
		rbui_refresh_list();
	}
}

static
void update_layer_list()
{
	char *layernames[MAX_LAYERS];
	int i;

	for (i = 0; i < MAX_LAYERS; i++) {
		layernames[i] = layers[i].name;
	}
	ui_lst_set_data(lst_layers, layernames, numlayers);
}

static
void cb_lst_layers(struct UI_LIST *lst)
{
	int i;

	i = lst->selectedindex;
	if (0 <= i && i < numlayers) {
		layer_activate(i);
	}
}

static
void cb_in_layername(struct UI_INPUT *in)
{
	int i;
	char c;

	if (active_layer == NULL) {
		return;
	}
	for (i = 0; i < sizeof(active_layer->name); i++) {
		c = in->value[i];
		if (c == ' ') {
			c = '_';
		}
		active_layer->name[i] = c;
		if (c == 0) {
			break;
		}
	}
	ensure_layer_name_unique(active_layer);
	update_layer_list();
	ui_lbl_recalc_size(lbl_layer);
}

static
void cb_cp_layercolor(struct UI_COLORPICKER *cp)
{
}

static
void cb_show_layers_window()
{
	ui_show_window(window_layers);
}

static
void cb_msg_show_objinfo_window(int choice)
{
	ui_show_window(window_objinfo);
}

static
void cb_show_objinfo_window()
{
	if (active_layer == NULL) {
		objects_show_select_layer_first_msg();
	} else {
		ui_show_window(window_objinfo);
	}
}

static
void cb_btn_add_layer(struct UI_BUTTON *btn)
{
	if (numlayers >= MAX_LAYERS) {
		msg_message = "Can't_add_more_layers";
		msg_title = "Objects";
		msg_btn1text = "Ok";
		msg_show((void*) cb_show_layers_window);
	} else {
		layers[numlayers].color = 0xFF000000;
		layers[numlayers].numobjects = 0;
		layers[numlayers].numremoves = 0;
		strcpy(layers[numlayers].name, "new_layer");
		ensure_layer_name_unique(layers + numlayers);
		numlayers++;
		update_layer_list();
		layer_activate(numlayers - 1);
	}
}

static
void cb_delete_layer_confirm(int choice)
{
	struct MSG_NC nc;
	struct OBJECTLAYER *layer;
	int i;
	int idx;

	idx = lst_layers->selectedindex;
	if (choice == MSGBOX_RESULT_1 && 0 <= idx && idx < numlayers) {
		layer = layers + idx;
		objectstorage_mark_layerfile_for_deletion(layer);
		for (i = 0; i < layer->numremoves; i++) {
			if (layer->removes[i].description) {
				free(layer->removes[i].description);
			}
		}
		for (i = 0; i < layer->numobjects; i++) {
			nc._parent.id = MAPEDIT_MSG_NATIVECALL;
			nc._parent.data = 0;
			nc.nc = NC_DestroyObject;
			nc.params.asint[1] = layer->objects[i].samp_objectid;
			sockets_send(&nc, sizeof(nc));
		}
		if (idx < numlayers - 1) {
			memcpy(layers + idx,
				layers + idx + 1,
				sizeof(struct OBJECTLAYER) *
					(numlayers - idx - 1));
		}
		numlayers--;
		update_layer_list();
		if (numlayers) {
			if (numlayers == idx) {
				layer_activate(idx - 1);
			} else {
				layer_activate(idx);
			}
		} else {
			active_layer = NULL;
			activelayeridx = 0;
			lbl_layer->text = "<none>";
		}
		rb_undo_all();
		rb_do_all();
	}
	ui_show_window(window_layers);
}

static
void cb_btn_delete_layer()
{
	int idx;

	idx = lst_layers->selectedindex;
	if (0 <= idx && idx < numlayers) {
		msg_message = "Delete_layer?";
		msg_message2 = "All_Objects_in_the_layer_will_be_deleted!";
		msg_title = "Delete_Layer";
		msg_btn1text = "Yes";
		msg_btn2text = "No";
		msg_show(cb_delete_layer_confirm);
	}
}

static
void cb_btn_remove_building(struct UI_BUTTON *btn)
{
	struct CEntity *entity;

	if (selected_entity) {
		entity = selected_entity;
		ui_hide_window(); /*clears selected_entity*/
		rbe_show_for_entity(entity, objects_cb_rb_save_new);
	}
}

static
void cb_btn_objclone(struct UI_BUTTON *btn)
{
	if (active_layer->numobjects >= MAX_OBJECTS) {
		msg_title = "Clone";
		msg_message = "Can't_clone,_object_limit_reached";
		msg_btn1text = "Ok";
		msg_show(NULL);
		return;
	}

	if (selected_entity != NULL) {
		objects_clone_object(selected_entity);
	}
}

static
void cb_btn_move_obj(struct UI_BUTTON *btn)
{
	struct MSG_NC nc;

	if (selected_object != NULL) {
		nc._parent.id = MAPEDIT_MSG_NATIVECALL;
		nc._parent.data = 0;
		nc.nc = NC_EditObject;
		nc.params.asint[1] = 0;
		nc.params.asint[2] = selected_object->samp_objectid;
		sockets_send(&nc, sizeof(nc));
	}
}

static
void cb_btn_tp_obj_to_cam(struct UI_BUTTON *btn)
{
	struct MSG_NC nc;

	if (selected_object != NULL) {
		nc._parent.id = MAPEDIT_MSG_NATIVECALL;
		nc._parent.data = 0;
		nc.nc = NC_SetObjectPos;
		nc.params.asint[1] = selected_object->samp_objectid;
		nc.params.asflt[2] = camera->position.x;
		nc.params.asflt[3] = camera->position.y;
		nc.params.asflt[4] = camera->position.z;
		sockets_send(&nc, sizeof(nc));
		objects_select_entity(selected_object->sa_object);
	}
}

static struct OBJECT *object_to_delete_after_confirm;

static
void cb_confirm_delete(int choice)
{
	if (object_to_delete_after_confirm == NULL) {
		ui_show_window(window_objinfo);
	}

	if (choice == MSGBOX_RESULT_1) {
		objects_delete_obj(object_to_delete_after_confirm);
		ui_show_window(window_objinfo);
		return;
	}

	ui_show_window(window_objinfo);
	objects_select_entity(object_to_delete_after_confirm->sa_object);
}

static
void cb_btn_delete_obj(struct UI_BUTTON *btn)
{
	if (selected_object != NULL) {
		object_to_delete_after_confirm = selected_object;
		objects_show_delete_confirm_msg(cb_confirm_delete);
	}
}

void objects_init()
{
	struct UI_BUTTON *btn;
	struct UI_LABEL *lbl;
	struct UI_COLORPICKER *cp;
	struct MSG msg;

	msg.id = MAPEDIT_MSG_RESETOBJECTS;
	sockets_send(&msg, sizeof(msg));

	/*context menu entry*/
	btn = ui_btn_make("Make_Object", cb_btn_mkobject);
	ui_wnd_add_child(context_menu, btn);
	btn->enabled = 0;
	btn_contextmenu_mkobject = btn;

	/*main menu entries*/
	lbl = ui_lbl_make("=_Objects_=");
	lbl->_parent.span = 2;
	ui_wnd_add_child(main_menu, lbl);
	btn = ui_btn_make("Layers", (btncb*) cb_show_layers_window);
	btn->_parent.span = 2;
	ui_wnd_add_child(main_menu, btn);
	btn->enabled = 0;
	btn_mainmenu_layers = btn;
	lbl = ui_lbl_make("Layer:");
	ui_wnd_add_child(main_menu, lbl);
	lbl_layer = ui_lbl_make("<none>");
	ui_wnd_add_child(main_menu, lbl_layer);
	btn = ui_btn_make("Select_Object", (btncb*) cb_show_objinfo_window);
	btn->_parent.span = 2;
	btn->enabled = 0;
	btn_mainmenu_selectobject = btn;
	ui_wnd_add_child(main_menu, btn);

	/*layers window*/
	window_layers = ui_wnd_make(500.0f, 500.0f, "Object_Layers");
	window_layers->columns = 2;

	lst_layers = ui_lst_make(5, cb_lst_layers);
	lst_layers->_parent.span = 2;
	ui_wnd_add_child(window_layers, lst_layers);
	btn = ui_btn_make("add_layer", cb_btn_add_layer);
	ui_wnd_add_child(window_layers, btn);
	btn = ui_btn_make("delete_layer", (void*) cb_btn_delete_layer);
	ui_wnd_add_child(window_layers, btn);
	lbl = ui_lbl_make("Layer_name:");
	ui_wnd_add_child(window_layers, lbl);
	in_layername = ui_in_make(cb_in_layername);
	ui_wnd_add_child(window_layers, in_layername);
	lbl = ui_lbl_make("Layer_color:");
	ui_wnd_add_child(window_layers, lbl);
	cp = ui_colpick_make(35.0f, cb_cp_layercolor);
	ui_wnd_add_child(window_layers, cp);

	/*objinfo window*/
	window_objinfo = ui_wnd_make(1500.0f, 400.0f, "Entity_Info");
	window_objinfo->columns = 2;

	ui_wnd_add_child(window_objinfo, ui_lbl_make("Entity:"));
	ui_wnd_add_child(window_objinfo, ui_lbl_make(txt_objentity));
	ui_wnd_add_child(window_objinfo, ui_lbl_make("Model:"));
	lbl_objmodel = ui_lbl_make(txt_objmodel);
	ui_wnd_add_child(window_objinfo, lbl_objmodel);
	ui_wnd_add_child(window_objinfo, ui_lbl_make("Type:"));
	ui_wnd_add_child(window_objinfo, ui_lbl_make(txt_objtype));
	ui_wnd_add_child(window_objinfo, ui_lbl_make("Flags:"));
	ui_wnd_add_child(window_objinfo, ui_lbl_make(txt_objflags));
	ui_wnd_add_child(window_objinfo, ui_lbl_make("LOD_Entity:"));
	ui_wnd_add_child(window_objinfo, ui_lbl_make(txt_objlodentity));
	ui_wnd_add_child(window_objinfo, ui_lbl_make("LOD_Model:"));
	lbl_objlodmodel = ui_lbl_make(txt_objlodmodel);
	ui_wnd_add_child(window_objinfo, lbl_objlodmodel);
	ui_wnd_add_child(window_objinfo, ui_lbl_make("LOD_Flags:"));
	ui_wnd_add_child(window_objinfo, ui_lbl_make(txt_objlodflags));
	btn = ui_btn_make("Remove_Building", cb_btn_remove_building);
	btn->_parent.span = 2;
	btn->enabled = 0;
	ui_wnd_add_child(window_objinfo, btn_remove_building = btn);
	btn = ui_btn_make("Clone", cb_btn_objclone);
	btn->_parent.span = 2;
	btn->enabled = 0;
	ui_wnd_add_child(window_objinfo, btn_objclone = btn);
	btn = ui_btn_make("Move_Object", cb_btn_move_obj);
	btn->_parent.span = 2;
	btn->enabled = 0;
	ui_wnd_add_child(window_objinfo, btn_move_obj = btn);
	btn = ui_btn_make("TP_Object_to_camera", cb_btn_tp_obj_to_cam);
	btn->_parent.span = 2;
	btn->enabled = 0;
	ui_wnd_add_child(window_objinfo, btn_tp_obj_to_cam = btn);
	btn = ui_btn_make("Delete_Object", cb_btn_delete_obj);
	btn->_parent.span = 2;
	btn->enabled = 0;
	ui_wnd_add_child(window_objinfo, btn_delete_obj = btn);

	selected_object = NULL;
}

void objects_dispose()
{
	TRACE("objects_dispose");

	objects_clearlayers();
}

void objects_prj_save(FILE *f, char *buf)
{
	int i;

	for (i = 0; i < numlayers; i++) {
		_asm push f
		_asm push 1
		_asm push 0
		_asm push buf
		sprintf(buf, "obj.layer.%c.name %s\n", i + 'a', layers[i].name);
		_asm mov [esp+0x4], eax
		_asm call fwrite
		sprintf(buf, "obj.layer.%c.col %d\n", i + 'a', layers[i].color);
		_asm mov [esp+0x4], eax
		_asm call fwrite
		sprintf(buf, "obj.numlayers %d\n", numlayers);
		_asm mov [esp+0x4], eax
		_asm call fwrite
		_asm add esp, 0x10

		objectstorage_save_layer(layers + i);
	}
	objectstorage_delete_layerfiles_marked_for_deletion();
}

int objects_prj_load_line(char *buf)
{
	int idx, i, j;

	if (strncmp("obj.layer.", buf, 10) == 0) {
		idx = buf[10] - 'a';
		if (0 <= idx && idx < MAX_LAYERS) {
			if (strncmp(".name", buf + 11, 5) == 0) {
				i = 17;
				j = 0;
				while (j < sizeof(layers[idx].name)) {
					if (buf[i] == 0 || buf[i] == '\n') {
						j++;
						break;
					}
					layers[idx].name[j] = buf[i];
					j++;
					i++;
				}
				layers[idx].name[j - 1] = 0;
			} else if (strncmp(".col", buf + 11, 4) == 0) {
				layers[idx].color = atoi(buf + 16);
			}
		}
		return 1;
	} else if (strncmp("obj.numlayers", buf, 13) == 0) {
		numlayers = atoi(buf + 14);
		return 1;
	}
	return 0;
}

void objects_prj_preload()
{
	struct MSG msg;

	msg.id = MAPEDIT_MSG_RESETOBJECTS;
	sockets_send(&msg, sizeof(msg));

	objects_clearlayers();
	lbl_layer->text = "<none>";
	ui_lbl_recalc_size(lbl_layer);
	ui_in_set_text(in_layername, "");
}

void objects_prj_postload()
{
	int layeridx;

	update_layer_list();
	btn_contextmenu_mkobject->enabled = 1;
	btn_mainmenu_layers->enabled = 1;
	btn_mainmenu_selectobject->enabled = 1;
	cloning_object.model = 0;
	for (layeridx = 0; layeridx < numlayers; layeridx++) {
		objectstorage_load_layer(layers + layeridx);
	}
	objbase_create_dummy_entity();
}

void objects_select_entity(void *entity)
{
	void *lod;
	unsigned short modelid, lodmodel;

	objbase_select_entity(entity);
	selected_entity = entity;

	if (entity == NULL) {
		btn_objclone->enabled = 0;
		selected_object = NULL;
		btn_remove_building->enabled = 0;
		btn_move_obj->enabled = 0;
		btn_tp_obj_to_cam->enabled = 0;
		btn_delete_obj->enabled = 0;
		strcpy(txt_objentity, "00000000");
		strcpy(txt_objmodel, "0");
		strcpy(txt_objtype, "00000000");
		strcpy(txt_objflags, "00000000");
		strcpy(txt_objlodentity, "00000000");
		strcpy(txt_objlodmodel, "0");
		strcpy(txt_objlodflags, "00000000");
		return;
	}

	btn_objclone->enabled = 1;
	lod = *((int**) ((char*) entity + 0x30));
	modelid = *((unsigned short*) entity + 0x11);
	sprintf(txt_objentity, "%p", entity);
	sprintf(txt_objmodel, "%hd", modelid);
	if (modelNames[modelid]) {
		sprintf(txt_objmodel, "%s", modelNames[modelid]);
	} else {
		sprintf(txt_objmodel, "%hd", modelid);
	}
	sprintf(txt_objtype, "%d", (int) *((char*) entity + 0x36));
	sprintf(txt_objflags, "%p", *((int*) entity + 7));
	if ((int) lod == -1 || lod == NULL) {
		strcpy(txt_objlodentity, "00000000");
		strcpy(txt_objlodflags, "00000000");
		strcpy(txt_objlodmodel, "0");
	} else {
		sprintf(txt_objlodentity, "%p", (int) lod);
		lodmodel = *((short*) lod + 0x11);
		if (modelNames[lodmodel]) {
			sprintf(txt_objlodmodel, "%s", modelNames[lodmodel]);
		} else {
			sprintf(txt_objlodmodel, "%hd", lodmodel);
		}
		sprintf(txt_objlodflags, "%p", *((int*) lod + 7));
	}
	ui_lbl_recalc_size(lbl_objmodel);
	ui_lbl_recalc_size(lbl_objlodmodel);
	selected_object = objects_find_by_sa_object(entity);
	if (selected_object == NULL) {
		btn_remove_building->enabled = 1;
		btn_move_obj->enabled = 0;
		btn_tp_obj_to_cam->enabled = 0;
		btn_delete_obj->enabled = 0;
	} else {
		btn_remove_building->enabled = 0;
		btn_move_obj->enabled = 1;
		btn_tp_obj_to_cam->enabled = 1;
		btn_delete_obj->enabled = 1;
	}
}

int objects_on_background_element_just_clicked(colpoint, entity)
	struct CColPoint *colpoint;
	void *entity;
{
	if (objects_is_currently_selecting_object()) {
		objects_select_entity(entity);
		return 0;
	}

	if (entity) {
		nextObjectPosition = colpoint->pos;
	} else {
		game_ScreenToWorld(
			&nextObjectPosition, bgclickx, bgclicky, 60.0f);
	}
	return 1;
}

static
void objects_prepare_selecting_object()
{
	game_EntitySetAlpha(player, 0);
	player_pos_before_selecting = player_position;
	player_position_for_camera = &player_pos_before_selecting;
}

static
void objects_restore_selecting_object()
{
	game_EntitySetAlpha(player, 255);
	player_position = player_pos_before_selecting;
	game_PedSetPos(player, &player_position);
	player_position_for_camera = &player_position;
}

void objects_on_active_window_changed(struct UI_WINDOW *wnd)
{
	if (wnd == window_objinfo) {
		is_selecting_object = 1;
		objects_prepare_selecting_object();
		btn_remove_building->enabled = 0;
		btn_move_obj->enabled = 0;
		btn_tp_obj_to_cam->enabled = 0;
		btn_delete_obj->enabled = 0;
	} else if (is_selecting_object) {
		is_selecting_object = 0;
		objects_restore_selecting_object();
	}
	objects_select_entity(NULL);
}

void objects_ui_activated()
{
	if (is_selecting_object) {
		objects_prepare_selecting_object();
	}
}

void objects_ui_deactivated()
{
	if (is_selecting_object) {
		objects_restore_selecting_object();
	}
}

int objects_is_currently_selecting_object()
{
	return ui_get_active_window() == window_objinfo;
}

void objects_frame_update()
{
	if (is_selecting_object) {
		game_TextSetAlign(CENTER);
		game_TextPrintString(fresx / 2.0f, 50.0f,
			"~w~Hover over an object and ~r~click~w~ to select it");
	}
}

int objects_handle_esc()
{
	if (is_selecting_object) {
		ui_hide_window();
		return 1;
	}
	return 0;
}

void objects_open_persistent_state()
{
	layer_activate(persistence_get_object_layerid());
}

void objects_clearlayers()
{
	char *description;
	int i;

	while (numlayers > 0) {
		numlayers--;
		for (i = 0; i < layers[numlayers].numremoves; i++) {
			description = layers[numlayers].removes[i].description;
			if (description != NULL) {
				free(description);
			}
		}
	}
	active_layer = NULL;
	activelayeridx = 0;
}

void objects_show_select_layer_first_msg()
{
	msg_message = "Create_and_select_an_object_layer_first!";
	msg_title = "Objects";
	msg_btn1text = "Ok";
	msg_show(cb_msg_openlayers);
}

int objects_object_created(struct OBJECT *object)
{
	struct MSG_NC nc;

	if (object != &cloning_object) {
		return 0;
	}

	if (cloning_object.model == 0) {
		return 1;
	}

	if (active_layer->numobjects >= MAX_OBJECTS) {
		msg_message = "Max_objects_reached!";
		msg_title = "Clone";
		msg_btn1text = "Ok";
		msg_show(NULL);
		return 1;
	}

	active_layer->objects[active_layer->numobjects] = cloning_object;
	active_layer->numobjects++;

	nc._parent.id = MAPEDIT_MSG_NATIVECALL;
	nc._parent.data = 0;
	nc.nc = NC_SetObjectRot;
	nc.params.asint[1] = cloning_object.samp_objectid;
	nc.params.asflt[2] = cloning_object_rot.x;
	nc.params.asflt[3] = cloning_object_rot.y;
	nc.params.asflt[4] = cloning_object_rot.z;
	sockets_send(&nc, sizeof(nc));

	nc._parent.id = MAPEDIT_MSG_NATIVECALL;
	nc._parent.data = 0;
	nc.nc = NC_EditObject;
	nc.params.asint[1] = 0;
	nc.params.asint[2] = cloning_object.samp_objectid;
	sockets_send(&nc, sizeof(nc));

	cloning_object.model = 0;
	return 1;
}

/**
TODO: optimize this
*/
struct OBJECT *objects_find_by_sa_handle(int sa_handle)
{
	struct OBJECTLAYER *layer;
	struct OBJECT *objects;
	int i, layersleft;

	objects = objbrowser_object_by_handle(sa_handle);
	if (objects != NULL) {
		return objects;
	}

	if (cloning_object.sa_handle == sa_handle) {
		return &cloning_object;
	}

	layer = layers;
	layersleft = numlayers;
	while (layersleft--) {
		objects = layer->objects;
		for (i = layer->numobjects - 1; i >= 0; i--) {
			if (objects[i].sa_handle == sa_handle) {
				return objects + i;
			}
		}
		layer++;
	}
	return NULL;
}

/**
TODO: optimize this
*/
struct OBJECT *objects_find_by_sa_object(void *sa_object)
{
	int i;
	struct OBJECT *objects;

	if (active_layer != NULL) {
		objects = active_layer->objects;
		for (i = active_layer->numobjects - 1; i >= 0; i--) {
			if (objects[i].sa_object == sa_object) {
				return objects + i;
			}
		}
	}
	return NULL;
}

void objects_cb_rb_save_new(struct REMOVEDBUILDING *remove)
{
	if (remove != NULL) {
		active_layer->removes[active_layer->numremoves++] = *remove;
		rbui_refresh_list();
	}
}

void objects_clone_object(struct CEntity *entity)
{
	struct RwV3D pos;

	if (cloning_object.model == 0) {
		cloning_object.model = entity->model;
		game_ObjectGetPos(entity, &pos);
		game_ObjectGetRot(entity, &cloning_object_rot);
		objbase_mkobject(&cloning_object, &pos);
	}
}

void objects_show_delete_confirm_msg(msgboxcb *cb)
{
	msg_title = "Object";
	msg_message = "Delete_object?";
	msg_message2 = "This_cannot_be_undone!";
	msg_btn1text = "Yes";
	msg_btn2text = "No";
	msg_show(cb);
}

void objects_delete_obj(struct OBJECT *obj)
{
	struct MSG_NC nc;
	int idx;

	nc._parent.id = MAPEDIT_MSG_NATIVECALL;
	nc._parent.data = 0;
	nc.nc = NC_DestroyObject;
	nc.params.asint[1] = obj->samp_objectid;
	sockets_send(&nc, sizeof(nc));

	idx = 0;
	for (;;) {
		if (active_layer->objects + idx == obj) {
			break;
		}
		idx++;
		if (idx >= MAX_OBJECTS) {
			return;
		}
	}

	active_layer->numobjects--;
	if (active_layer->numobjects > 0) {
		active_layer->objects[idx] =
			active_layer->objects[active_layer->numobjects];
	}
}
