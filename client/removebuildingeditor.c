/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ide.h"
#include "im2d.h"
#include "objbase.h"
#include "player.h"
#include "removedbuildings.h"
#include "removebuildingeditor.h"
#include "sockets.h"
#include "ui.h"
#include "vk.h"
#include "../shared/serverlink.h"
#include <stdio.h>
#include <stdlib.h>

#define MOVE_MODE_NONE 0
#define MOVE_MODE_ORIGIN 1
#define MOVE_MODE_RADIUS 2

static struct UI_WINDOW *wnd;
static struct UI_LIST *lst_removelist;
static struct UI_INPUT *in_origin_x;
static struct UI_INPUT *in_origin_y;
static struct UI_INPUT *in_origin_z;
static struct UI_INPUT *in_radius;
static struct UI_INPUT *in_model;
static struct UI_INPUT *in_description;

static char active;
static char move_mode;

static struct REMOVEDBUILDING current_remove;
static float radius;
static rbe_cb *cb;

static char txt_model[45];
static char txt_warn_two_removes[27];

static struct IM2DSPHERE *sphere;

#define MAX_PREVIEW_REMOVES 200
#define VISIBLE_FLAG 0x00000080

struct REMOVEDOBJECTPREVIEW {
	struct CEntity *entity;
	char was_visible;
};

static struct IM2DVERTEX boxverts[] = {
	{0, 0, 0, 0xFF555556, 0x556666DD, 1.0f, 0.0f},
	{0, 0, 0, 0xFF555556, 0x556666DD, 1.0f, 0.0f},
	{0, 0, 0, 0xFF555556, 0x556666DD, 1.0f, 0.0f},
	{0, 0, 0, 0xFF555556, 0x556666DD, 1.0f, 0.0f},
};

static struct REMOVEDOBJECTPREVIEW previewremoves[MAX_PREVIEW_REMOVES];
static int numpreviewremoves;

static
void rbe_center_cam_on(struct RwV3D *pos)
{
	camera->lookVector.x = 45.0f;
	camera->lookVector.y = 45.0f;
	camera->lookVector.z = -25.0f;
	camera->position.x = pos->x - camera->lookVector.x;
	camera->position.y = pos->y - camera->lookVector.y;
	camera->position.z = pos->z - camera->lookVector.z;
	ui_update_camera_after_manual_position();
	ui_store_camera();
}

static
void cb_removelist_click(struct UI_LIST *lst)
{
	struct RwV3D pos;
	int idx;

	idx = lst->selectedindex;
	if (0 <= idx && idx < numpreviewremoves) {
		game_ObjectGetPos(previewremoves[idx].entity, &pos);
		rbe_center_cam_on(&pos);
	}
}

static
void rbe_update_list_items()
{
	char unkname[2];
	char *names[MAX_PREVIEW_REMOVES];
	int i;
	short model;

	unkname[0] = '?';
	unkname[1] = 0;
	for (i = 0; i < numpreviewremoves; i++) {
		model = previewremoves[i].entity->model;
		if (modelNames[model]) {
			names[i] = modelNames[model];
		} else {
			/*unkname should contain model id,
			but this shouldn't happen anyways*/
			names[i] = unkname;
		}
	}
	ui_lst_set_data(lst_removelist, names, numpreviewremoves);
}

/**
@return 0 if there are now no more slots for removing entities
*/
static
int rbe_do_remove_check_entity_check_dups(struct CEntity *entity)
{
	struct RwV3D pos;
	float dx, dy, dz;
	int i;

	if (current_remove.model == -1 ||
		entity->model == current_remove.model)
	{
		game_ObjectGetPos(entity, &pos);
		dx = current_remove.origin.x - pos.x;
		dy = current_remove.origin.y - pos.y;
		if (dx * dx + dy * dy > current_remove.radiussq) {
			return 1;
		}
		dz = current_remove.origin.z - pos.z;
		if (dx * dx + dy * dy + dz * dz > current_remove.radiussq) {
			return 1;
		}
		/*the same entity might be in multiple sectors,
		so check for duplicates here before adding the entity*/
		for (i = 0; i < numpreviewremoves; i++) {
			if (previewremoves[i].entity == entity) {
				return 1;
			}
		}
repeatforlod:
		previewremoves[numpreviewremoves].entity = entity;
		previewremoves[numpreviewremoves].was_visible =
			entity->flags & VISIBLE_FLAG;
		numpreviewremoves++;
		if (numpreviewremoves == MAX_PREVIEW_REMOVES) {
			return 0;
		}
		if (entity->lod > 0) {
			entity = entity->lod;
			if (current_remove.model != -1) {
				current_remove.lodmodel = entity->model;
				txt_warn_two_removes[0] = '~';
			}
			goto repeatforlod;
		}
	}
	return 1;
}

static
void rbe_do_removes()
{
	struct CSector *sector;
	struct CDoubleLinkListNode *node;
	int sectorindex;

	/*TODO: figure out sector coordinates so whole sectors can be culled
	if they're not within reach*/

	/*TODO: this does not contain dynamic objects such as: hay,
	lampposts, building lights, fences, ..*/

	TRACE("rbe_do_removes");
	txt_warn_two_removes[0] = 0;
	current_remove.lodmodel = 0;
	sector = worldSectors;
	sectorindex = MAX_SECTORS;
	while (--sectorindex >= 0) {
		node = sector->buildings;
		while (node != NULL) {
			if (!rbe_do_remove_check_entity_check_dups(node->item))
			{
				goto limitreached;
			}
			node = node->next;
		}
		node = sector->dummies;
		while (node != NULL) {
			if (!rbe_do_remove_check_entity_check_dups(node->item))
			{
				goto limitreached;
			}
			node = node->next;
		}
		sector++;
	}

	goto ret;
limitreached:
	sprintf(debugstring, "reached limit of preview removes");
	ui_push_debug_string();
ret:
	rbe_update_list_items();
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
void rbe_animate_preview_removes()
{
	struct RwV3D p, screenPos;
	int i;
	int doHide;

	TRACE("rbe_animate_preview_removes");
	doHide = *timeInGame % 1000 < 500;
	game_RwIm2DPrepareRender();
	for (i = 0; i < numpreviewremoves; i++) {
		game_ObjectGetPos(previewremoves[i].entity, &p);
		game_WorldToScreen(&screenPos, &p);
		if (screenPos.z > 0) {
			boxverts[0].x = screenPos.x - 5.0f;
			boxverts[0].y = screenPos.y - 5.0f;
			boxverts[1].x = screenPos.x + 5.0f;
			boxverts[1].y = screenPos.y - 5.0f;
			boxverts[2].x = screenPos.x + 5.0f;
			boxverts[2].y = screenPos.y + 5.0f;
			boxverts[3].x = screenPos.x - 5.0f;
			boxverts[3].y = screenPos.y + 5.0f;
			game_RwIm2DRenderPrimitive(5, boxverts, 4);
		}
		if (doHide) {
			previewremoves[i].entity->flags &= ~VISIBLE_FLAG;
		} else if (previewremoves[i].was_visible) {
			previewremoves[i].entity->flags |= VISIBLE_FLAG;
		}
	}
}

static
void rbe_update_position_from_manipulate_object()
{
	struct RwV3D pos;
	float dx, dy, dz, distsq;

	if (move_mode == MOVE_MODE_NONE) {
		return;
	}

	game_ObjectGetPos(manipulateEntity, &pos);
	dx = pos.x - current_remove.origin.x;
	dy = pos.y - current_remove.origin.y;
	dz = pos.z - current_remove.origin.z;
	distsq = dx * dx + dy * dy + dz * dz;
	switch (move_mode) {
	case MOVE_MODE_ORIGIN:
		if (distsq < 0.2f) {
			return;
		}
		current_remove.origin = pos;
		break;
	case MOVE_MODE_RADIUS:
		if (fabs(distsq - current_remove.radiussq) < 0.2f) {
			return;
		}
		current_remove.radiussq = distsq;
		radius = (float) sqrt(distsq);
		break;
	}

	rbe_update_removes();
	im2d_sphere_pos(sphere, &current_remove.origin, radius);
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
	rbe_update_position_from_manipulate_object();
}

static
void rbe_update_position_ui_text()
{
	char buf[20];

	TRACE("rbe_update_position_ui_text");
	sprintf(buf, "%.3f", current_remove.origin.x);
	ui_in_set_text(in_origin_x, buf);
	sprintf(buf, "%.3f", current_remove.origin.y);
	ui_in_set_text(in_origin_y, buf);
	sprintf(buf, "%.3f", current_remove.origin.z);
	ui_in_set_text(in_origin_z, buf);
	sprintf(buf, "%.3f", radius);
	ui_in_set_text(in_radius, buf);
}

static
void rbe_hide()
{
	TRACE("rbe_hide");
	rbe_undo_removes();
	rb_do_all();
	active = 0;
	ui_exclusive_mode = NULL;
}

static
void cb_in_origin_radius(struct UI_INPUT *in)
{
	float *value;

	TRACE("cb_in_origin_radius");
	value = (float*) in->_parent.userdata;
	*value = (float) atof(in->value);
	current_remove.radiussq = radius * radius;
	rbe_update_removes();
	im2d_sphere_pos(sphere, &current_remove.origin, radius);
}

static
void rbe_update_model_name()
{
	if (current_remove.model > 0 && modelNames[current_remove.model]) {
		sprintf(txt_model, "%s", modelNames[current_remove.model]);
	} else {
		txt_model[0] = '?';
		txt_model[1] = 0;
	}
}

static
void cb_in_model(struct UI_INPUT *in)
{
	TRACE("cb_in_model");
	current_remove.model = atoi(in->value);
	if (current_remove.model >= MAX_MODELS) {
		current_remove.model = MAX_MODELS - 1;
	}
	rbe_update_model_name();
	rbe_update_removes();
}

static
void cb_btn_center_cam(struct UI_BUTTON *btn)
{
	TRACE("cb_btn_center_cam");
	rbe_center_cam_on(&current_remove.origin);
}

static
void cb_btn_move_origin(struct UI_BUTTON *btn)
{
	int rot[3];
	struct MSG_NC nc;

	if (manipulateEntity != NULL) {
		rot[0] = rot[1] = rot[2] = 0;
		game_ObjectSetRotRad(manipulateEntity, (struct RwV3D*) &rot);
		game_ObjectSetPos(manipulateEntity, &current_remove.origin);
		move_mode = MOVE_MODE_ORIGIN;
		nc._parent.id = MAPEDIT_MSG_NATIVECALL;
		nc._parent.data = 0;
		nc.nc = NC_EditObject;
		nc.params.asint[1] = 0;
		nc.params.asint[2] = manipulateObject.samp_objectid;
		sockets_send(&nc, sizeof(nc));
	}
}

static
void cb_btn_move_radius(struct UI_BUTTON *btn)
{
	struct MSG_NC nc;
	struct RwV3D pos;
	int rot[3];

	if (manipulateEntity != NULL) {
		rot[0] = rot[1] = rot[2] = 0;
		game_ObjectSetRotRad(manipulateEntity, (struct RwV3D*) &rot);
		pos = current_remove.origin;
		pos.x += radius;
		game_ObjectSetPos(manipulateEntity, &pos);
		move_mode = MOVE_MODE_RADIUS;
		nc._parent.id = MAPEDIT_MSG_NATIVECALL;
		nc._parent.data = 0;
		nc.nc = NC_EditObject;
		nc.params.asint[1] = 0;
		nc.params.asint[2] = manipulateObject.samp_objectid;
		sockets_send(&nc, sizeof(nc));
	}
}

static
void cb_btn_save(struct UI_BUTTON *btn)
{
	TRACE("cb_btn_save");
	if (in_description->valuelen == 0) {
		if (current_remove.description != NULL) {
			free(current_remove.description);
			current_remove.description = NULL;
		}
	} else {
		if (current_remove.description == NULL) {
			current_remove.description = malloc(INPUT_TEXTLEN + 1);
		}
		strcpy(current_remove.description, in_description->value);
	}
	cb(&current_remove);
	rbe_hide();
}

static
void cb_btn_cancel(struct UI_BUTTON *btn)
{
	TRACE("cb_btn_cancel");
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
	in_origin_x->_parent.userdata = (void*) &current_remove.origin.x;
	ui_wnd_add_child(wnd, NULL);
	ui_wnd_add_child(wnd, in_origin_y = ui_in_make(cb_in_origin_radius));
	in_origin_y->_parent.userdata = (void*) &current_remove.origin.y;
	ui_wnd_add_child(wnd, NULL);
	ui_wnd_add_child(wnd, in_origin_z = ui_in_make(cb_in_origin_radius));
	in_origin_z->_parent.userdata = (void*) &current_remove.origin.z;
	ui_wnd_add_child(wnd, NULL);
	ui_wnd_add_child(wnd, ui_btn_make("Move", cb_btn_move_origin));
	ui_wnd_add_child(wnd, NULL);
	ui_wnd_add_child(wnd, ui_btn_make("Center_Camera", cb_btn_center_cam));
	ui_wnd_add_child(wnd, ui_lbl_make("Radius:"));
	ui_wnd_add_child(wnd, in_radius = ui_in_make(cb_in_origin_radius));
	in_radius->_parent.userdata = (void*) &radius;
	ui_wnd_add_child(wnd, NULL);
	ui_wnd_add_child(wnd, ui_btn_make("Move", cb_btn_move_radius));
	ui_wnd_add_child(wnd, ui_lbl_make("Model:"));
	ui_wnd_add_child(wnd, in_model = ui_in_make(cb_in_model));
	ui_wnd_add_child(wnd, NULL);
	ui_wnd_add_child(wnd, ui_lbl_make(txt_model));
	ui_wnd_add_child(wnd, NULL);
	ui_wnd_add_child(wnd, ui_lbl_make("use_-1_for_all"));
	ui_wnd_add_child(wnd, NULL);
	ui_wnd_add_child(wnd, ui_lbl_make(txt_warn_two_removes));
	ui_wnd_add_child(wnd, ui_lbl_make("Description:"));
	ui_wnd_add_child(wnd, in_description = ui_in_make(NULL));
	lbl = ui_lbl_make("Affected_buildings:_(click_to_focus)");
	ui_wnd_add_child(wnd, lbl);
	lbl->_parent.span = 2;
	lst_removelist = ui_lst_make(20, cb_removelist_click);
	lst_removelist->_parent.span = 2;
	ui_wnd_add_child(wnd, lst_removelist);
	ui_wnd_add_child(wnd, btn = ui_btn_make("Save", cb_btn_save));
	btn->_parent.span = 2;
	ui_wnd_add_child(wnd, btn = ui_btn_make("Cancel", cb_btn_cancel));
	btn->_parent.span = 2;

	sphere = im2d_sphere_make(0x1F22CC22);
	strcpy(txt_warn_two_removes, "~r~takes_two_remove_slots");
}

void rbe_dispose()
{
	TRACE("rbe_dispose");
	ui_wnd_dispose(wnd);
	free(sphere);
}

void rbe_show(struct REMOVEDBUILDING *remove, rbe_cb *_cb)
{
	TRACE("rbe_show");
	rb_undo_all();
	ui_exclusive_mode = rbe_do_ui;
	active = 1;
	current_remove = *remove;
	current_remove.lodmodel = 0;
	if (remove->description != NULL) {
		ui_in_set_text(in_description, remove->description);
	} else {
		ui_in_set_text(in_description, "");
	}
	radius = (float) sqrt(current_remove.radiussq);
	cb = _cb;
	move_mode = MOVE_MODE_NONE;
	txt_warn_two_removes[0] = 0;

	sprintf(txt_model, "%hd", current_remove.model);
	ui_in_set_text(in_model, txt_model);
	rbe_update_model_name();

	rbe_update_position_ui_text();
	im2d_sphere_pos(sphere, &current_remove.origin, radius);
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
