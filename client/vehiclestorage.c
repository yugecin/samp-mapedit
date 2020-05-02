/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "project.h"
#include "ui.h"
#include "vehicles.h"
#include <string.h>
#include <stdio.h>

#pragma pack(push,1)
union VEHICLEDATA {
	char buf[200];
	int i;
	struct {
		short model;
		struct RwV3D pos;
		float heading;
		char col[2];
	} vehicle;
};
#pragma pack(pop)

/**
<projectname>_vehicles.veh some binary format, write and read
<projectname>_vehicles.txt some readable format, only written to
*/

static
FILE *vs_fopen(char *ext, char *mode)
{
	FILE *file;
	char fname[100];

	sprintf(fname, "samp-mapedit\\%s_vehicles.%s", prj_get_name(), ext);
	file = fopen(fname, mode);

	if (!file && mode[0] == 'w') {
		sprintf(debugstring, "failed to write file %s", fname);
		ui_push_debug_string();
	}

	return file;
}

void vehiclestorage_save()
{
	union VEHICLEDATA data;
	struct VEHICLE *v;
	FILE *veh, *txt;
	int i;

	if (!numvehicles) {
		return;
	}

	veh = vs_fopen("veh", "wb");
	if (!veh) {
		return;
	}
	txt = vs_fopen("txt", "wb");
	if (!txt) {
		fclose(veh);
		return;
	}

	data.i = numvehicles;
	fwrite(data.buf, sizeof(int), 1, veh);
	for (i = 0; i < numvehicles; i++) {
		v = vehicles + i;
		data.vehicle.model = v->model;
		data.vehicle.pos = v->pos;
		data.vehicle.heading = v->heading;
		data.vehicle.col[0] = v->col[0];
		data.vehicle.col[1] = v->col[1];
		fwrite(data.buf, sizeof(data.vehicle), 1, veh);
		fwrite(data.buf,
			sprintf(data.buf,
				"%hd %.4f %.4f %.4f %.4f %hd %hd\n",
				v->model,
				v->pos.x,
				v->pos.y,
				v->pos.z,
				v->heading * 180.0f / M_PI,
				(short) v->col[0],
				(short) v->col[1]),
			1,
			txt);
	}

	fclose(veh);
	fclose(txt);
}

void vehiclestorage_load()
{
	union VEHICLEDATA data;
	struct VEHICLE *v;
	FILE *veh;
	int i;

	veh = vs_fopen("veh", "rb");
	if (!veh) {
		return;
	}

	fread(data.buf, sizeof(int), 1, veh);
	numvehicles = data.i;
	for (i = 0; i < numvehicles; i++) {
		v = vehicles + i;
		fread(data.buf, sizeof(data.vehicle), 1, veh);
		v->model = data.vehicle.model;
		v->pos = data.vehicle.pos;
		v->heading = data.vehicle.heading;
		v->col[0] = data.vehicle.col[0];
		v->col[1] = data.vehicle.col[1];
		v->sa_vehicle = NULL;
		vehicles_spawn(v);
	}

	fclose(veh);
}
