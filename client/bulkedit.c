/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "ui.h"
#include "game.h"
#include "objects.h"
#include "bulkedit.h"
#include "entity.h"
#include "objectseditor.h"
#include "msgbox.h"
#include <math.h>
#include <string.h>

static struct RwV3D initialPositions[MAX_OBJECTS * 10];
static struct RwV3D initialRotations[MAX_OBJECTS * 10];
static struct RwV3D initialPos;
static struct RwV3D initialRot;
static struct OBJECT *handlingObject;
static struct OBJECT *bulkEditObjects[MAX_OBJECTS * 10];
static int numBulkEditObjects;
static char hasUpdated;

void (*bulkedit_pos_update_method)() = bulkedit_update_pos_sync;
void (*bulkedit_rot_update_method)() = bulkedit_update_rot_sync;
char bulkedit_direction_add_90, bulkedit_direction_remove_90, bulkedit_direction_add_180;

void bulkedit_delete_objects()
{
	int i, numToDelete, highestPos, highestPosIndex;
	struct OBJECT *objects_to_delete[MAX_OBJECTS * 10];

	/*since deleting objects shift the objects around in the layer,
	collect them from highest address to lowest and delete them as such,
	then the remaining objects to delete _should_ never change address*/
	numToDelete = 0;
	while (numBulkEditObjects > 0) {
		highestPosIndex = 0;
		highestPos = (int) bulkEditObjects[0];
		for (i = 1; i < numBulkEditObjects; i++) {
			if ((int) bulkEditObjects[i] > highestPos) {
				highestPos = (int) bulkEditObjects[i];
				highestPosIndex = i;
			}
		}
		objects_to_delete[numToDelete++] = bulkEditObjects[highestPosIndex];
		numBulkEditObjects--;
		if (highestPosIndex != numBulkEditObjects) {
			bulkEditObjects[highestPosIndex] = bulkEditObjects[numBulkEditObjects];
		}
	}
	numBulkEditObjects = 0;

	for (i = 0; i < numToDelete; i++) {
		objects_delete_obj(objects_to_delete[i]);
	}
}

void bulkedit_clone_all()
{
	struct OBJECT *clone;
	int i, j;

	if (numBulkEditObjects == 0) {
		return;
	}

	if (active_layer == NULL) {
		msg_title = "Bulk_edit";
		msg_message = "Select_a_layer_first!";
		msg_btn1text = "Ok";
		msg_show(NULL);
		return;
	}

	if (active_layer->numobjects > MAX_OBJECTS - numBulkEditObjects) {
		msg_title = "Bulk_edit";
		msg_message = "New_objects_would_not_fit,_too_many_objects!";
		msg_btn1text = "Ok";
		msg_show(NULL);
		return;
	}

	bulkedit_commit();

	for (i = 0; i < numBulkEditObjects; i++) {
		clone = active_layer->objects + active_layer->numobjects;
		memcpy(clone, bulkEditObjects[i], sizeof(struct OBJECT));
		game_ObjectGetPos(bulkEditObjects[i]->sa_object, &clone->pos);
		game_ObjectGetRot(bulkEditObjects[i]->sa_object, &clone->rot);
		clone->num_materials = bulkEditObjects[i]->num_materials;
		for (j = 0; j < OBJECT_MAX_MATERIALS; j++) {
			clone->material_index[j] = bulkEditObjects[i]->material_index[j];
			clone->material_texture[j] = bulkEditObjects[i]->material_texture[j];
		}
		objects_mkobject(clone);
		active_layer->numobjects++;
	}

	for (i = 0; i < numBulkEditObjects; i++) {
		bulkEditObjects[i] = active_layer->objects + active_layer->numobjects - numBulkEditObjects + i;
	}

	bulkedit_begin(NULL);
	/*so the cloned objects are still being created at this point*/
	/*stuff will crash when bulkedit updates, but that doesn't happen
	until the user selects an object (handlingObject)*/
	/*assume for now that the objects will be created by the time the user manages
	to choose a handlingObject, and stuff will not crash*/
	/*we could be lazy and use the original objects instead of the cloned ones,
	they're clones after all, but they're cloned to the active layer and may come from
	other layers... so I guess we should really use the cloned objects?*/
}

void bulkedit_begin(struct OBJECT *object)
{
	int i;

	if (numBulkEditObjects == 0 || !bulkedit_is_in_list(object)) {
		handlingObject = NULL;
		return;
	}

	hasUpdated = 0;
	handlingObject = object;

	for (i = 0; i < numBulkEditObjects; i++) {
		game_ObjectGetPos(bulkEditObjects[i]->sa_object, initialPositions + i);
		game_ObjectGetRot(bulkEditObjects[i]->sa_object, initialRotations + i);
		initialRotations[i].x *= M_PI / 180.0f;
		initialRotations[i].y *= M_PI / 180.0f;
		initialRotations[i].z *= M_PI / 180.0f;
	}
	game_ObjectGetPos(object->sa_object, &initialPos);
	game_ObjectGetRot(object->sa_object, &initialRot);
	initialRot.x *= M_PI / 180.0f;
	initialRot.y *= M_PI / 180.0f;
	initialRot.z *= M_PI / 180.0f;
}

void bulkedit_update()
{
	if (handlingObject != NULL) {
		hasUpdated = 1;
		bulkedit_pos_update_method();
		if (bulkedit_pos_update_method != bulkedit_update_pos_rotaroundz) {
			bulkedit_rot_update_method();
		}
	}
}

void bulkedit_commit()
{
	if (handlingObject != NULL) {
		// TODO send samp data?
		bulkedit_begin(handlingObject);
	}
}

void bulkedit_revert()
{
	int i;

	if (!hasUpdated) {
		return;
	}
	for (i = 0; i < numBulkEditObjects; i++) {
		game_ObjectSetPos(bulkEditObjects[i]->sa_object, initialPositions + i);
		game_ObjectSetRotRad(bulkEditObjects[i]->sa_object, initialRotations + i);
	}
}

void bulkedit_end()
{
	handlingObject = NULL;
}

void bulkedit_reset()
{
	bulkedit_end();
	numBulkEditObjects = 0;
}

int bulkedit_add(struct OBJECT *object)
{
	if (!bulkedit_is_in_list(object)) {
		bulkEditObjects[numBulkEditObjects] = object;
		game_ObjectGetPos(object->sa_object, initialPositions + numBulkEditObjects);
		game_ObjectGetRot(object->sa_object, initialRotations + numBulkEditObjects);
		initialRotations[numBulkEditObjects].x *= M_PI / 180.0f;
		initialRotations[numBulkEditObjects].y *= M_PI / 180.0f;
		initialRotations[numBulkEditObjects].z *= M_PI / 180.0f;
		numBulkEditObjects++;
		if (objedit_get_editing_object() == object) {
			bulkedit_begin(object);
		}
		return 1;
	}
	return 0;
}

int bulkedit_remove(struct OBJECT *object)
{
	int i;

	for (i = 0; i < numBulkEditObjects; i++) {
		if (bulkEditObjects[i] == object) {
			if (handlingObject) {
				/*means edit is going on, so need to revert*/
				game_ObjectSetPos(object->sa_object, &initialPositions[i]);
				game_ObjectSetRotRad(object->sa_object, &initialRotations[i]);
			}
			numBulkEditObjects--;
			bulkEditObjects[i] = bulkEditObjects[numBulkEditObjects];
			initialPositions[i] = initialPositions[numBulkEditObjects];
			initialRotations[i] = initialRotations[numBulkEditObjects];
			return 1;
		}
	}
	return 0;
}

int bulkedit_is_in_list(struct OBJECT *object)
{
	int i;

	for (i = 0; i < numBulkEditObjects; i++) {
		if (bulkEditObjects[i] == object) {
			return 1;
		}
	}
	return 0;
}

void bulkedit_move_layer(struct OBJECTLAYER *tolayer)
{
	struct OBJECT *originobject, *destobject, *layerobject;
	struct OBJECTLAYER *originlayer;
	int i, j;

	if (MAX_OBJECTS - tolayer->numobjects < numBulkEditObjects) {
		msg_title = "Bulk_edit";
		msg_message = "Not_enough_free_space_in_selected_layer!";
		msg_btn1text = "Ok";
		msg_show(NULL);
	}

	for (i = 0; i < numBulkEditObjects; i++) {
		originobject = bulkEditObjects[i];
		originlayer = objects_layer_for_object(originobject);
		if (originlayer != tolayer) {
			destobject = tolayer->objects + tolayer->numobjects++;
			memcpy(destobject, originobject, sizeof(struct OBJECT));
			originlayer->numobjects--;
			layerobject = originobject;
			while (layerobject < originlayer->objects + originlayer->numobjects) {
				for (j = i + 1; j < numBulkEditObjects; j++) {
					if (bulkEditObjects[j] == layerobject + 1) {
						bulkEditObjects[j] = layerobject;
					}
				}
				*layerobject = *(layerobject + 1);
				layerobject++;
			}
		}
	}
	/*since indexes will get messed up..*/
	bulkedit_reset();
}

void bulkedit_draw_object_boxes()
{
	int i;

	for (i = 0; i < numBulkEditObjects; i++) {
		if (bulkEditObjects[i]->sa_object) {
			entity_draw_bound_rect(bulkEditObjects[i]->sa_object, 0xFF8000);
		}
	}
}

/**
Puts the furthest object at the first position.
*/
static
struct OBJECT *bulkedit_get_furthest_object_ensure_first()
{
	struct OBJECT *obj, *temp_obj;
	struct RwV3D pos;
	struct RwV3D handle_pos, furthest_pos, delta;
	int i, furthest_object_index;
	float furthest_distance, distance;

	game_ObjectGetPos(handlingObject->sa_object, &handle_pos);
	furthest_distance = -10.0f;
	for (i = 0; i < numBulkEditObjects; i++) {
		if (bulkEditObjects[i] != handlingObject) {
			game_ObjectGetPos(bulkEditObjects[i]->sa_object, &pos);
			delta.x = pos.x - handle_pos.x;
			delta.y = pos.y - handle_pos.y;
			delta.z = pos.z - handle_pos.z;
			distance = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
			if (distance > furthest_distance) {
				furthest_distance = distance;
				furthest_pos = pos;
				obj = bulkEditObjects[i];
				furthest_object_index = i;
			}
		}
	}
	if (furthest_object_index != 0) {
	        temp_obj = bulkEditObjects[0];
		bulkEditObjects[0] = bulkEditObjects[furthest_object_index];
		bulkEditObjects[furthest_object_index] = temp_obj;
	}
	return obj;
}

void bulkedit_update_nop()
{
}

void bulkedit_update_pos_sync()
{
	struct RwV3D pos;
	struct RwV3D delta;
	int i;

	game_ObjectGetPos(handlingObject->sa_object, &pos);
	delta.x = pos.x - initialPos.x;
	delta.y = pos.y - initialPos.y;
	delta.z = pos.z - initialPos.z;
	for (i = 0; i < numBulkEditObjects; i++) {
		if (bulkEditObjects[i] != handlingObject) {
			pos = initialPositions[i];
			pos.x += delta.x;
			pos.y += delta.y;
			pos.z += delta.z;
			game_ObjectSetPos(bulkEditObjects[i]->sa_object, &pos);
		}
	}
}

void bulkedit_update_pos_spread()
{
	struct OBJECT *furthest_obj;
	struct RwV3D pos;
	struct RwV3D handle_pos, furthest_pos, delta;
	int i, j;
	float p;

	if (numBulkEditObjects < 2) {
		return;
	}

	/*it may scramble the objects but oh well, worries for later*/
	furthest_obj = bulkedit_get_furthest_object_ensure_first();
	game_ObjectGetPos(handlingObject->sa_object, &handle_pos);
	game_ObjectGetPos(furthest_obj->sa_object, &furthest_pos);
	delta.x = handle_pos.x - furthest_pos.x;
	delta.y = handle_pos.y - furthest_pos.y;
	delta.z = handle_pos.z - furthest_pos.z;
	for (i = 0, j = 0; i < numBulkEditObjects; i++) {
		if (bulkEditObjects[i] != handlingObject) {
			p = (float) j / (numBulkEditObjects - 1);
			pos.x = furthest_pos.x + delta.x * p;
			pos.y = furthest_pos.y + delta.y * p;
			pos.z = furthest_pos.z + delta.z * p;
			game_ObjectSetPos(bulkEditObjects[i]->sa_object, &pos);
			j++;
		}
	}
}

void bulkedit_update_pos_copyx()
{
	struct RwV3D pos;
	int i;
	float x;

	game_ObjectGetPos(handlingObject->sa_object, &pos);
	x = pos.x;
	for (i = 0; i < numBulkEditObjects; i++) {
		if (bulkEditObjects[i] != handlingObject) {
			game_ObjectGetPos(bulkEditObjects[i]->sa_object, &pos);
			pos.x = x;
			game_ObjectSetPos(bulkEditObjects[i]->sa_object, &pos);
		}
	}
}

void bulkedit_update_pos_copyy()
{
	struct RwV3D pos;
	int i;
	float y;

	game_ObjectGetPos(handlingObject->sa_object, &pos);
	y = pos.y;
	for (i = 0; i < numBulkEditObjects; i++) {
		if (bulkEditObjects[i] != handlingObject) {
			game_ObjectGetPos(bulkEditObjects[i]->sa_object, &pos);
			pos.y = y;
			game_ObjectSetPos(bulkEditObjects[i]->sa_object, &pos);
		}
	}
}

void bulkedit_update_pos_copyz()
{
	struct RwV3D pos;
	int i;
	float z;

	game_ObjectGetPos(handlingObject->sa_object, &pos);
	z = pos.z;
	for (i = 0; i < numBulkEditObjects; i++) {
		if (bulkEditObjects[i] != handlingObject) {
			game_ObjectGetPos(bulkEditObjects[i]->sa_object, &pos);
			pos.z = z;
			game_ObjectSetPos(bulkEditObjects[i]->sa_object, &pos);
		}
	}
}

void bulkedit_update_pos_rotaroundz()
{
	struct RwV3D dif;
	struct RwV3D d;
	struct RwV3D pos;
	struct RwV3D rot;
	int i;
	float angledif;
	float distance;
	float angle;

	game_ObjectGetPos(handlingObject->sa_object, &pos);
	game_ObjectGetRot(handlingObject->sa_object, &rot);
	rot.x *= M_PI / 180.0f;
	rot.y *= M_PI / 180.0f;
	rot.z *= M_PI / 180.0f;
	dif.x = pos.x - initialPos.x;
	dif.y = pos.y - initialPos.y;
	dif.z = pos.z - initialPos.z;
	angledif = initialRot.z - rot.z;

	for (i = 0; i < numBulkEditObjects; i++) {
		if (bulkEditObjects[i] != handlingObject) {
			d.x = initialPositions[i].x - initialPos.x;
			d.y = initialPositions[i].y - initialPos.y;
			d.z = initialPositions[i].z + dif.z;
			distance = (float) sqrt(d.x * d.x + d.y * d.y);
			angle = (float) atan2(d.y, d.x);
			angle -= angledif;
			d.x = pos.x + (float) cos(angle) * distance;
			d.y = pos.y + (float) sin(angle) * distance;
			game_ObjectSetPos(bulkEditObjects[i]->sa_object, &d);
			rot.x = initialRotations[i].x;
			rot.y = initialRotations[i].y;
			rot.z = initialRotations[i].z - angledif;
			game_ObjectSetRotRad(bulkEditObjects[i]->sa_object, &rot);
		}
	}
}

void bulkedit_update_rot_sync()
{
	struct RwV3D rot;
	struct RwV3D delta;
	int i;

	game_ObjectGetRot(handlingObject->sa_object, &rot);
	delta.x = rot.x * M_PI / 180.0f - initialRot.x;
	delta.y = rot.y * M_PI / 180.0f - initialRot.y;
	delta.z = rot.z * M_PI / 180.0f - initialRot.z;
	for (i = 0; i < numBulkEditObjects; i++) {
		if (bulkEditObjects[i] != handlingObject) {
			rot = initialRotations[i];
			rot.x += delta.x;
			rot.y += delta.y;
			rot.z += delta.z;
			game_ObjectSetRotRad(bulkEditObjects[i]->sa_object, &rot);
		}
	}
}

void bulkedit_update_rot_spread()
{
	struct OBJECT *furthest_obj;
	struct RwV3D rot;
	struct RwV3D handle_pos, handle_rot, furthest_rot, delta;
	int i, j;
	float p;

	if (numBulkEditObjects < 2) {
		return;
	}

	furthest_obj = bulkedit_get_furthest_object_ensure_first();
	game_ObjectGetPos(handlingObject->sa_object, &handle_pos);
	game_ObjectGetRot(furthest_obj->sa_object, &furthest_rot);
	furthest_rot.x *= M_PI / 180.0f;
	furthest_rot.y *= M_PI / 180.0f;
	furthest_rot.z *= M_PI / 180.0f;
	game_ObjectGetRot(handlingObject->sa_object, &handle_rot);
	delta.x = handle_rot.x * M_PI / 180.0f - furthest_rot.x;
	delta.y = handle_rot.y * M_PI / 180.0f - furthest_rot.y;
	delta.z = handle_rot.z * M_PI / 180.0f - furthest_rot.z;
	for (i = 0, j = 0; i < numBulkEditObjects; i++) {
		if (bulkEditObjects[i] != handlingObject) {
			p = (float) j / (numBulkEditObjects - 1);
			rot.x = furthest_rot.x + delta.x * p;
			rot.y = furthest_rot.y + delta.y * p;
			rot.z = furthest_rot.z + delta.z * p;
			game_ObjectSetRotRad(bulkEditObjects[i]->sa_object, &rot);
			j++;
		}
	}
}

void bulkedit_update_rot_pseudorandomz()
{
	struct RwV3D rot, handle_rot;
	float summed_rot;
	int i;

	if (numBulkEditObjects < 2) {
		return;
	}

	game_ObjectGetRot(handlingObject->sa_object, &handle_rot);
	summed_rot = handle_rot.z;
	for (i = 0; i < numBulkEditObjects; i++) {
		if (bulkEditObjects[i] != handlingObject) {
			game_ObjectGetRot(bulkEditObjects[i]->sa_object, &rot);
			rot.z = summed_rot;
			game_ObjectSetRotRad(bulkEditObjects[i]->sa_object, &rot);
			summed_rot += handle_rot.z;
		}
	}
}

void bulkedit_update_rot_object_direction()
{
	struct OBJECT *furthest_obj;
	struct RwV3D handle_pos, furthest_pos, rot;
	float angle;
	int i;

	furthest_obj = bulkedit_get_furthest_object_ensure_first();
	game_ObjectGetPos(handlingObject->sa_object, &handle_pos);
	game_ObjectGetPos(furthest_obj->sa_object, &furthest_pos);
	angle = (float) atan((furthest_pos.y - handle_pos.y) / (furthest_pos.x - handle_pos.x));
	if (bulkedit_direction_add_90) {
		angle += M_PI2;
	} else if (bulkedit_direction_remove_90) {
		angle -= M_PI2;
	} else if (bulkedit_direction_add_180) {
		angle += M_PI;
	}
	for (i = 0; i < numBulkEditObjects; i++) {
		game_ObjectGetRot(bulkEditObjects[i]->sa_object, &rot);
		rot.x *= M_PI / 180.0f;
		rot.y *= M_PI / 180.0f;
		rot.z = angle;
		game_ObjectSetRotRad(bulkEditObjects[i]->sa_object, &rot);
	}
}
