/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "entity.h"
#include "game.h"
#include "objbase.h"
#include "objects.h"
#include "objbrowser.h"
#include "player.h"
#include "sockets.h"
#include "ui.h"
#include "vehicles.h"
#include "../shared/serverlink.h"
#include <math.h>
#include <string.h>
#include <windows.h>

struct CEntity *exclusiveEntity = NULL;

static struct ENTITYCOLORINFO selected_entity, hovered_entity;

struct OBJECT manipulateObject;
struct CEntity *manipulateEntity = NULL;
struct CEntity *lastCarSpawned;

void objbase_set_entity_to_render_exclusively(void *entity)
{
	struct RwV3D pos;

	TRACE("objbase_set_entity_to_render_exclusively");
	exclusiveEntity = entity;
	if (entity != NULL && manipulateEntity != NULL) {
		game_ObjectGetPos(entity, &pos);
		game_ObjectSetPos(manipulateEntity, &pos);
	}
}

/**
Since x-coord of creation is a pointer to the object handle, the position needs
to be reset.
*/
static
void objbase_set_position_rotation_after_creation(struct OBJECT *object)
{
	struct RwV3D pos;
	struct MSG_NC nc;

	TRACE("objbase_set_position_rotation_after_creation");

	game_ObjectGetPos(object->sa_object, &pos);
	pos.x = object->temp_x;
	nc._parent.id = MAPEDIT_MSG_NATIVECALL;
	nc._parent.data = 0;
	nc.nc = NC_SetObjectPos;
	nc.params.asint[1] = object->samp_objectid;
	nc.params.asflt[2] = pos.x;
	nc.params.asflt[3] = pos.y;
	nc.params.asflt[4] = pos.z;
	sockets_send(&nc, sizeof(nc));

	if (object->rot != NULL) {
		nc._parent.id = MAPEDIT_MSG_NATIVECALL;
		nc._parent.data = 0;
		nc.nc = NC_SetObjectRot;
		nc.params.asint[1] = object->samp_objectid;
		nc.params.asflt[2] = object->rot->x;
		nc.params.asflt[3] = object->rot->y;
		nc.params.asflt[4] = object->rot->z;
		sockets_send(&nc, sizeof(nc));
		free(object->rot);
		object->rot = NULL;
	}
}

static
void objbase_object_creation_confirmed(struct OBJECT *object)
{
	TRACE("objbase_object_creation_confirmed");
	objbase_set_position_rotation_after_creation(object);

	if (object == &manipulateObject) {
		manipulateEntity = object->sa_object;
		manipulateEntity->flags &= ~1; /*collision flag*/
		*(float*)((char*) object->sa_object + 0x15C) = 0.01f; /*scale*/
	} else {
		objbrowser_object_created(object) ||
			objects_object_created(object);
	}
}

void objbase_mkobject(struct OBJECT *object, struct RwV3D *pos)
{
	struct MSG_NC nc;

	TRACE("objbase_mkobject");
	object->temp_x = pos->x;
	object->rot = NULL;
	object->justcreated = 1;
	object->samp_objectid = -1;
	object->sa_object = NULL;

	nc._parent.id = MAPEDIT_MSG_NATIVECALL;
	nc._parent.data = 0; /*TODO*/
	nc.nc = NC_CreateObject;
	nc.params.asint[1] = object->model;
	nc.params.asint[2] = (int) object;
	nc.params.asflt[3] = pos->y;
	nc.params.asflt[4] = pos->z;
	nc.params.asflt[5] = 0.0f;
	nc.params.asflt[6] = 0.0f;
	nc.params.asflt[7] = 0.0f;
	nc.params.asflt[8] = 500.0f;
	sockets_send(&nc, sizeof(nc));
}

void objbase_server_object_created(struct MSG_OBJECT_CREATED *msg)
{
	struct OBJECT *object;

	TRACE("objbase_server_object_created");
	object = msg->object;
	object->samp_objectid = msg->samp_objectid;
	if (!object->justcreated) {
		/*race with objects_object_rotation_changed*/
		objbase_object_creation_confirmed(object);
	}
}

void objbase_select_entity(void *entity)
{
	TRACE("objbase_select_entity");
	/*remove hovered entity to revert its changes first*/
	entity_color(&hovered_entity, NULL, 0);
	entity_color(&selected_entity, entity, 0xFF0000FF);
}

static
void objbase_do_hover()
{
	struct CColPoint cp;
	void *entity;

	TRACE("objbase_do_hover");
	if (objects_is_currently_selecting_object()) {
		if (ui_is_cursor_hovering_any_window()) {
			entity = NULL;
		} else {
			ui_get_entity_pointed_at(&entity, &cp);
			if (entity != NULL) {
				player_position = cp.pos;
			} else {
				game_ScreenToWorld(&player_position,
					cursorx, cursory, 200.0f);
			}
			if (entity == selected_entity.entity) {
				entity = NULL;
			}
		}
		entity_color(&hovered_entity, entity, 0xFFFF00FF);
	}
}

void objbase_on_world_entity_removed(void *entity)
{
	TRACE("objbase_on_world_entity_removed");
	if (entity == selected_entity.entity) {
		objects_select_entity(NULL);
	}
	if (entity == hovered_entity.entity) {
		entity_color(&hovered_entity, NULL, 0);
	}
}

void objbase_frame_update()
{
	union {
		int full;
		struct {
			char b, g, r, a;
		} comps;
	} color;

	if (!objects_is_currently_selecting_object()) {
		return;
	}

	color.full = 0x00FF0000;
	color.comps.a = BBOX_ALPHA_ANIM_VALUE;
	objbase_do_hover();
	entity_draw_bound_rect(selected_entity.entity, color.full);
	color.comps.b = 0xFF;
	entity_draw_bound_rect(hovered_entity.entity, color.full);
}

void objbase_object_created(object, sa_object, sa_handle)
	struct OBJECT *object;
	struct CEntity *sa_object;
	int sa_handle;
{
	object->sa_object = sa_object;
	object->sa_handle = sa_handle;
}

void objbase_object_rotation_changed(int sa_handle)
{
	struct OBJECT *object;

	object = objects_find_by_sa_handle(sa_handle);
	if (object == NULL) {
		if (manipulateObject.sa_handle == sa_handle) {
			object = &manipulateObject;
		} else {
			return;
		}
	}
	if (object->justcreated) {
		object->justcreated = 0;
		/*race with objects_server_object_created*/
		if (object->samp_objectid != -1) {
			objbase_object_creation_confirmed(object);
		}
	}
}

void objbase_create_dummy_entity()
{
	struct RwV3D pos;

	pos.x = 0.0f;
	pos.y = 0.0f;
	pos.z = -10000.0f;
	manipulateObject.model = 1214;
	objbase_mkobject(&manipulateObject, &pos);
}

void objbase_init()
{
}

void objbase_dispose()
{
	entity_color(&selected_entity, NULL, 0);
	entity_color(&hovered_entity, NULL, 0);
}
