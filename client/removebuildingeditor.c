/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ide.h"
#include "im2d.h"
#include "player.h"
#include "removebuildingeditor.h"
#include "ui.h"
#include "vk.h"
#include <stdio.h>
#include <stdlib.h>

static struct UI_WINDOW *wnd;
static struct UI_LIST *lst_removelist;
static struct UI_INPUT *in_origin_x;
static struct UI_INPUT *in_origin_y;
static struct UI_INPUT *in_origin_z;
static struct UI_INPUT *in_radius;
static struct UI_INPUT *in_model;

static char active;

static short modelid;
static struct RwV3D origin;
static float radius;

static char txt_model[25];

static struct IM2DSPHERE *sphere;

#define MAX_PREVIEW_REMOVES 200
#define VISIBLE_FLAG 0x00000080

struct REMOVEDOBJECTPREVIEW {
	struct CEntity *entity;
	char was_visible;
};

static struct REMOVEDOBJECTPREVIEW previewremoves[MAX_PREVIEW_REMOVES];
static int numpreviewremoves;

static
void rbe_animate_preview_removes()
{
	struct RwV3D p;
	int i;
	int doHide;

	TRACE("rbe_animate_preview_removes");
	doHide = *timeInGame % 1000 < 500;
	for (i = 0; i < numpreviewremoves; i++) {
		game_ObjectGetPos(previewremoves[i].entity, &p);
		if (doHide) {
			previewremoves[i].entity->flags &= ~VISIBLE_FLAG;
		} else if (previewremoves[i].was_visible) {
			previewremoves[i].entity->flags |= VISIBLE_FLAG;
		}
	}
}

static
void rbe_hide()
{
	TRACE("rbe_hide");
	active = 0;
	ui_exclusive_mode = NULL;
}

static
void rbe_do_ui()
{
	TRACE("rbe_do_ui");
	game_PedSetPos(player, &player_position);
	rbe_animate_preview_removes();
	im2d_sphere_project(sphere);
	im2d_sphere_draw(sphere);
	ui_do_exclusive_mode_basics(wnd, 1);
	ui_draw_default_help_text();
}

static
void rbe_do_removes()
{
	struct REMOVEDOBJECTPREVIEW *preview;
	struct CEntity *entity;
	struct CEntity *entities[1000];
	short numEntities;
	short i;

	TRACE("rbe_do_removes");
	game_WorldFindObjectsInRange(
		&origin,
		radius,
		0,
		&numEntities,
		(short) (sizeof(entities)/sizeof(entities[0])),
		entities,
		1,
		0,
		0,
		0,
		0
	);

	preview = previewremoves;
	for (i = 0; i < numEntities; i++) {
		entity = entities[i];
		if (modelid == -1 || entity->model == modelid) {
add_to_removes:
			preview->entity = entity;
			preview->was_visible = entity->flags & VISIBLE_FLAG;
			preview++;
			numpreviewremoves++;
			if (numpreviewremoves == MAX_PREVIEW_REMOVES) {
				break;
			}
			if (entity->lod > 0) {
				entity = entity->lod;
				goto add_to_removes;
			}
		}
	}
}

static
void rbe_undo_removes()
{
	int i;

	TRACE("rbe_undo_removes");
	for (i = 0; i < numpreviewremoves; i++) {
		if (previewremoves[i].was_visible) {
			previewremoves[i].entity->flags |= VISIBLE_FLAG;
		}
	}
	numpreviewremoves = 0;
}

static
void rbe_update_removes()
{
	TRACE("rbe_update_removes");
	rbe_undo_removes();
	rbe_do_removes();
}

static
void rbe_update_position_ui_text()
{
	char buf[20];

	TRACE("rbe_update_position_ui_text");
	sprintf(buf, "%.3f", origin.x);
	ui_in_set_text(in_origin_x, buf);
	sprintf(buf, "%.3f", origin.y);
	ui_in_set_text(in_origin_y, buf);
	sprintf(buf, "%.3f", origin.z);
	ui_in_set_text(in_origin_z, buf);
	sprintf(buf, "%.3f", radius);
	ui_in_set_text(in_radius, buf);
}

static
void cb_in_origin_radius(struct UI_INPUT *in)
{
	float *value;

	TRACE("cb_in_origin_radius");
	value = (float*) in->_parent.userdata;
	*value = (float) atof(in->value);
	rbe_update_removes();
	im2d_sphere_pos(sphere, &origin, radius);
}

static
void cb_in_model(struct UI_INPUT *in)
{
	TRACE("cb_in_model");
	modelid = atoi(in->value);
	rbe_update_removes();
}

static
void cb_btn_center_cam(struct UI_BUTTON *btn)
{
	TRACE("cb_btn_center_cam");
	camera->lookVector.x = 45.0f;
	camera->lookVector.y = 45.0f;
	camera->lookVector.z = -25.0f;
	camera->position.x = origin.x - camera->lookVector.x;
	camera->position.y = origin.y - camera->lookVector.y;
	camera->position.z = origin.z - camera->lookVector.z;
	ui_update_camera_after_manual_position();
	ui_store_camera();
}

static
void cb_btn_close(struct UI_BUTTON *btn)
{
	TRACE("cb_btn_close");
	rbe_hide();
}

void rbe_init()
{
	struct UI_BUTTON *btn;
	struct UI_LABEL *lbl;

	TRACE("rbe_init");
	active = 0;

	wnd = ui_wnd_make(10000.0f, 300.0f, "Remove_Building");
	wnd->columns = 2;
	wnd->closeable = 0;

	ui_wnd_add_child(wnd, ui_lbl_make("Origin:"));
	ui_wnd_add_child(wnd, in_origin_x = ui_in_make(cb_in_origin_radius));
	in_origin_x->_parent.userdata = (void*) &origin.x;
	ui_wnd_add_child(wnd, NULL);
	ui_wnd_add_child(wnd, in_origin_y = ui_in_make(cb_in_origin_radius));
	in_origin_y->_parent.userdata = (void*) &origin.y;
	ui_wnd_add_child(wnd, NULL);
	ui_wnd_add_child(wnd, in_origin_z = ui_in_make(cb_in_origin_radius));
	in_origin_z->_parent.userdata = (void*) &origin.z;
	ui_wnd_add_child(wnd, NULL);
	ui_wnd_add_child(wnd, ui_btn_make("Center_Camera", cb_btn_center_cam));
	ui_wnd_add_child(wnd, ui_lbl_make("Radius:"));
	ui_wnd_add_child(wnd, in_radius = ui_in_make(cb_in_origin_radius));
	in_radius->_parent.userdata = (void*) &radius;
	ui_wnd_add_child(wnd, ui_lbl_make("Model:"));
	ui_wnd_add_child(wnd, in_model = ui_in_make(cb_in_model));
	ui_wnd_add_child(wnd, NULL);
	ui_wnd_add_child(wnd, ui_lbl_make("use_-1_for_all"));
	ui_wnd_add_child(wnd, lbl = ui_lbl_make("Affected_buildings:"));
	lbl->_parent.span = 2;
	lst_removelist = ui_lst_make(20, NULL);
	lst_removelist->_parent.span = 2;
	ui_wnd_add_child(wnd, lst_removelist);
	ui_wnd_add_child(wnd, btn = ui_btn_make("Close", cb_btn_close));
	btn->_parent.span = 2;

	sphere = im2d_sphere_make(0x6622CC22);
}

void rbe_dispose()
{
	TRACE("rbe_dispose");
	ui_wnd_dispose(wnd);
	free(sphere);
}

void rbe_show(short model, struct RwV3D *_origin, float _radius)
{
	TRACE("rbe_show");
	ui_exclusive_mode = rbe_do_ui;
	active = 1;
	origin = *_origin;
	radius = _radius;

	modelid = model;
	sprintf(txt_model, "%hd", model);
	ui_in_set_text(in_model, txt_model);
	if (modelNames[model]) {
		sprintf(txt_model, "%s", modelNames[model]);
	}

	rbe_update_position_ui_text();
	im2d_sphere_pos(sphere, &origin, radius);
	rbe_update_removes();
}

int rbe_handle_keydown(int vk)
{
	TRACE("rbe_handle_keydown");
	if (active) {
		if (vk == VK_ESCAPE) {
			rbe_hide();
			return 1;
		}
		if (vk == VK_Y) {
			cb_btn_center_cam(NULL);
			return 1;
		}
	}
	return 0;
}

void rbe_on_world_entity_removed(struct CEntity *entity)
{
	int i;

	if (numpreviewremoves == 0 ||
		!ENTITY_IS_TYPE(entity, ENTITY_TYPE_BUILDING) &&
		!ENTITY_IS_TYPE(entity, ENTITY_TYPE_DUMMY) &&
		!ENTITY_IS_TYPE(entity, ENTITY_TYPE_OBJECT))
	{
		return;
	}

	for (i = 0; i < numpreviewremoves; i++) {
removed:
		if (previewremoves[i].entity == entity) {
			numpreviewremoves--;
			memcpy(previewremoves + i,
				previewremoves + numpreviewremoves,
				sizeof(struct REMOVEDOBJECTPREVIEW));
			goto removed;
		}
	}
}
