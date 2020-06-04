/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "ui.h"
#include "game.h"
#include "objects.h"
#include "bulkedit.h"
#include "entity.h"
#include <string.h>

static struct RwV3D initialPositions[1000];
static struct RwV3D initialRotations[1000];
static struct RwV3D initialPos;
static struct RwV3D initialRot;
static struct OBJECT *handlingObject;
static struct OBJECT *bulkEditObjects[1000];
static int numBulkEditObjects;

void (*bulkedit_pos_update_method)() = bulkedit_update_pos_sync;
void (*bulkedit_rot_update_method)() = bulkedit_update_rot_sync;

void bulkedit_begin(struct OBJECT *object)
{
	int i;

	if (numBulkEditObjects == 0 || !bulkedit_is_in_list(object)) {
		handlingObject = NULL;
		return;
	}

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
}

void bulkedit_update()
{
	if (handlingObject != NULL) {
		bulkedit_pos_update_method();
		bulkedit_rot_update_method();
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
		bulkEditObjects[numBulkEditObjects++] = object;
		return 1;
	}
	return 0;
}

int bulkedit_remove(struct OBJECT *object)
{
	int i;

	for (i = 0; i < numBulkEditObjects; i++) {
		if (bulkEditObjects[i] == object) {
			numBulkEditObjects--;
			bulkEditObjects[i] = bulkEditObjects[numBulkEditObjects];
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

void bulkedit_draw_object_boxes()
{
	int i;

	for (i = 0; i < numBulkEditObjects; i++) {
		entity_draw_bound_rect(bulkEditObjects[i]->sa_object, 0xFF8000);
	}
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
	struct RwV3D pos;
	struct RwV3D handle_pos, furthest_pos, delta;
	int i, j;
	float p, furthest_distance, distance;

	if (numBulkEditObjects < 2) {
		return;
	}

	/*it may scramble the objects but oh well, worries for later*/
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
			}
		}
	}
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
	struct RwV3D pos, rot;
	struct RwV3D handle_pos, handle_rot, furthest_rot, delta;
	int i, j;
	float p, furthest_distance, distance;

	if (numBulkEditObjects < 2) {
		return;
	}

	/*it may scramble the objects but oh well, worries for later*/
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
				game_ObjectGetRot(bulkEditObjects[i]->sa_object, &furthest_rot);
			}
		}
	}
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
