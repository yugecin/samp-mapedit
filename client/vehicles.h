/* vim: set filetype=c ts=8 noexpandtab: */

#define MAX_VEHICLES 2000

struct VEHICLE {
	short model;
	struct RwV3D pos;
	float heading;
	char col[2];
	struct CEntity *sa_vehicle;
};

extern struct VEHICLE vehicles[MAX_VEHICLES];
extern int numvehicles;

void vehicles_spawn(struct VEHICLE *veh);
/**
@return ptr to primary color, secondary color is ptr+1
*/
char *vehicles_get_rand_color(int model);
void vehicles_frame_update();
void vehicles_ui_activated();
void vehicles_ui_deactivated();
void vehicles_on_entity_removed_from_world(struct CEntity *entity);
struct VEHICLE *vehicles_create(short model, struct RwV3D *pos);
struct VEHICLE *vehicles_from_entity(struct CEntity *entity);
void vehicles_update_color(struct VEHICLE *veh);
void vehicles_delete(struct VEHICLE *veh);
void vehicles_destroy();
