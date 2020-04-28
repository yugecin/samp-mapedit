/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "objbase.h"
#include "sockets.h"
#include "ui.h"
#include "vehicles.h"
#include "../shared/serverlink.h"

struct VEHICLE vehicles[MAX_VEHICLES];
int numvehicles;

static char process_vehicle_deletions = 1;

/*TODO: When deactivating UI, spawn actual vehicles using samp so they
can be driven, and despawn them all again when activating the UI.*/

void vehicles_frame_update()
{
	struct VEHICLE *veh;
	struct CEntity *entity;
	int i;

	for (i = 0; i < numvehicles; i++) {
		veh = vehicles + i;
		entity = veh->sa_vehicle;
		if (entity) {
			*((float*) entity + 0x130) = 1000.0f; /*health*/
			process_vehicle_deletions = 0;
			/*TODO: vehicle gets deleted when player is in the
			way*/
			game_VehicleSetPos(entity, &veh->pos);
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

	veh = vehicles + numvehicles++;
	veh->model = model;
	veh->pos = *pos;
	veh->heading = 0.0f;
	veh->sa_vehicle = game_SpawnVehicle(411);

	process_vehicle_deletions = 0;
	game_VehicleSetPos(veh->sa_vehicle, pos);
	process_vehicle_deletions = 1;
}

void vehicles_destroy()
{
	struct CEntity *entity;

	process_vehicle_deletions = 0;
	while (numvehicles--) {
		entity = vehicles[numvehicles].sa_vehicle;
		if (entity) {
			game_DestroyVehicle(entity);
		}
	}
	process_vehicle_deletions = 1;
}
