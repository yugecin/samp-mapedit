/* vim: set filetype=c ts=8 noexpandtab: */

#include "carcols.h"
#include "common.h"
#include "game.h"
#include "ui.h"
#include "vehicles.h"
#include "vehicleselector.h"
#include "../shared/serverlink.h"

struct VEHICLE vehicles[MAX_VEHICLES];
int numvehicles;

static char process_vehicle_deletions = 1;
static int lastRespawnTime;

/*TODO: When deactivating UI, spawn actual vehicles using samp so they
can be driven, and despawn them all again when activating the UI.*/

/**
@return ptr to primary color, secondary color is ptr+1
*/
static
char *vehicles_get_rand_color(int model)
{
	char *colors;

	colors = carcols + carcoldata[model].position;
	colors += (randMax65535() % carcoldata[model].amount) * 2;
	return colors;
}

static
void vehicles_spawn(struct VEHICLE *veh)
{
	veh->sa_vehicle = game_SpawnVehicle(veh->model);
	*((char*) veh->sa_vehicle + 0x434) = veh->col1;
	*((char*) veh->sa_vehicle + 0x435) = veh->col2;
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
			game_ObjectSetHeading(entity, veh->heading);
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

void vehicles_create(short model, struct RwV3D *pos)
{
	struct VEHICLE *veh;
	char *colors;

	colors = vehicles_get_rand_color(model);
	veh = vehicles + numvehicles++;
	veh->model = model;
	veh->pos = *pos;
	veh->heading = 0.0f;
	veh->col1 = *colors;
	veh->col2 = *(colors + 1);
	vehicles_spawn(veh);
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
