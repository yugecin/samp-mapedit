/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "msgbox.h"
#include "ui.h"
#include "objbase.h"
#include "objects.h"
#include "sockets.h"
#include "../shared/serverlink.h"
#include <string.h>
#include <windows.h>

static struct UI_WINDOW *window_layers;
static struct UI_LABEL *lbl_layer;

static struct UI_BUTTON *btn_mainmenu_layers;
static struct UI_BUTTON *btn_contextmenu_mkobject;
static struct UI_LIST *lst_layers;
static struct UI_INPUT *in_layername;

static struct UI_WINDOW *window_objinfo;
static struct UI_LABEL *lbl_objentity;
static struct UI_LABEL *lbl_objflags;
static struct UI_BUTTON *btn_remove_building;
static struct UI_BUTTON *btn_move_obj_samp;
static struct UI_BUTTON *btn_move_obj_click;

static char txt_objentity[9];
static char txt_objtype[9];
static char txt_objflags[9];
static char txt_objlodentity[9];
static char txt_objlodflags[9];

static struct OBJECT *selected_object;
struct OBJECTLAYER *active_layer = NULL;
static struct OBJECTLAYER layers[MAX_LAYERS];
static int activelayeridx = 0;
static int numlayers = 0;
static struct RwV3D nextObjectPosition;
static struct RwV3D player_pos_before_selecting;
static int is_selecting_object = 0;

static
void cb_msg_mkobject_needlayer(int choice)
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
		msg_show(cb_msg_mkobject_needlayer);
		return;
	}

	if (active_layer->numobjects == MAX_OBJECTS) {
		msg_message = "Layer_object_limit_reached.";
		msg_title = "Objects";
		msg_btn1text = "Ok";
		msg_show(NULL);
		return;
	}

	objbase_mkobject(active_layer, 3279, &nextObjectPosition);
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
void cb_show_objinfo_window()
{
	ui_show_window(window_objinfo);
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
		strcpy(layers[numlayers].name, "new_layer");
		numlayers++;
		update_layer_list();
		layer_activate(numlayers - 1);
	}
}

static
void cb_delete_layer_confirm(int choice)
{
	int idx;

	idx = lst_layers->selectedindex;
	if (choice == MSGBOX_RESULT_1 && 0 <= idx && idx < numlayers) {
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
}

static
void cb_btn_move_obj_samp(struct UI_BUTTON *btn)
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
void cb_btn_move_obj_click(struct UI_BUTTON *btn)
{
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
	ui_wnd_add_child(window_objinfo, ui_lbl_make("Type:"));
	ui_wnd_add_child(window_objinfo, ui_lbl_make(txt_objtype));
	ui_wnd_add_child(window_objinfo, ui_lbl_make("Flags:"));
	ui_wnd_add_child(window_objinfo, ui_lbl_make(txt_objflags));
	ui_wnd_add_child(window_objinfo, ui_lbl_make("LOD_Entity:"));
	ui_wnd_add_child(window_objinfo, ui_lbl_make(txt_objlodentity));
	ui_wnd_add_child(window_objinfo, ui_lbl_make("LOD_Flags:"));
	ui_wnd_add_child(window_objinfo, ui_lbl_make(txt_objlodflags));
	btn = ui_btn_make("Remove_Building", cb_btn_remove_building);
	btn->_parent.span = 2;
	btn->enabled = 0;
	ui_wnd_add_child(window_objinfo, btn_remove_building = btn);
	btn = ui_btn_make("Move_Object_(samp)", cb_btn_move_obj_samp);
	btn->_parent.span = 2;
	btn->enabled = 0;
	ui_wnd_add_child(window_objinfo, btn_move_obj_samp = btn);
	btn = ui_btn_make("Move_Object_(click)", cb_btn_move_obj_click);
	btn->_parent.span = 2;
	btn->enabled = 0;
	ui_wnd_add_child(window_objinfo, btn_move_obj_click = btn);

	selected_object = NULL;
}

void objects_dispose()
{
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
	}
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

	numlayers = 0;
	active_layer = NULL;
	activelayeridx = 0;
	lbl_layer->text = "<none>";
	ui_lbl_recalc_size(lbl_layer);
	ui_in_set_text(in_layername, "");
}

void objects_prj_postload()
{
	update_layer_list();
	btn_contextmenu_mkobject->enabled = 1;
	btn_mainmenu_layers->enabled = 1;
}

static
void objects_select_entity(void *entity)
{
	void *lod;

	objbase_select_entity(entity);
	if (entity != NULL) {
		lod = *((int**) ((char*) entity + 0x30));
		sprintf(txt_objentity, "%p", entity);
		sprintf(txt_objtype, "%d", (int) *((char*) entity + 0x36));
		sprintf(txt_objflags, "%p", *((int*) entity + 7));
		if ((int) lod == -1 || lod == NULL) {
			strcpy(txt_objlodentity, "00000000");
			strcpy(txt_objlodflags, "00000000");
		} else {
			sprintf(txt_objlodentity, "%p", (int) lod);
			sprintf(txt_objlodflags, "%p", *((int*) lod + 7));
		}
		selected_object = objects_find_by_sa_object(entity);
		if (selected_object == NULL) {
			btn_remove_building->enabled = 1;
			btn_move_obj_samp->enabled = 0;
			btn_move_obj_click->enabled = 0;
		} else {
			btn_remove_building->enabled = 0;
			btn_move_obj_samp->enabled = 1;
			btn_move_obj_click->enabled = 1;
		}
	} else {
		selected_object = NULL;
		btn_remove_building->enabled = 0;
		btn_move_obj_samp->enabled = 0;
		btn_move_obj_click->enabled = 0;
		strcpy(txt_objentity, "00000000");
		strcpy(txt_objtype, "00000000");
		strcpy(txt_objflags, "00000000");
		strcpy(txt_objlodentity, "00000000");
		strcpy(txt_objlodflags, "00000000");
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
	float rot;
	struct RwV3D *pos;

	game_EntitySetAlpha(player, 0);
	game_PedGetPos(player, &pos, &rot);
	player_pos_before_selecting = *pos;
}

static
void objects_restore_selecting_object()
{
	game_EntitySetAlpha(player, 255);
	game_PedSetPos(player, &player_pos_before_selecting);
}

void objects_on_active_window_changed(struct UI_WINDOW *wnd)
{
	if (wnd == window_objinfo) {
		is_selecting_object = 1;
		objects_prepare_selecting_object();
		btn_remove_building->enabled = 0;
		btn_move_obj_samp->enabled = 0;
		btn_move_obj_click->enabled = 0;
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
