/* vim: set filetype=c ts=8 noexpandtab: */

#include "carcols.h"
#include "common.h"
#include "game.h"
#include "ui.h"
#include "vehicles.h"
#include "vehicleselector.h"

struct VEHICLE vehicles[MAX_VEHICLES];
int numvehicles;

static char process_vehicle_deletions = 1;
static int lastRespawnTime;

/*TODO: When deactivating UI, spawn actual vehicles using samp so they
can be driven, and despawn them all again when activating the UI.*/

char *vehicles_get_rand_color(int model)
{
	char *colors;

	colors = carcols + carcoldata[model - 400].position;
	colors += (randMax65535() % carcoldata[model - 400].amount) * 2;
	return colors;
}

void vehicles_spawn(struct VEHICLE *veh)
{
	veh->sa_vehicle = game_SpawnVehicle(veh->model);
	vehicles_update_color(veh);
}

void vehicles_frame_update()
{
	struct VEHICLE *veh;
	struct CEntity *entity;
	int i;

	if (*timeInGame - lastRespawnTime > 3000) {
		lastRespawnTime = *timeInGame;
		for (i = 0; i < numvehicles; i++) {
			if (vehicles[i].sa_vehicle == NULL) {
				vehicles_spawn(vehicles + i);
			}
		}
	}

	for (i = 0; i < numvehicles; i++) {
		veh = vehicles + i;
		entity = veh->sa_vehicle;
		if (entity) {
			*((float*) entity + 0x130) = 1000.0f; /*health*/
			process_vehicle_deletions = 0;
			game_ObjectSetPos(entity, &veh->pos);
			game_ObjectSetHeadingRad(entity, veh->heading);
			process_vehicle_deletions = 1;
		}
	}
}

void vehicles_on_entity_removed_from_world(struct CEntity *entity)
{
	int i;

	if (ENTITY_IS_TYPE(entity, ENTITY_TYPE_VEHICLE) &&
		process_vehicle_deletions)
	{
		for (i = 0; i < numvehicles; i++) {
			if (vehicles[i].sa_vehicle == entity) {
				/*TODO: make them again when close*/
				/*TODO: this leaves orphan vehicles when
				it's not deleted by us*/
				vehicles[i].sa_vehicle = NULL;
				return;
			}
		}
	}
}

struct VEHICLE *vehicles_create(short model, struct RwV3D *pos)
{
	struct VEHICLE *veh;
	char *colors;

	colors = vehicles_get_rand_color(model);
	veh = vehicles + numvehicles++;
	veh->model = model;
	veh->pos = *pos;
	veh->heading =
		(float) atan2(camera->lookVector.y, camera->lookVector.x)
		+ M_PI2;
	veh->col[0] = *colors;
	veh->col[1] = *(colors + 1);
	vehicles_spawn(veh);

	return veh;
}

struct VEHICLE *vehicles_from_entity(struct CEntity *entity)
{
	int i;

	for (i = 0; i < numvehicles; i++) {
		if (vehicles[i].sa_vehicle == entity) {
			return vehicles + i;
		}
	}
	return NULL;
}

void vehicles_delete(struct VEHICLE *veh)
{
	if (veh->sa_vehicle) {
		game_DestroyVehicle(veh->sa_vehicle);
	}
	numvehicles--;
	if (!numvehicles) {
		memcpy(veh, vehicles + numvehicles, sizeof(struct VEHICLE));
	}
}

void vehicles_update_color(struct VEHICLE *veh)
{
	*((char*) veh->sa_vehicle + 0x434) = veh->col[0];
	*((char*) veh->sa_vehicle + 0x435) = veh->col[1];
}

void vehicles_destroy()
{
	struct CEntity *entity;

	process_vehicle_deletions = 0;
	while (numvehicles) {
		numvehicles--;
		entity = vehicles[numvehicles].sa_vehicle;
		if (entity) {
			game_DestroyVehicle(entity);
		}
	}
	process_vehicle_deletions = 1;
}
