/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "entity.h"
#include "game.h"
#include "ide.h"
#include "msgbox.h"
#include "ui.h"
#include "objects.h"
#include "objectsui.h"
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
static struct RwV3D nextObjectPosition;
static struct RwV3D player_pos_before_selecting;
static int is_selecting_object = 0;

static struct ENTITYCOLORINFO selected_colored, hovered_colored;

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
		objects_activate_layer(i);
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
		objui_show_select_layer_first_msg();
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
		objects_activate_layer(numlayers - 1);
	}
}

static
void cb_delete_layer_confirm(int choice)
{
	struct OBJECTLAYER *layer;
	int idx;

	idx = lst_layers->selectedindex;
	if (choice == MSGBOX_RESULT_1 && 0 <= idx && idx < numlayers) {
		layer = layers + idx;
		objects_delete_layer(layer);
		update_layer_list();
		if (active_layer == NULL) {
			lbl_layer->text = "<none>";
		}
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
		rbe_show_for_entity(entity, objui_cb_rb_save_new);
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
		objects_clone(selected_entity);
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
		objui_select_entity(selected_object->sa_object);
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
	objui_select_entity(object_to_delete_after_confirm->sa_object);
}

static
void cb_btn_delete_obj(struct UI_BUTTON *btn)
{
	if (selected_object != NULL) {
		object_to_delete_after_confirm = selected_object;
		objui_show_delete_confirm_msg(cb_confirm_delete);
	}
}

void objui_init()
{
	struct UI_BUTTON *btn;
	struct UI_LABEL *lbl;
	struct UI_COLORPICKER *cp;

	memset(&selected_colored, 0, sizeof(selected_colored));
	selected_object = NULL;

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
}

void objui_dispose()
{
	entity_color(&selected_colored, NULL, 0);
	entity_color(&hovered_colored, NULL, 0);
	ui_wnd_dispose(window_objinfo);
	ui_wnd_dispose(window_layers);
}

void objui_prj_preload()
{
	lbl_layer->text = "<none>";
	ui_lbl_recalc_size(lbl_layer);
	ui_in_set_text(in_layername, "");
}

void objui_prj_postload()
{
	update_layer_list();
	btn_contextmenu_mkobject->enabled = 1;
	btn_mainmenu_layers->enabled = 1;
	btn_mainmenu_selectobject->enabled = 1;
}

static
void get_object_pointed_at(struct CEntity **entity, struct CColPoint *cp)
{
	struct RwV3D target, from;

	from = camera->position;
	game_ScreenToWorld(&target, cursorx, cursory, 300.0f);
	if (!game_WorldIntersectBuildingObject(&from, &target, cp, entity)) {
		*entity = NULL;
	}
}

static
void objui_do_hover()
{
	struct CColPoint cp;
	struct CEntity *entity;

	TRACE("objui_do_hover");
	if (is_selecting_object) {
		if (ui_is_cursor_hovering_any_window()) {
			entity = NULL;
		} else {
			get_object_pointed_at(&entity, &cp);
			if (entity != NULL) {
				player_position = cp.pos;
			} else {
				game_ScreenToWorld(&player_position,
					cursorx, cursory, 200.0f);
			}
			if (entity == selected_colored.entity) {
				entity = NULL;
			}
		}
		entity_color(&hovered_colored, entity, 0xFFFF00FF);
	}
}

void objui_cb_rb_save_new(struct REMOVEDBUILDING *remove)
{
	if (remove != NULL) {
		active_layer->removes[active_layer->numremoves++] = *remove;
		rbui_refresh_list();
	}
}

void objui_layer_changed()
{
	lbl_layer->text = active_layer->name;
	ui_lbl_recalc_size(lbl_layer);
	ui_in_set_text(in_layername, active_layer->name);
	ui_lst_set_selected_index(lst_layers, active_layer - layers);
}

void objui_frame_update()
{
	if (is_selecting_object) {
		objui_do_hover();
		entity_draw_bound_rect(selected_colored.entity, 0xFF0000);
		entity_draw_bound_rect(hovered_colored.entity, 0xFF00FF);

		game_TextSetAlign(CENTER);
		game_TextPrintString(fresx / 2.0f, 50.0f,
			"~w~Hover over an object and ~r~click~w~ to select it");
	}
}

void objui_select_entity(void *entity)
{
	void *lod;
	unsigned short modelid, lodmodel;

	selected_entity = entity;
	/*remove hovered entity to revert its changes first*/
	entity_color(&hovered_colored, NULL, 0);
	entity_color(&selected_colored, entity, 0xFF0000FF);

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

void objui_on_world_entity_removed(void *entity)
{
	TRACE("objui_on_world_entity_removed");
	if (entity == selected_colored.entity) {
		objui_select_entity(NULL);
	}
	if (entity == hovered_colored.entity) {
		entity_color(&hovered_colored, NULL, 0);
	}
}

int objui_on_background_element_just_clicked()
{
	if (is_selecting_object) {
		objui_select_entity(clicked_entity);
		return 0;
	}

	if (clicked_entity) {
		nextObjectPosition = clicked_colpoint.pos;
	} else {
		game_ScreenToWorld(
			&nextObjectPosition, bgclickx, bgclicky, 60.0f);
	}
	return 1;
}

static
void objui_prepare_selecting_object()
{
	game_EntitySetAlpha(player, 0);
	player_pos_before_selecting = player_position;
	player_position_for_camera = &player_pos_before_selecting;
}

static
void objui_restore_selecting_object()
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
		objui_prepare_selecting_object();
		btn_remove_building->enabled = 0;
		btn_move_obj->enabled = 0;
		btn_tp_obj_to_cam->enabled = 0;
		btn_delete_obj->enabled = 0;
	} else if (is_selecting_object) {
		is_selecting_object = 0;
		objui_restore_selecting_object();
	}
	objui_select_entity(NULL);
}

void objui_ui_activated()
{
	if (is_selecting_object) {
		objui_prepare_selecting_object();
	}
}

void objui_ui_deactivated()
{
	if (is_selecting_object) {
		objui_restore_selecting_object();
	}
}

int objui_handle_esc()
{
	if (is_selecting_object) {
		ui_hide_window();
		return 1;
	}
	return 0;
}

void objui_show_select_layer_first_msg()
{
	msg_message = "Create_and_select_an_object_layer_first!";
	msg_title = "Objects";
	msg_btn1text = "Ok";
	msg_show(cb_msg_openlayers);
}

void objui_show_delete_confirm_msg(msgboxcb *cb)
{
	msg_title = "Object";
	msg_message = "Delete_object?";
	msg_message2 = "This_cannot_be_undone!";
	msg_btn1text = "Yes";
	msg_btn2text = "No";
	msg_show(cb);
}
