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

static struct OBJECT cloning_object;
static struct RwV3D cloning_object_rot;
static int activelayeridx = 0;

struct OBJECTLAYER layers[MAX_LAYERS];
struct OBJECTLAYER *active_layer = NULL;
int numlayers = 0;

struct OBJECT manipulateObject;
struct CEntity *manipulateEntity = NULL;

/**
Since x-coord of creation is a pointer to the object handle, the position needs
to be reset.
*/
static
void objects_set_position_rotation_after_creation(struct OBJECT *object)
{
	struct RwV3D pos;
	struct MSG_NC nc;

	TRACE("objects_set_position_rotation_after_creation");

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
int objects_object_created(struct OBJECT *object)
{
	struct MSG_NC nc;

	if (object != &cloning_object) {
		return 0;
	}

	if (cloning_object.model == 0) {
		return 1;
	}

	if (active_layer->numobjects >= MAX_OBJECTS) {
		msg_message = "Max_objects_reached!";
		msg_title = "Clone";
		msg_btn1text = "Ok";
		msg_show(NULL);
		return 1;
	}

	active_layer->objects[active_layer->numobjects] = cloning_object;
	active_layer->numobjects++;

	nc._parent.id = MAPEDIT_MSG_NATIVECALL;
	nc._parent.data = 0;
	nc.nc = NC_SetObjectRot;
	nc.params.asint[1] = cloning_object.samp_objectid;
	nc.params.asflt[2] = cloning_object_rot.x;
	nc.params.asflt[3] = cloning_object_rot.y;
	nc.params.asflt[4] = cloning_object_rot.z;
	sockets_send(&nc, sizeof(nc));

	nc._parent.id = MAPEDIT_MSG_NATIVECALL;
	nc._parent.data = 0;
	nc.nc = NC_EditObject;
	nc.params.asint[1] = 0;
	nc.params.asint[2] = cloning_object.samp_objectid;
	sockets_send(&nc, sizeof(nc));

	objui_select_entity(NULL);

	cloning_object.model = 0;
	return 1;
}

static
void objects_object_creation_confirmed(struct OBJECT *object)
{
	TRACE("objects_object_creation_confirmed");
	objects_set_position_rotation_after_creation(object);

	if (object == &manipulateObject) {
		manipulateEntity = object->sa_object;
		manipulateEntity->flags &= ~1; /*collision flag*/
		*(float*)((char*) object->sa_object + 0x15C) = 0.01f; /*scale*/
	} else {
		objbrowser_object_created(object) ||
			objects_object_created(object);
	}
}

void objects_mkobject(struct OBJECT *object, struct RwV3D *pos)
{
	struct MSG_NC nc;

	TRACE("objects_mkobject");
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

void objects_server_object_created(struct MSG_OBJECT_CREATED *msg)
{
	struct OBJECT *object;

	TRACE("objects_server_object_created");
	object = msg->object;
	object->samp_objectid = msg->samp_objectid;
	if (!object->justcreated) {
		/*race with objects_object_rotation_changed*/
		objects_object_creation_confirmed(object);
	}
}

void objects_client_object_created(object, sa_object, sa_handle)
	struct OBJECT *object;
	struct CEntity *sa_object;
	int sa_handle;
{
	object->sa_object = sa_object;
	object->sa_handle = sa_handle;
}

void objects_object_rotation_changed(int sa_handle)
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
			objects_object_creation_confirmed(object);
		}
	}
}

void objects_create_dummy_entity()
{
	struct RwV3D pos;

	pos.x = 0.0f;
	pos.y = 0.0f;
	pos.z = -10000.0f;
	manipulateObject.model = 1214;
	objects_mkobject(&manipulateObject, &pos);
}

void objects_activate_layer(int idx)
{
	if (0 <= idx && idx < numlayers) {
		activelayeridx = idx;
		active_layer = layers + idx;
		objui_layer_changed();
		persistence_set_object_layerid(activelayeridx);
		rbui_refresh_list();
	}
}

void objects_init()
{
	struct MSG msg;

	msg.id = MAPEDIT_MSG_RESETOBJECTS;
	sockets_send(&msg, sizeof(msg));
}

void objects_dispose()
{
	objects_clearlayers();
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

		objectstorage_save_layer(layers + i);
	}
	objectstorage_delete_layerfiles_marked_for_deletion();
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

void objects_delete_layer(struct OBJECTLAYER *layer)
{
	struct MSG_NC nc;
	int i, idx;

	idx = layer - layers;
	objectstorage_mark_layerfile_for_deletion(layer);
	for (i = 0; i < layer->numremoves; i++) {
		if (layer->removes[i].description) {
			free(layer->removes[i].description);
		}
	}
	for (i = 0; i < layer->numobjects; i++) {
		nc._parent.id = MAPEDIT_MSG_NATIVECALL;
		nc._parent.data = 0;
		nc.nc = NC_DestroyObject;
		nc.params.asint[1] = layer->objects[i].samp_objectid;
		sockets_send(&nc, sizeof(nc));
	}
	if (idx < numlayers - 1) {
		memcpy(layers + idx,
			layers + idx + 1,
			sizeof(struct OBJECTLAYER) * (numlayers - idx - 1));
	}
	numlayers--;
	if (numlayers) {
		if (numlayers == idx) {
			objects_activate_layer(idx - 1);
		} else {
			objects_activate_layer(idx);
		}
	} else {
		active_layer = NULL;
		activelayeridx = 0;
	}
	rb_undo_all();
	rb_do_all();
}

void objects_prj_preload()
{
	struct MSG msg;

	msg.id = MAPEDIT_MSG_RESETOBJECTS;
	sockets_send(&msg, sizeof(msg));

	objects_clearlayers();
}

void objects_prj_postload()
{
	int layeridx;

	cloning_object.model = 0;
	for (layeridx = 0; layeridx < numlayers; layeridx++) {
		objectstorage_load_layer(layers + layeridx);
	}
	objects_create_dummy_entity();
}

void objects_open_persistent_state()
{
	objects_activate_layer(persistence_get_object_layerid());
}

void objects_clearlayers()
{
	char *description;
	int i;

	while (numlayers > 0) {
		numlayers--;
		for (i = 0; i < layers[numlayers].numremoves; i++) {
			description = layers[numlayers].removes[i].description;
			if (description != NULL) {
				free(description);
			}
		}
	}
	active_layer = NULL;
	activelayeridx = 0;
}

/**
TODO: optimize this
*/
struct OBJECT *objects_find_by_sa_handle(int sa_handle)
{
	struct OBJECTLAYER *layer;
	struct OBJECT *objects;
	int i, layersleft;

	objects = objbrowser_object_by_handle(sa_handle);
	if (objects != NULL) {
		return objects;
	}

	if (cloning_object.sa_handle == sa_handle) {
		return &cloning_object;
	}

	layer = layers;
	layersleft = numlayers;
	while (layersleft--) {
		objects = layer->objects;
		for (i = layer->numobjects - 1; i >= 0; i--) {
			if (objects[i].sa_handle == sa_handle) {
				return objects + i;
			}
		}
		layer++;
	}
	return NULL;
}

/**
TODO: optimize this
*/
struct OBJECT *objects_find_by_sa_object(void *sa_object)
{
	int i;
	struct OBJECT *objects;

	if (active_layer != NULL) {
		objects = active_layer->objects;
		for (i = active_layer->numobjects - 1; i >= 0; i--) {
			if (objects[i].sa_object == sa_object) {
				return objects + i;
			}
		}
	}
	return NULL;
}

void objects_clone(struct CEntity *entity)
{
	struct RwV3D pos;

	if (cloning_object.model == 0) {
		cloning_object.model = entity->model;
		game_ObjectGetPos(entity, &pos);
		game_ObjectGetRot(entity, &cloning_object_rot);
		objects_mkobject(&cloning_object, &pos);
	}
}

void objects_delete_obj(struct OBJECT *obj)
{
	struct MSG_NC nc;
	int idx;

	nc._parent.id = MAPEDIT_MSG_NATIVECALL;
	nc._parent.data = 0;
	nc.nc = NC_DestroyObject;
	nc.params.asint[1] = obj->samp_objectid;
	sockets_send(&nc, sizeof(nc));

	idx = 0;
	for (;;) {
		if (active_layer->objects + idx == obj) {
			break;
		}
		idx++;
		if (idx >= MAX_OBJECTS) {
			return;
		}
	}

	active_layer->numobjects--;
	if (active_layer->numobjects > 0) {
		active_layer->objects[idx] =
			active_layer->objects[active_layer->numobjects];
	}
}
