/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "msgbox.h"
#include "ui.h"
#include "objbrowser.h"
#include "objbase.h"
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

static int isactive = 0;
static int hasvalidobject = 0;
static struct RwV3D originalCameraPos, originalCameraRot;
static struct OBJECT picking_object;
static struct RwV3D *positionToCommit;
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
	btn_create->enabled = 0;
	btn_prev->enabled = btn_next->enabled = 0;
	objbase_mkobject(&picking_object, &positionToPreview);
}

static void destroy_object()
{
	struct MSG_NC nc;

	hasvalidobject = 0;
	nc._parent.id = MAPEDIT_MSG_NATIVECALL;
	nc._parent.data = 0;
	nc.nc = NC_DestroyObject;
	nc.params.asint[1] = picking_object.samp_objectid;
	sockets_send(&nc, sizeof(nc));
}

static
void recreate_object()
{
	destroy_object();
	create_object();
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
		objbase_set_entity_to_render_exclusively(entity);
		colmodel = game_EntityGetColModel(entity);
		if (colmodel != NULL) {
			pos = positionToPreview;
			pos.z -= (colmodel->max.z + colmodel->min.z) / 2.0f;
			game_ObjectSetPos(entity, &pos);
		}
		rotationStartTime = *timeInGame;
		hasvalidobject = 1;
		manual_rotation_x = 0.0f;
		manual_rotation_z = 0.0f;
		manual_rotation_base_x = 0.0f;
		manual_rotation_base_z = 0.0f;
		manual_rotate = 1;
		objbrowser_update_camera(); /*for function below to work*/
		objbrowser_try_find_optimal_camera_distance(colmodel);
		manual_rotate = 0;
		manual_rotation_x = M_PI - atanf(1.0f / DEFAULT_ANGLE_RATIO);
		objbrowser_update_camera();
		return 1;
	}
	return 0;
}

static
void cb_btn_prev_model(struct UI_BUTTON *btn)
{
	if (picking_object.model > 0) {
		picking_object.model--;
	}
	recreate_object();
}

static
void cb_btn_next_model(struct UI_BUTTON *btn)
{
	picking_object.model++;
	recreate_object();
}

static
void objbrowser_do_ui()
{
	struct RwV3D rot;
	int ignore;

	game_PedSetPos(player, &player_position);

	ignore = ui_element_being_clicked != NULL;
	ui_do_exclusive_mode_basics(wnd);
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
	camera->position = originalCameraPos;
	camera->lookVector = originalCameraRot;
	ui_update_camera();
	objbase_set_entity_to_render_exclusively(NULL);
	ui_hide_window();
	samp_restore_ui_f7();
	isactive = 0;
	hasvalidobject = 0;
	timeweather_resync();
	ui_set_trapped_in_ui(0);
	ui_exclusive_mode = NULL;
	*cloud_render_opcode = cloud_render_original_opcode;
}

static
void cb_btn_create(struct UI_BUTTON *btn)
{
	struct OBJECT *object;

	object = active_layer->objects + active_layer->numobjects++;
	memcpy(object, &picking_object, sizeof(struct OBJECT));
	picking_object.model = 0;
	game_ObjectSetPos(object->sa_object, positionToCommit);
	restore_after_hide();
}

static
void cb_btn_cancel()
{
	destroy_object();
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
	positionToCommit = positionToCreate;
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
	ui_set_trapped_in_ui(1);
	ui_exclusive_mode = objbrowser_do_ui;
	cloud_render_original_opcode = *cloud_render_opcode;
	*cloud_render_opcode = 0xC3; /*ret*/
}

void objbrowser_init()
{
	struct UI_BUTTON *btn;
	DWORD oldvp;

	picking_object.model = 3279;
	desired_time = 12;

	cloud_render_opcode = (unsigned char*) 0x716380;
	VirtualProtect(cloud_render_opcode, 1, PAGE_EXECUTE_READWRITE, &oldvp);

	wnd = ui_wnd_make(10.0f, 200.0f, "Object_browser");
	wnd->closeable = 0;
	wnd_original_mousewheel_proc = wnd->_parent._parent.proc_mousewheel;
	wnd->_parent._parent.proc_mousewheel = (void*) objbrowser_on_mousewheel;
	wnd_original_draw_proc = wnd->_parent._parent.proc_draw;
	wnd->_parent._parent.proc_draw = (void*) objbrowser_draw_wnd;

	btn_next = ui_btn_make("Next_model", cb_btn_next_model);
	ui_wnd_add_child(wnd, btn_next);
	btn_prev = ui_btn_make("Previous_model", cb_btn_prev_model);
	ui_wnd_add_child(wnd, btn_prev);
	btn = ui_btn_make("Switch_day/night", cb_btn_switchtime);
	ui_wnd_add_child(wnd, btn);
	btn_create = ui_btn_make("Create", cb_btn_create);
	ui_wnd_add_child(wnd, btn_create);
	ui_wnd_add_child(wnd, ui_btn_make("Cancel", (void*) cb_btn_cancel));
}

void objbrowser_dispose()
{
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
