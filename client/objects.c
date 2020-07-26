/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "bulkedit.h"
#include "entity.h"
#include "game.h"
#include "ide.h"
#include "msgbox.h"
#include "ui.h"
#include "objects.h"
#include "objectsui.h"
#include "objbrowser.h"
#include "objectseditor.h"
#include "objectstorage.h"
#include "bulkedit.h"
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
static int last_create_time = 0;
static int need_create_objects = 0;
static int objects_created_this_frame = 0;

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
	struct MSG_NC nc;

	TRACE("objects_set_position_rotation_after_creation");

	nc._parent.id = MAPEDIT_MSG_NATIVECALL;
	nc.nc = NC_SetObjectPos;
	nc.params.asint[1] = object->samp_objectid;
	nc.params.asflt[2] = object->pos.x;
	nc.params.asflt[3] = object->pos.y;
	nc.params.asflt[4] = object->pos.z;
	sockets_send(&nc, sizeof(nc));
}

static
int objects_object_created(struct OBJECT *object)
{
	struct OBJECT *newobject;
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

	newobject = active_layer->objects + active_layer->numobjects;
	*newobject = cloning_object;
	active_layer->numobjects++;

	nc._parent.id = MAPEDIT_MSG_NATIVECALL;
	nc.nc = NC_SetObjectRot;
	nc.params.asint[1] = cloning_object.samp_objectid;
	nc.params.asflt[2] = cloning_object_rot.x;
	nc.params.asflt[3] = cloning_object_rot.y;
	nc.params.asflt[4] = cloning_object_rot.z;
	sockets_send(&nc, sizeof(nc));

	nc._parent.id = MAPEDIT_MSG_NATIVECALL;
	nc.nc = NC_EditObject;
	nc.params.asint[1] = 0;
	nc.params.asint[2] = cloning_object.samp_objectid;
	sockets_send(&nc, sizeof(nc));

	objui_select_entity(NULL);

	cloning_object.model = 0;

	objedit_show(newobject);
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

static
void objects_do_create_object(struct OBJECT *object)
{
	struct MSG_NC nc;

	if (objects_created_this_frame > 50) {
		object->status = OBJECT_STATUS_WAITING;
		need_create_objects = 1;
		last_create_time = *timeInGame;
		return;
	}
	objects_created_this_frame++;

	nc._parent.id = MAPEDIT_MSG_NATIVECALL;
	nc.nc = NC_CreateObject;
	nc.params.asint[1] = object->model;
	nc.params.asint[2] = (int) object;
	nc.params.asflt[3] = object->pos.y;
	nc.params.asflt[4] = object->pos.z;
	nc.params.asflt[5] = object->rot.x;
	nc.params.asflt[6] = object->rot.y;
	nc.params.asflt[7] = object->rot.z;
	nc.params.asflt[8] = 1500.0f;
	sockets_send(&nc, sizeof(nc));
}

void objects_update()
{
	struct OBJECT *object;
	int l, i;

	objects_created_this_frame = 0;
	if (need_create_objects && *timeInGame - last_create_time > 750) {
		need_create_objects = 0;
		last_create_time = *timeInGame;
		for (l = 0; l < numlayers; l++) {
			for (i = 0; i < layers[l].numobjects; i++) {
				object = layers[l].objects + i;
				if (object->status == OBJECT_STATUS_WAITING) {
					objects_do_create_object(object);
				}
			}
		}
	}
}

void objects_mkobject(struct OBJECT *object)
{
	TRACE("objects_mkobject");
	object->status = OBJECT_STATUS_CREATING;
	object->samp_objectid = -1;
	object->sa_object = NULL;

	objects_do_create_object(object);
}

void objects_mkobject_dontcreate(struct OBJECT *object)
{
	TRACE("objects_mkobject_dontcreate");
	object->status = OBJECT_STATUS_HIDDEN;
	object->samp_objectid = -1;
	object->sa_object = NULL;
}

void objects_server_object_created(struct MSG_OBJECT_CREATED *msg)
{
	struct OBJECT *object;

	TRACE("objects_server_object_created");
	object = msg->object;
	object->samp_objectid = msg->samp_objectid;
	if (object->status == OBJECT_STATUS_CREATED) {
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
	if (object->status != OBJECT_STATUS_CREATED) {
		object->status = OBJECT_STATUS_CREATED;
		/*race with objects_server_object_created*/
		if (object->samp_objectid != -1) {
			objects_object_creation_confirmed(object);
		}
	}
}

void objects_create_dummy_entity()
{
	manipulateObject.model = 1214;
	manipulateObject.pos.x = 0.0f;
	manipulateObject.pos.x = 0.0f;
	manipulateObject.pos.z = -10000.0;
	objects_mkobject(&manipulateObject);
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

void objects_layer_destroy_objects(struct OBJECTLAYER *layer)
{
	struct MSG_BULKDELETE *msg;
	struct OBJECT *object;
	int i, len;

	if (layer->numobjects) {
		len = sizeof(struct MSG_BULKDELETE) + sizeof(int) * layer->numobjects;
		msg = malloc(len);
		msg->_parent.id = MAPEDIT_MSG_BULKDELETE;
		msg->num_deletions = 0;
		for (i = 0; i < layer->numobjects; i++) {
			object = layer->objects + i;
			if (object->samp_objectid != -1) {
				msg->objectids[msg->num_deletions++] = object->samp_objectid;
				if (object->sa_object) {
					game_ObjectGetPos(object->sa_object, &object->pos);
					game_ObjectGetRot(object->sa_object, &object->rot);
				}
				object->samp_objectid = -1;
				object->sa_object = NULL;
				object->status = OBJECT_STATUS_HIDDEN;
			}
		}
		if (msg->num_deletions) {
			sockets_send(msg, len);
		}
	}
}

void objects_delete_layer(struct OBJECTLAYER *layer)
{
	int i, idx;

	bulkedit_revert();
	bulkedit_reset();
	idx = layer - layers;
	objectstorage_mark_layerfile_for_deletion(layer);
	for (i = 0; i < layer->numremoves; i++) {
		if (layer->removes[i].description) {
			free(layer->removes[i].description);
		}
	}
	objects_layer_destroy_objects(layer);
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

	objects_create_dummy_entity();
	objects_clearlayers();
}

void objects_prj_postload()
{
	int layeridx;

	cloning_object.model = 0;
	for (layeridx = 0; layeridx < numlayers; layeridx++) {
		layers[layeridx].show = layeridx == 0;
		objectstorage_load_layer(layers + layeridx);
	}
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
	int layerid, i;
	struct OBJECT *objects;

	for (layerid = 0; layerid < numlayers; layerid++) {
		objects = layers[layerid].objects;
		for (i = layers[layerid].numobjects - 1; i >= 0; i--) {
			if (objects[i].sa_object == sa_object) {
				if (active_layer - layers != layerid) {
					objects_activate_layer(layerid);
				}
				return objects + i;
			}
		}
	}
	return NULL;
}

void objects_clone(struct CEntity *entity)
{
	if (cloning_object.model == 0) {
		cloning_object.model = entity->model;
		game_ObjectGetPos(entity, &cloning_object.pos);
		game_ObjectGetRot(entity, &cloning_object_rot);
		objects_mkobject(&cloning_object);
	}
}

void objects_delete_obj(struct OBJECT *obj)
{
	struct MSG_NC nc;
	int idx;

	nc._parent.id = MAPEDIT_MSG_NATIVECALL;
	nc.nc = NC_DestroyObject;
	nc.params.asint[1] = obj->samp_objectid;
	sockets_send(&nc, sizeof(nc));

	bulkedit_remove(obj);

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

void objects_show_creation_progress()
{
	int l, i;
	int readyobjs, totalobjs;
	char buf[100];

	if (need_create_objects) {
		readyobjs = totalobjs = 0;
		for (l = 0; l < numlayers; l++) {
			for (i = 0; i < layers[l].numobjects; i++) {
				if (layers[l].objects[i].status !=
					OBJECT_STATUS_WAITING)
				{
					readyobjs++;
				}
				totalobjs++;
			}
		}
		sprintf(buf, "loading objects %d/%d", readyobjs, totalobjs);
		game_TextSetAlign(CENTER);
		game_TextPrintString(fresx / 2.0f, fresy / 2.0f, buf);
	}
}

struct OBJECTLAYER *objects_layer_for_object(struct OBJECT *obj)
{
	int i;

	for (i = 0; i < numlayers; i++) {
		if (layers[i].objects + 0 <= obj && obj < layers[i].objects + MAX_OBJECTS) {
			return layers + i;
		}
	}
	return NULL;
}
