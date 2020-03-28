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
#include <string.h>
#include <windows.h>

static struct UI_WINDOW *wnd;
static struct UI_BUTTON *btn_next, *btn_prev;
static struct UI_BUTTON *btn_create, *btn_cancel;

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

static
void objbrowser_update_camera()
{
	float heightdiff;

	camera->position.x = positionToPreview.x - camera_distance;
	camera->rotation.x = camera_distance;
	camera->position.y = positionToPreview.y;
	camera->rotation.y = 0;
	heightdiff = camera_distance * 0.2f;
	camera->position.z = positionToPreview.z + heightdiff;
	camera->rotation.z = -heightdiff;
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

int objbrowser_object_created(struct OBJECT *object)
{
	struct RwV3D pos;
	void *entity;
	float d;

	if (object == &picking_object) {
		btn_cancel->enabled = btn_create->enabled = 1;
		btn_next->enabled = btn_prev->enabled = 1;
		entity = object->sa_object;
		objbase_set_entity_to_render_exclusively(entity);
		game_ObjectGetPos(entity, &pos);
		d = game_EntityGetDistanceFromCentreOfMassToBaseOfModel(entity);
		pos.z -= d;
		game_ObjectSetPos(entity, &pos);
		rotationStartTime = *timeInGame;
		hasvalidobject = 1;
		return 1;
	}
	return 0;
}

static
void create_object()
{
	btn_cancel->enabled = btn_create->enabled = 0;
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

	game_PedSetPos(player, &player_position);

	ui_do_exclusive_mode_basics(wnd);

	if (hasvalidobject) {
		rot.x = 0.0;
		rot.y = 0.0f;
		rot.z = (*timeInGame - rotationStartTime) * 0.00175f;
		game_ObjectSetRotRad(picking_object.sa_object, &rot);
	}
}

static
void restore_after_hide()
{
	camera->position = originalCameraPos;
	camera->rotation = originalCameraRot;
	ui_update_camera();
	objbase_set_entity_to_render_exclusively(NULL);
	ui_hide_window(wnd);
	samp_restore_ui_f7();
	isactive = 0;
	hasvalidobject = 0;
	timeweather_resync();
	ui_set_trapped_in_ui(0);
	ui_exclusive_mode = NULL;
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
void cb_btn_cancel(struct UI_BUTTON *btn)
{
	destroy_object();
	restore_after_hide();
}

void objbrowser_show(struct RwV3D *positionToCreate)
{
	positionToCommit = positionToCreate;
	originalCameraPos = camera->position;
	originalCameraRot = camera->rotation;
	positionToPreview.x = camera->position.x;
	positionToPreview.y = camera->position.y;
	positionToPreview.z = 60.0f;
	camera_distance = 30.0f;
	objbrowser_update_camera();
	create_object();
	ui_show_window(wnd);
	samp_hide_ui_f7();
	isactive = 1;
	timeweather_set_time(12);
	timeweather_set_weather(0);
	ui_set_trapped_in_ui(1);
	ui_exclusive_mode = objbrowser_do_ui;
}

void objbrowser_init()
{
	picking_object.model = 3279;

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
	btn_create = ui_btn_make("Create", cb_btn_create);
	ui_wnd_add_child(wnd, btn_create);
	btn_cancel = ui_btn_make("Cancel", cb_btn_cancel);
	ui_wnd_add_child(wnd, btn_cancel);
}

void objbrowser_dispose()
{
	ui_wnd_dispose(wnd);
}
