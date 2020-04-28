/* vim: set filetype=c ts=8 noexpandtab: */

#define MAX_VEHICLES 2000

struct VEHICLE {
	short model;
	struct RwV3D pos;
	float heading;
	unsigned char col1, col2;
	struct CEntity *sa_vehicle;
};

extern struct VEHICLE vehicles[MAX_VEHICLES];
extern int numvehicles;

void vehicles_frame_update();
void vehicles_on_entity_removed_from_world(struct CEntity *entity);
void vehicles_create(short model, struct RwV3D *pos);

