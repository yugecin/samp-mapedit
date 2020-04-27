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

/*
Vehicles.

Are created by calling samp's CreateVehicle.
Server returns a message with the associated vehicleid.

When a vehicle is streamed in, a message is sent to the server asking what
vehicleid the vehicle at the streamed in position belongs to.
The server returns a message with the associated vehicleid.
*/

/*
static
void vehicles_move(struct VEHICLE *veh)
{
	struct RwV3D rot;
	struct MSG_NC nc;

	if (manipulateEntity != NULL) {
		rot.x = rot.y = 0.0f;
		rot.z = veh->heading;
		game_ObjectSetRotRad(manipulateEntity, &rot);
		game_ObjectSetPos(manipulateEntity, &veh->pos);
		nc._parent.id = MAPEDIT_MSG_NATIVECALL;
		nc._parent.data = 0;
		nc.nc = NC_EditObject;
		nc.params.asint[1] = 0;
		nc.params.asint[2] = manipulateObject.samp_objectid;
		sockets_send(&nc, sizeof(nc));
	}
}
*/

void vehicles_frame_update()
{
	struct VEHICLE *veh;
	struct CEntity *entity;
	int i;

	for (i = 0; i < numvehicles; i++) {
		veh = vehicles + numvehicles;
		entity = veh->sa_vehicle;
		if (entity) {
			*((float*) entity + 0x130) = 1000.0f; /*health*/
			game_VehicleSetPos(entity, &veh->pos);
		}
	}
}

void vehicles_create(short model, struct RwV3D *pos)
{
	vehicles[numvehicles].model = model;
	vehicles[numvehicles].pos = *pos;
	vehicles[numvehicles].heading = 0.0f;
	vehicles[numvehicles].sa_vehicle = game_SpawnVehicle(411);
	sprintf(debugstring, "hi %p", vehicles[numvehicles].sa_vehicle);
	ui_push_debug_string();
}

