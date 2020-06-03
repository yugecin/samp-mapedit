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
	struct RwV3D pos;
	struct RwV3D delta;
	int i;

	if (handlingObject == NULL) {
		return;
	}

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

void bulkedit_render()
{
	int i;

	for (i = 0; i < numBulkEditObjects; i++) {
		entity_draw_bound_rect(bulkEditObjects[i]->sa_object, 0xFF8000);
	}
}
