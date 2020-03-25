/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "msgbox.h"
#include "ui.h"
#include "objbase.h"
#include "objects.h"
#include "sockets.h"
#include "samp.h"
#include "../shared/serverlink.h"
#include <string.h>
#include <windows.h>

static struct UI_WINDOW *wnd;
static struct UI_BUTTON *btn_next, *btn_prev;
static struct UI_BUTTON *btn_create, *btn_cancel;

static struct RwV3D originalCameraPos, originalCameraRot;
static struct OBJECT picking_object;
static struct RwV3D *positionToCommit;
static struct RwV3D positionToPreview;

struct OBJECT *objpick_object_by_handle(int sa_handle)
{
	if (picking_object.sa_handle == sa_handle) {
		return &picking_object;
	}
	return NULL;
}

int objpick_object_created(struct OBJECT *object)
{
	if (object == &picking_object) {
		btn_cancel->enabled = btn_create->enabled = 1;
		btn_next->enabled = btn_prev->enabled = 1;
		objbase_set_entity_to_render_exclusively(object->sa_object);
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
void restore_after_hide()
{
	camera->position = originalCameraPos;
	camera->rotation = originalCameraRot;
	ui_update_camera();
	objbase_set_entity_to_render_exclusively(NULL);
	ui_hide_window(wnd);
	samp_show_ui_f10();
}

static
void cb_btn_create(struct UI_BUTTON *btn)
{
	struct OBJECT *object;

	object = active_layer->objects + active_layer->numobjects++;
	memcpy(object, &picking_object, sizeof(struct OBJECT));
	picking_object.model = 0;
	game_ObjectSetPos(object->sa_object, positionToCommit);
	/*game_ObjectSetRot();//TODO*/
	restore_after_hide();
}

static
void cb_btn_cancel(struct UI_BUTTON *btn)
{
	destroy_object();
	restore_after_hide();
}

void objpick_show(struct RwV3D *positionToCreate)
{
	positionToCommit = positionToCreate;
	originalCameraPos = camera->position;
	originalCameraRot = camera->rotation;
	camera->position.z = 70.0f;
	camera->rotation.x = 0.0f;
	camera->rotation.y = 0.0f;
	camera->rotation.z = 0.0f;
	positionToPreview = camera->position;
	positionToPreview.x += 30.0f;
	ui_update_camera();
	create_object();
	ui_show_window(wnd);
	samp_hide_ui_f10();
}

void objpick_init()
{
	picking_object.model = 3279;

	wnd = ui_wnd_make(10.0f, 200.0f, "Objects");
	wnd->closeable = 0;

	btn_next = ui_btn_make("Next_model", cb_btn_next_model);
	ui_wnd_add_child(wnd, btn_next);
	btn_prev = ui_btn_make("Previous_model", cb_btn_prev_model);
	ui_wnd_add_child(wnd, btn_prev);
	btn_create = ui_btn_make("Create", cb_btn_create);
	ui_wnd_add_child(wnd, btn_create);
	btn_cancel = ui_btn_make("Cancel", cb_btn_cancel);
	ui_wnd_add_child(wnd, btn_cancel);
}

void objpick_dispose()
{
	ui_wnd_dispose(wnd);
}
