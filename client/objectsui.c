/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "entity.h"
#include "game.h"
#include "ide.h"
#include "msgbox.h"
#include "ui.h"
#include "objects.h"
#include "bulkedit.h"
#include "objectsui.h"
#include "objbrowser.h"
#include "objectseditor.h"
#include "objectstorage.h"
#include "persistence.h"
#include "player.h"
#include "project.h"
#include "removedbuildings.h"
#include "removebuildingeditor.h"
#include "removedbuildingsui.h"
#include "sockets.h"
#include "vehnames.h"
#include "../shared/serverlink.h"
#include <string.h>
#include <stdio.h>
#include <windows.h>

static struct UI_WINDOW *window_layers;
static struct UI_LABEL *lbl_layer;

static struct UI_BUTTON *btn_mainmenu_layers, *btn_mainmenu_selectobject;
static struct UI_BUTTON *btn_contextmenu_mkobject;
static struct UI_BUTTON *btn_contextmenu_editobject;
static struct UI_BUTTON *btn_contextmenu_moveobject;
static struct UI_LABEL *lbl_contextmenu_model;
static struct UI_BUTTON *btn_add_layer;
static struct UI_LABEL *lbl_layers_info;
static struct UI_BUTTON *btn_layer_activate[MAX_LAYERS];
static struct UI_BUTTON *btn_layer_delete[MAX_LAYERS];
static struct UI_INPUT *in_layer_name[MAX_LAYERS];
static struct UI_CHECKBOX *chk_layer_visible[MAX_LAYERS];
static struct UI_INPUT *in_layer_radin[MAX_LAYERS];
static struct UI_INPUT *in_layer_radout[MAX_LAYERS];
static struct UI_LABEL *lbl_layer_removes[MAX_LAYERS];
static struct UI_LABEL *lbl_layer_objects[MAX_LAYERS];
static struct UI_LABEL *lbl_layer_models[MAX_LAYERS];
static char txt_layer_removes[MAX_LAYERS][8];
static char txt_layer_objects[MAX_LAYERS][8];
static char txt_layer_models[MAX_LAYERS][8];
static char txt_lbl_layers_info[100];

static struct UI_WINDOW *window_objinfo;
static struct UI_LABEL *lbl_objentity;
static struct UI_LABEL *lbl_objflags;
static struct UI_LABEL *lbl_objmodel;
static struct UI_LABEL *lbl_objlodmodel;
static struct UI_BUTTON *btn_view_in_browser;
static struct UI_BUTTON *btn_remove_building;
static struct UI_BUTTON *btn_objclone;
static struct UI_BUTTON *btn_edit_obj;
static struct UI_BUTTON *btn_delete_obj;

static char txt_contextmenu_model[50];
static char txt_contextmenu_objsize[50];
static char txt_objentity[9];
static char txt_objmodel[45];
static char txt_modelsize[20];
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
void cb_btn_contextmenu_mkobject(struct UI_BUTTON *btn)
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
void cb_btn_contextmenu_editobject(struct UI_BUTTON *btn)
{
	objedit_show(objects_find_by_sa_object(clicked_entity));
}

static
void cb_btn_contextmenu_moveobject(struct UI_BUTTON *btn)
{
	objedit_show(objects_find_by_sa_object(clicked_entity));
	objedit_move();
}

static
void cb_btn_contextmenu_disbandbulk(struct UI_BUTTON *btn)
{
	bulkedit_reset();
}

static
void update_layer_info_text()
{
	int total_objects;
	int active_objects;
	int model;
	int total_object_models;
	int active_object_models;
	int model_usage_total[20000];
	int model_usage_active[20000];
	int i, j;

	memset(model_usage_total, 0, sizeof(model_usage_total));
	memset(model_usage_active, 0, sizeof(model_usage_active));
	total_objects = 0;
	active_objects = 0;
	total_object_models = 0;
	active_object_models = 0;
	for (i = 0; i < numlayers; i++) {
		total_objects += layers[i].numobjects;
		if (layers[i].show) {
			active_objects += layers[i].numobjects;
		}
		for (j = 0; j < layers[i].numobjects; j++) {
			model = layers[i].objects[j].model;
			if (!model_usage_total[model]) {
				model_usage_total[model] = 1;
				total_object_models++;
			}
			if (layers[i].show) {
				if (!model_usage_active[model]) {
					model_usage_active[model] = 1;
					active_object_models++;
				}
			}
		}
	}
	sprintf(txt_lbl_layers_info,
		"total_objects:_%d_(active:_%d),_models:_%d_(active:_%d)",
		total_objects,
		active_objects,
		total_object_models,
		active_object_models);
}

static void update_layer_ui();
static struct OBJECTLAYER *layertodelete;

static
void cb_delete_layer_confirm(int choice)
{
	if (choice == MSGBOX_RESULT_1) {
		objects_delete_layer(layertodelete);
		update_layer_ui();
		if (active_layer == NULL) {
			lbl_layer->text = "<none>";
		}
	}
	ui_show_window(window_layers);
}

static
void cb_btn_layer_delete(struct UI_BUTTON *btn)
{
	layertodelete = layers + (int) btn->_parent.userdata;
	msg_message = "Delete_layer?";
	msg_message2 = "All_Objects_in_the_layer_will_be_deleted!";
	msg_title = "Delete_Layer";
	msg_btn1text = "Yes";
	msg_btn2text = "No";
	msg_show(cb_delete_layer_confirm);
}

static
void cb_btn_layer_activate(struct UI_BUTTON *btn)
{
	objects_activate_layer((int) btn->_parent.userdata);
}

static
void cb_in_layername(struct UI_INPUT *in)
{
	struct OBJECTLAYER *layer;
	int i;
	char c;

	if ((int) in->_parent.userdata >= numlayers) {
		return;
	}
	layer = layers + (int) in->_parent.userdata;
	for (i = 0; i < sizeof(layer->name); i++) {
		c = in->value[i];
		if (c == ' ') {
			c = '_';
		}
		layer->name[i] = c;
		if (c == 0) {
			break;
		}
	}
	ensure_layer_name_unique(layer);
	ui_lbl_recalc_size(lbl_layer);
}

static
void cb_chk_layer_show(struct UI_CHECKBOX *chk)
{
	struct OBJECTLAYER *layer;
	int numobjects;
	struct OBJECT *objects;
	int i;

	layer = layers + (int) chk->_parent._parent.userdata;
	if (chk->checked) {
		numobjects = layer->numobjects;
		objects = layer->objects;
		for (i = 0; i < numobjects; i++) {
			if (objects[i].samp_objectid == -1) {
				objects_mkobject(objects + i);
			}
		}
		layer->show = 1;
	} else {
		objects_layer_destroy_objects(layer);
		layer->show = 0;
	}
	update_layer_info_text();
}

static
void cb_in_radin(struct UI_INPUT *in)
{
	layers[(int) in->_parent.userdata].stream_in_radius = (float) atof(in->value);
}

static
void cb_in_radout(struct UI_INPUT *in)
{
	layers[(int) in->_parent.userdata].stream_out_radius = (float) atof(in->value);
}

static
void update_layer_ui()
{
	int i, j;
	int model;
	int object_models;
	int model_usage[20000];
	char buf[20];

	ui_wnd_remove_child(window_layers, btn_add_layer);
	ui_wnd_remove_child(window_layers, lbl_layers_info);

	for (i = 0; i < MAX_LAYERS; i++) {
		if (btn_layer_delete[i]) {
			ui_wnd_remove_child(window_layers, btn_layer_delete[i]);
			UIPROC(btn_layer_delete[i], proc_dispose);
			btn_layer_delete[i] = NULL;
		}
		if (btn_layer_activate[i]) {
			ui_wnd_remove_child(window_layers, btn_layer_activate[i]);
			UIPROC(btn_layer_activate[i], proc_dispose);
			btn_layer_activate[i] = NULL;
		}
		if (in_layer_name[i]) {
			ui_wnd_remove_child(window_layers, in_layer_name[i]);
			UIPROC(in_layer_name[i], proc_dispose);
			in_layer_name[i] = NULL;
		}
		if (chk_layer_visible[i]) {
			ui_wnd_remove_child(window_layers, chk_layer_visible[i]);
			UIPROC(chk_layer_visible[i], proc_dispose);
			chk_layer_visible[i] = NULL;
		}
		if (in_layer_radin[i]) {
			ui_wnd_remove_child(window_layers, in_layer_radin[i]);
			UIPROC(in_layer_radin[i], proc_dispose);
			in_layer_radin[i] = NULL;
		}
		if (in_layer_radout[i]) {
			ui_wnd_remove_child(window_layers, in_layer_radout[i]);
			UIPROC(in_layer_radout[i], proc_dispose);
			in_layer_radout[i] = NULL;
		}
		if (lbl_layer_removes[i]) {
			ui_wnd_remove_child(window_layers, lbl_layer_removes[i]);
			UIPROC(lbl_layer_removes[i], proc_dispose);
			lbl_layer_removes[i] = NULL;
		}
		if (lbl_layer_objects[i]) {
			ui_wnd_remove_child(window_layers, lbl_layer_objects[i]);
			UIPROC(lbl_layer_objects[i], proc_dispose);
			lbl_layer_objects[i] = NULL;
		}
		if (lbl_layer_models[i]) {
			ui_wnd_remove_child(window_layers, lbl_layer_models[i]);
			UIPROC(lbl_layer_models[i], proc_dispose);
			lbl_layer_models[i] = NULL;
		}
	}

	for (i = 0; i < numlayers; i++) {
		if (!btn_layer_delete[i]) {
			btn_layer_delete[i] = ui_btn_make("X", cb_btn_layer_delete);
			btn_layer_delete[i]->_parent.userdata = (void*) i;
			ui_wnd_add_child(window_layers, btn_layer_delete[i]);
		}
		if (!btn_layer_activate[i]) {
			btn_layer_activate[i] = ui_btn_make("set_active", cb_btn_layer_activate);
			btn_layer_activate[i]->_parent.userdata = (void*) i;
			ui_wnd_add_child(window_layers, btn_layer_activate[i]);
		}
		if (!in_layer_name[i]) {
			in_layer_name[i] = ui_in_make(cb_in_layername);
			in_layer_name[i]->_parent.userdata = (void*) i;
			ui_wnd_add_child(window_layers, in_layer_name[i]);
		}
		if (!chk_layer_visible[i]) {
			chk_layer_visible[i] = ui_chk_make("Show_objs", layers[i].show, cb_chk_layer_show);
			chk_layer_visible[i]->_parent._parent.userdata = (void*) i;
			ui_wnd_add_child(window_layers, chk_layer_visible[i]);
		}
		if (!in_layer_radin[i]) {
			in_layer_radin[i] = ui_in_make(cb_in_radin);
			in_layer_radin[i]->_parent.userdata = (void*) i;
			in_layer_radin[i]->_parent.pref_width = 80.0f;
			ui_wnd_add_child(window_layers, in_layer_radin[i]);
		}
		if (!in_layer_radout[i]) {
			in_layer_radout[i] = ui_in_make(cb_in_radout);
			in_layer_radout[i]->_parent.userdata = (void*) i;
			in_layer_radout[i]->_parent.pref_width = 80.0f;
			ui_wnd_add_child(window_layers, in_layer_radout[i]);
		}
		if (!lbl_layer_removes[i]) {
			lbl_layer_removes[i] = ui_lbl_make(txt_layer_removes[i]);
			ui_wnd_add_child(window_layers, lbl_layer_removes[i]);
		}
		if (!lbl_layer_objects[i]) {
			lbl_layer_objects[i] = ui_lbl_make(txt_layer_objects[i]);
			ui_wnd_add_child(window_layers, lbl_layer_objects[i]);
		}
		if (!lbl_layer_models[i]) {
			lbl_layer_models[i] = ui_lbl_make(txt_layer_models[i]);
			ui_wnd_add_child(window_layers, lbl_layer_models[i]);
		}

		memset(model_usage, 0, sizeof(model_usage));
		object_models = 0;
		for (j = 0; j < layers[i].numobjects; j++) {
			model = layers[i].objects[j].model;
			if (!model_usage[model]) {
				model_usage[model]++;
				object_models++;
			}
		}
		sprintf(txt_layer_removes[i], "%d", layers[i].numremoves);
		sprintf(txt_layer_objects[i], "%d", layers[i].numobjects);
		sprintf(txt_layer_models[i], "%d", object_models);
		ui_in_set_text(in_layer_name[i], layers[i].name);
		sprintf(buf, "%d", (int) layers[i].stream_in_radius);
		ui_in_set_text(in_layer_radin[i], buf);
		sprintf(buf, "%d", (int) layers[i].stream_out_radius);
		ui_in_set_text(in_layer_radout[i], buf);
	}
	update_layer_info_text();

	ui_wnd_add_child(window_layers, lbl_layers_info);
	ui_wnd_add_child(window_layers, btn_add_layer);
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
		layers[numlayers].show = 1;
		layers[numlayers].stream_in_radius = 500.0f;
		layers[numlayers].stream_out_radius = 600.0f;
		strcpy(layers[numlayers].name, "new_layer");
		ensure_layer_name_unique(layers + numlayers);
		numlayers++;
		update_layer_ui();
		objects_activate_layer(numlayers - 1);
	}
}

static
void cb_btn_view_in_object_browser(struct UI_BUTTON *btn)
{
	struct RwV3D pos;

	if (selected_entity) {
		game_ObjectGetPos(selected_entity, &pos);
		objbrowser_highlight_model(selected_entity->model);
		ui_hide_window(); /*clears selected_entity*/
		objbrowser_show(&pos);
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
void cb_btn_edit_obj(struct UI_BUTTON *btn)
{
	objedit_show(selected_object);
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

	memset(&selected_colored, 0, sizeof(selected_colored));
	selected_object = NULL;

	/*context menu entries*/
	txt_contextmenu_objsize[0] = txt_contextmenu_model[0] = '_';
	txt_contextmenu_objsize[1] = txt_contextmenu_model[1] = 0;
	ui_wnd_add_child(context_menu, lbl_contextmenu_model = ui_lbl_make(txt_contextmenu_model));
	ui_wnd_add_child(context_menu, ui_lbl_make(txt_contextmenu_objsize));
	btn = ui_btn_make("Make_Object", cb_btn_contextmenu_mkobject);
	ui_wnd_add_child(context_menu, btn);
	btn->enabled = 0;
	btn_contextmenu_mkobject = btn;
	btn = ui_btn_make("Edit_Object", cb_btn_contextmenu_editobject);
	ui_wnd_add_child(context_menu, btn);
	btn->enabled = 0;
	btn_contextmenu_editobject = btn;
	btn = ui_btn_make("Move_Object", cb_btn_contextmenu_moveobject);
	ui_wnd_add_child(context_menu, btn);
	btn->enabled = 0;
	btn_contextmenu_moveobject = btn;

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
	window_layers->columns = 9;
	window_layers->proc_shown = (ui_method*) update_layer_ui;

	ui_wnd_add_child(window_layers, ui_lbl_make("Del"));
	ui_wnd_add_child(window_layers, ui_lbl_make("Actiate"));
	ui_wnd_add_child(window_layers, ui_lbl_make("Name"));
	ui_wnd_add_child(window_layers, ui_lbl_make("Show"));
	ui_wnd_add_child(window_layers, ui_lbl_make("radin"));
	ui_wnd_add_child(window_layers, ui_lbl_make("radout"));
	ui_wnd_add_child(window_layers, ui_lbl_make("Removes"));
	ui_wnd_add_child(window_layers, ui_lbl_make("Objects"));
	ui_wnd_add_child(window_layers, ui_lbl_make("Models"));
	lbl_layers_info = ui_lbl_make(txt_lbl_layers_info);
	lbl_layers_info->_parent.span = 9;
	btn_add_layer = ui_btn_make("Add_layer", cb_btn_add_layer);
	btn_add_layer->_parent.span = 9;
	/*colorpicker? dome? drawdistance?*/

	/*objinfo window*/
	window_objinfo = ui_wnd_make(1500.0f, 400.0f, "Entity_Info");
	window_objinfo->columns = 2;

	ui_wnd_add_child(window_objinfo, ui_lbl_make("Entity:"));
	ui_wnd_add_child(window_objinfo, ui_lbl_make(txt_objentity));
	ui_wnd_add_child(window_objinfo, ui_lbl_make("Model:"));
	lbl_objmodel = ui_lbl_make(txt_objmodel);
	ui_wnd_add_child(window_objinfo, lbl_objmodel);
	ui_wnd_add_child(window_objinfo, ui_lbl_make("Size:"));
	ui_wnd_add_child(window_objinfo, ui_lbl_make(txt_modelsize));
	btn = ui_btn_make("View_in_object_browser", cb_btn_view_in_object_browser);
	btn->_parent.span = 2;
	btn->enabled = 0;
	ui_wnd_add_child(window_objinfo, btn_view_in_browser = btn);
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
	ui_wnd_add_child(window_objinfo, btn_remove_building = btn);
	btn = ui_btn_make("Clone", cb_btn_objclone);
	btn->_parent.span = 2;
	ui_wnd_add_child(window_objinfo, btn_objclone = btn);
	btn = ui_btn_make("Edit_Object", cb_btn_edit_obj);
	btn->_parent.span = 2;
	ui_wnd_add_child(window_objinfo, btn_edit_obj = btn);
	btn = ui_btn_make("Delete_Object", cb_btn_delete_obj);
	btn->_parent.span = 2;
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
}

void objui_prj_postload()
{
	update_layer_ui();
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
	struct CColModel *colmodel;

	selected_entity = entity;
	/*remove hovered entity to revert its changes first*/
	entity_color(&hovered_colored, NULL, 0);
	entity_color(&selected_colored, entity, 0xFF0000FF);

	if (entity == NULL) {
		btn_objclone->enabled = 0;
		selected_object = NULL;
		btn_view_in_browser->enabled = 0;
		btn_remove_building->enabled = 0;
		btn_edit_obj->enabled = 0;
		btn_delete_obj->enabled = 0;
		strcpy(txt_objentity, "00000000");
		strcpy(txt_objmodel, "0");
		strcpy(txt_modelsize, "0");
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
	if (modelNames[modelid]) {
		sprintf(txt_objmodel, "%s", modelNames[modelid]);
	} else {
		sprintf(txt_objmodel, "%hd", modelid);
	}
	colmodel = game_EntityGetColModel(entity);
	if (colmodel == NULL) {
		txt_modelsize[0] = '_';
		txt_modelsize[1] = 0;
	} else {
		sprintf(
			txt_modelsize,
			"%.0fx%.0fx%.0f",
			colmodel->max.x - colmodel->min.x,
			colmodel->max.y - colmodel->min.y,
			colmodel->max.z - colmodel->min.z
		);
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
		btn_view_in_browser->enabled = 1;
		btn_remove_building->enabled = 1;
		btn_edit_obj->enabled = 0;
		btn_delete_obj->enabled = 0;
	} else {
		btn_view_in_browser->enabled = 0;
		btn_remove_building->enabled = 0;
		btn_edit_obj->enabled = 1;
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
	struct CEntity *entity;
	struct CColPoint cp;
	struct OBJECT *clicked_object;
	struct CColModel *colmodel;

	if (is_selecting_object) {
		get_object_pointed_at(&entity, &cp);
		objui_select_entity(entity);
		return 0;
	}

	clicked_object = NULL;
	btn_contextmenu_editobject->enabled =
	btn_contextmenu_moveobject->enabled =
		clicked_entity != NULL &&
		(clicked_object = objects_find_by_sa_object(clicked_entity)) != NULL;

	if (clicked_entity != NULL)
	{
		if (ENTITY_IS_TYPE(clicked_entity, ENTITY_TYPE_VEHICLE)) {
			sprintf(txt_contextmenu_model, "%d:_%s",
				clicked_entity->model, vehnames[clicked_entity->model - 400]);
		} else if (ENTITY_IS_TYPE(clicked_entity, ENTITY_TYPE_BUILDING) ||
			ENTITY_IS_TYPE(clicked_entity, ENTITY_TYPE_OBJECT) ||
			ENTITY_IS_TYPE(clicked_entity, ENTITY_TYPE_DUMMY))
		{
			strcpy(txt_contextmenu_model, modelNames[clicked_entity->model]);
		} else {
			goto nothing;
		}

		colmodel = game_EntityGetColModel(clicked_entity);
		if (colmodel == NULL) {
			txt_contextmenu_objsize[0] = '_';
			txt_contextmenu_objsize[1] = 0;
		} else {
			sprintf(
				txt_contextmenu_objsize,
				"%.0fx%.0fx%.0f",
				colmodel->max.x - colmodel->min.x,
				colmodel->max.y - colmodel->min.y,
				colmodel->max.z - colmodel->min.z
			);
		}
	} else {
		txt_contextmenu_objsize[0] = '_';
		txt_contextmenu_objsize[1] = 0;
nothing:
		txt_contextmenu_model[0] = '_';
		txt_contextmenu_model[1] = 0;
	}
	ui_lbl_recalc_size(lbl_contextmenu_model);

	ui_get_clicked_position(&nextObjectPosition);
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
		btn_view_in_browser->enabled = 0;
		btn_remove_building->enabled = 0;
		btn_edit_obj->enabled = 0;
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
