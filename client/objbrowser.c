/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "entity.h"
#include "game.h"
#include "ide.h"
#include "msgbox.h"
#include "ui.h"
#include "objbrowser.h"
#include "objects.h"
#include "player.h"
#include "sockets.h"
#include "samp.h"
#include "timeweather.h"
#include "../shared/serverlink.h"
#include <math.h>
#include <string.h>
#include <windows.h>

static struct UI_WINDOW *wnd;
static struct UI_BUTTON *btn_next, *btn_prev;
static struct UI_BUTTON *btn_create;
static struct UI_LABEL *lbl_modelname;
static struct UI_INPUT *in_filter;
static struct UI_LIST *lst_browser;
static char lbltxt_modelid[7], lbltxt_modelname[40];
static int lst_index_to_model_mapping[MAX_MODELS];
static int lst_model_to_index_mapping[MAX_MODELS];
static ui_method *proc_orig_lst_post_layout;

static int isactive = 0;
static int hasvalidobject = 0;
static struct RwV3D originalCameraPos, originalCameraRot;
static struct OBJECT picking_object;
static struct RwV3D positionToCommit;
static struct RwV3D positionToPreview;
static int rotationStartTime;
static float camera_distance;
static ui_method1 *wnd_original_mousewheel_proc;
static ui_method *wnd_original_draw_proc;
static int manual_rotate;
static float manual_rotation_base_x, manual_rotation_base_z;
static float manual_rotation_x, manual_rotation_z;
static float clickx, clicky;
static int desired_time;
static unsigned char *cloud_render_opcode;
static unsigned char cloud_render_original_opcode;
static char blacklistedObjects[MAX_MODELS];

int objbrowser_never_create;
void (*objbrowser_cb)(int);

#define DEFAULT_ANGLE_RATIO 0.2f

static
void objbrowser_update_camera()
{
	float x, y, z, xylen;

	if (manual_rotate) {
		xylen = sinf(manual_rotation_x);
		x = camera_distance * cosf(manual_rotation_z) * xylen;
		y = camera_distance * sinf(manual_rotation_z) * xylen;
		z = camera_distance * cosf(manual_rotation_x);
	} else {
		x = camera_distance;
		y = 0.0f;
		z = -camera_distance * DEFAULT_ANGLE_RATIO;
	}
	camera->position.x = positionToPreview.x - x;
	camera->position.y = positionToPreview.y - y;
	camera->position.z = positionToPreview.z - z;
	camera->lookVector.x = x;
	camera->lookVector.y = y;
	camera->lookVector.z = z;
	ui_update_camera();
}

static
void objbrowser_draw_wnd(void *wnd)
{
	wnd_original_draw_proc(wnd);
	game_TextSetAlign(CENTER);
	game_TextPrintString(fresx / 2.0f, 50.0f,
		"~r~Left-lick_drag~w~_to_rotate,_~r~mousewheel~w~_to_zoom.");
}

static
int objbrowser_on_mousewheel(void *wnd, int value)
{
	if (wnd_original_mousewheel_proc(wnd, (void*) value)) {
		return 1;
	}
	if (isactive) {
		camera_distance -= 3.0f * value;
		if (camera_distance < 1.0f) {
			camera_distance = 1.0f;
		}
		objbrowser_update_camera();
		return 1;
	}
	return 0;
}

struct OBJECT *objbrowser_object_by_handle(int sa_handle)
{
	if (picking_object.sa_handle == sa_handle) {
		return &picking_object;
	}
	return NULL;
}

static
void create_object()
{
	sprintf(lbltxt_modelid, "%d", picking_object.model);
	if (modelNames[picking_object.model] != NULL) {
		strcpy(lbltxt_modelname, modelNames[picking_object.model] + 7);
		ui_lbl_recalc_size(lbl_modelname);
	} else {
		lbltxt_modelname[0] = '?';
		lbltxt_modelname[1] = 0;
	}
	btn_create->enabled = 0;
	btn_prev->enabled = btn_next->enabled = 0;
	picking_object.pos = positionToPreview;
	objects_mkobject(&picking_object);
}

static void destroy_object()
{
	struct MSG_NC nc;

	if (hasvalidobject) {
		hasvalidobject = 0;
		nc._parent.id = MAPEDIT_MSG_NATIVECALL;
		nc.nc = NC_DestroyObject;
		nc.params.asint[1] = picking_object.samp_objectid;
		sockets_send(&nc, sizeof(nc));
	}
}

static
void recreate_object()
{
	destroy_object();
	create_object();
}

static
void cb_force_buttons_enabled(struct UI_BUTTON *btn)
{
	btn_next->enabled = btn_prev->enabled = 1;
}

void objbrowser_try_find_optimal_camera_distance(struct CColModel *colmodel)
{
	struct RwV3D colsize;

	if (colmodel == NULL) {
		camera_distance = 30.0f;
	} else {
		colsize.x = (float) fabs(colmodel->max.x - colmodel->min.x);
		colsize.y = (float) fabs(colmodel->max.y - colmodel->min.y);
		if (colsize.y > colsize.x) {
			colsize.x = colsize.y;
		}
		colsize.z = (float) fabs(colmodel->max.z - colmodel->min.z);
		if (colsize.z > colsize.x) {
			colsize.x = colsize.z;
		}
		camera_distance = colsize.x * 2.0f;
		if (camera_distance < 3.0f) {
			camera_distance = 3.0f;
		}
	}
}

int objbrowser_object_created(struct OBJECT *object)
{
	struct RwV3D pos;
	void *entity;
	struct CColModel *colmodel;

	if (object == &picking_object) {
		if (!isactive) {
			destroy_object();
			return 1;
		}
		btn_create->enabled = 1;
		btn_next->enabled = btn_prev->enabled = 1;
		entity = object->sa_object;
		entity_render_exclusively(entity);
		game_ObjectSetPos(manipulateEntity, &positionToPreview);
		colmodel = game_EntityGetColModel(entity);
		if (colmodel != NULL) {
			pos = positionToPreview;
			pos.z += (colmodel->max.z + colmodel->min.z) / 2.0f;
			game_ObjectSetPos(entity, &pos);
		}
		rotationStartTime = *timeInGame;
		hasvalidobject = 1;
		manual_rotate = 0;
		manual_rotation_x = M_PI - atanf(1.0f / DEFAULT_ANGLE_RATIO);
		manual_rotation_z = 0.0f;
		manual_rotation_base_x = 0.0f;
		manual_rotation_base_z = 0.0f;
		objbrowser_try_find_optimal_camera_distance(colmodel);
		objbrowser_update_camera();
		return 1;
	}
	return 0;
}

static
int list_valid_index_for_model(int model)
{
	int list_index;

	if (modelNames[model]== NULL || blacklistedObjects[model]) {
		return -1;
	}

	list_index = lst_model_to_index_mapping[model];
	if (!ui_lst_is_index_valid(lst_browser, list_index)) {
		return -1;
	}

	return list_index;
}

static
void cb_btn_prev_next_model(struct UI_BUTTON *btn)
{
	int list_index;
	int direction;

	direction = (int) btn->_parent.userdata;
	do {
		picking_object.model += direction;
		if (picking_object.model <= 0) {
			picking_object.model = MAX_MODELS - 1;
		}
		if (picking_object.model >= MAX_MODELS) {
			picking_object.model = 1;
		}
		list_index = list_valid_index_for_model(picking_object.model);
	} while (list_index == -1);
	recreate_object();
	ui_lst_set_selected_index(lst_browser, list_index);
}

static
void objbrowser_do_ui()
{
	struct RwV3D rot;
	int ignore;

	game_PedSetPos(player, &player_position);

	ignore = ui_element_being_clicked != NULL;
	ui_do_exclusive_mode_basics(wnd, 0);
	ignore |= ui_element_being_clicked != NULL;

	if (!hasvalidobject) {
		return;
	}

	rot.y = 0.0f;
	rot.x = 0.0f;
	rot.z = (*timeInGame - rotationStartTime) * 0.00175f;
	if (ui_mouse_is_just_down && !ignore) {
		if (!manual_rotate) {
			manual_rotation_base_x = manual_rotation_x;
			manual_rotation_base_z = manual_rotation_z;
			manual_rotate = 1;
		}
		clickx = cursorx;
		clicky = cursory;
	}
	if (manual_rotate) {
		if ((ui_mouse_is_just_up || activeMouseState->lmb) && !ignore) {
			manual_rotation_x = manual_rotation_base_x;
			manual_rotation_x += (cursory - clicky) * 0.005f;
			manual_rotation_z = manual_rotation_base_z;
			manual_rotation_z -= (cursorx - clickx) * 0.0075f;
			if (manual_rotation_x > M_PI * 0.99f) {
				manual_rotation_x = M_PI * 0.99f;
			} else if (manual_rotation_x < 0.01f) {
				manual_rotation_x = 0.01f;
			}
			if (ui_mouse_is_just_up) {
				manual_rotation_base_x = manual_rotation_x;
				manual_rotation_base_z = manual_rotation_z;
			}
			if (activeMouseState->lmb) {
				objbrowser_update_camera();
			}
		}
	} else {
		game_ObjectSetRotRad(picking_object.sa_object, &rot);
	}
}

static
void restore_after_hide()
{
	destroy_object();
	camera->position = originalCameraPos;
	camera->lookVector = originalCameraRot;
	ui_update_camera();
	entity_render_exclusively(NULL);
	ui_hide_window();
	samp_restore_ui_f7();
	isactive = 0;
	hasvalidobject = 0;
	timeweather_resync();
	ui_exclusive_mode = NULL;
	*cloud_render_opcode = cloud_render_original_opcode;
}

static
void cb_btn_create(struct UI_BUTTON *btn)
{
	struct OBJECT *object;

	if (!objbrowser_never_create) {
		object = active_layer->objects + active_layer->numobjects++;
		memcpy(object, &picking_object, sizeof(struct OBJECT));
		object->pos = positionToCommit;
		memset(&object->rot, 0, sizeof(struct RwV3D));
		objects_mkobject(object);
	}
	restore_after_hide();
	if (objbrowser_cb) {
		objbrowser_cb(picking_object.model);
	}
}

static
void cb_btn_cancel()
{
	restore_after_hide();
}

static
void cb_btn_switchtime(struct UI_BUTTON *btn)
{
	if (desired_time == 1) {
		desired_time = 12;
	} else {
		desired_time = 1;
	}
	timeweather_set_time(desired_time);
}

void objbrowser_show(struct RwV3D *positionToCreate)
{
	objbrowser_never_create = 0;
	objbrowser_cb = NULL;
	lbltxt_modelid[0] = lbltxt_modelname[0] = 0;
	positionToCommit = *positionToCreate;
	originalCameraPos = camera->position;
	originalCameraRot = camera->lookVector;
	positionToPreview.x = camera->position.x;
	positionToPreview.y = camera->position.y;
	positionToPreview.z = 560.0f;
	objbrowser_update_camera();
	create_object();
	ui_show_window(wnd);
	samp_hide_ui_f7();
	isactive = 1;
	timeweather_set_time(desired_time);
	timeweather_set_weather(17); /*DE extra sunny*/
	ui_exclusive_mode = objbrowser_do_ui;
	cloud_render_original_opcode = *cloud_render_opcode;
	*cloud_render_opcode = 0xC3; /*ret*/
}

void objbrowser_highlight_model(int model)
{
	int idx;

	ui_in_set_text(in_filter, "");
	ui_lst_recalculate_filter(lst_browser);
	idx = list_valid_index_for_model(model);
	if (idx != -1) {
		ui_lst_set_selected_index(lst_browser, idx);
		picking_object.model = model;
	}
}

static
void cb_lst_object_selected(struct UI_LIST *lst)
{
	int idx;

	idx = ui_lst_get_selected_index(lst);
	if (idx != -1) {
		picking_object.model = lst_index_to_model_mapping[idx];
		recreate_object();
	}
}

static
void objbrowser_init_blacklist()
{
	memset(blacklistedObjects, 0, sizeof(blacklistedObjects));
	/*Train cross barrier pole. Shows error in SA:MP chatbox when deleted*/
	/*1374 is the barrier itself, it seems to move but not produce errors.*/
	blacklistedObjects[1373] = 1;
	/*Various crane elements that spawn a rope.*/
	/*When deleting the object, the game will crash if the rope is not
	manually deleted. This is done in the mapeditor, but when actually
	exporting and using it on a live server, clients will crash.*/
	blacklistedObjects[1382] = 1; /*dock crane*/
	blacklistedObjects[1385] = 1; /*tower crane movable element*/
	blacklistedObjects[16329] = 1; /*tower crane movable element*/
	/*Crashes (caught by SA:MP) when the object is being created.*/
	blacklistedObjects[3118] = 1; /*imy_shash_LOD*/
}

static
void objbrowser_set_list_data()
{
	char *names[MAX_MODELS];
	int numnames;
	int i;

	TRACE("objbrowser_set_list_data");
	numnames = 0;
	for (i = 0; i < 20000; i++) {
		if (modelNames[i] != NULL && !blacklistedObjects[i]) {
			lst_index_to_model_mapping[numnames] = i;
			lst_model_to_index_mapping[i] = numnames;
			names[numnames] = modelNames[i];
			numnames++;
			continue;
		}
		lst_model_to_index_mapping[i] = -1;
	}
	ui_lst_set_data(lst_browser, names, numnames);
}

static
int objbrowser_lst_post_layout(struct UI_LIST *lst)
{
	int list_index, result;

	/*need to do original post_layout first, so the height is set, so the
	set_selected_index will scroll the selected index to the middle of the
	list's viewport*/
	result = proc_orig_lst_post_layout(lst);
	list_index = lst_model_to_index_mapping[picking_object.model];
	ui_lst_set_selected_index(lst_browser, list_index);
	lst->_parent.proc_post_layout = proc_orig_lst_post_layout;
	return result;
}

static
void cb_in_filter_updated(struct UI_INPUT *in)
{
	ui_lst_recalculate_filter(lst_browser);
	if (lst_browser->numitems == 1) {
		ui_lst_set_selected_index(lst_browser, lst_browser->filteredIndexMapping[0]);
		cb_lst_object_selected(lst_browser);
	}
}

static ui_method *proc_lst_browser_recalc_size;

static
void lst_browser_recalc_size(struct UI_LIST *lst)
{
	proc_lst_browser_recalc_size(lst);
	lst->_parent.pref_width = 650.0f * font_size_x;
}

void objbrowser_init()
{
	struct UI_BUTTON *btn;
	DWORD oldvp;

	picking_object.model = 3279;
	memset(&picking_object.rot, 0, sizeof(struct RwV3D));
	desired_time = 12;

	objbrowser_init_blacklist();

	cloud_render_opcode = (unsigned char*) 0x716380;
	VirtualProtect(cloud_render_opcode, 1, PAGE_EXECUTE_READWRITE, &oldvp);

	wnd = ui_wnd_make(10.0f, 200.0f, "Object_browser");
	wnd->closeable = 0;
	wnd_original_mousewheel_proc = wnd->_parent._parent.proc_mousewheel;
	wnd->_parent._parent.proc_mousewheel = (void*) objbrowser_on_mousewheel;
	wnd_original_draw_proc = wnd->_parent._parent.proc_draw;
	wnd->_parent._parent.proc_draw = (void*) objbrowser_draw_wnd;

	ui_wnd_add_child(wnd, ui_lbl_make(lbltxt_modelid));
	ui_wnd_add_child(wnd, lbl_modelname = ui_lbl_make(lbltxt_modelname));
	btn_next = ui_btn_make("Next_model", cb_btn_prev_next_model);
	btn_next->_parent.userdata = (void*) 1;
	ui_wnd_add_child(wnd, btn_next);
	btn_prev = ui_btn_make("Previous_model", cb_btn_prev_next_model);
	btn_prev->_parent.userdata = (void*) -1;
	ui_wnd_add_child(wnd, btn_prev);
	btn = ui_btn_make("Help_UI_is_stuck", cb_force_buttons_enabled);
	ui_wnd_add_child(wnd, btn);
	btn = ui_btn_make("Switch_day/night", cb_btn_switchtime);
	ui_wnd_add_child(wnd, btn);
	btn_create = ui_btn_make("Create", cb_btn_create);
	ui_wnd_add_child(wnd, btn_create);
	ui_wnd_add_child(wnd, ui_btn_make("Cancel", (void*) cb_btn_cancel));
	in_filter = ui_in_make(cb_in_filter_updated);
	ui_wnd_add_child(wnd, in_filter);
	lst_browser = ui_lst_make(35, cb_lst_object_selected);
	proc_lst_browser_recalc_size = lst_browser->_parent.proc_recalc_size;
	lst_browser->_parent.proc_recalc_size = (void*) lst_browser_recalc_size;
	proc_orig_lst_post_layout = lst_browser->_parent.proc_post_layout;
	lst_browser->_parent.proc_post_layout = objbrowser_lst_post_layout;
	ui_wnd_add_child(wnd, lst_browser);
	lst_browser->filter = in_filter->value;

	objbrowser_set_list_data();
}

void objbrowser_dispose()
{
	TRACE("objbrowser_dispose");
	ui_wnd_dispose(wnd);
}

int objbrowser_handle_esc()
{
	if (isactive) {
		cb_btn_cancel();
		return 1;
	}
	return 0;
}
