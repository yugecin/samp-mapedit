/* vim: set filetype=c ts=8 noexpandtab: */

#define MAX_OBJECTS 1000
/*250 and every remove can have a LOD (and nobody should remove that much)*/
#define MAX_REMOVES 500
#define rpCLUMP 2

#define BBOX_ALPHA_ANIM_VALUE \
	(char) (55 * (1.0f - fabs(sinf(*timeInGame * 0.004f))))

struct OBJECT {
	void *sa_object;
	int sa_handle;
	int samp_objectid;
	int model;
	float temp_x; /*only used during creation*/
	char justcreated;
};

struct REMOVEDBUILDING {
	struct RwV3D origin;
	float radiussq;
	short model;
	/*0 when none*/
	short lodmodel;
	char *description;
};

struct OBJECT manipulateObject;
struct CEntity *manipulateEntity;

void objbase_init();
void objbase_dispose();
struct OBJECT *objects_find_by_sa_handle(int sa_handle);
struct OBJECT *objects_find_by_sa_object(void *sa_object);
void objbase_mkobject(struct OBJECT*, struct RwV3D*);
void objbase_server_object_created(struct MSG_OBJECT_CREATED *msg);
void objbase_select_entity(void *entity);
void objbase_frame_update();
void objbase_set_entity_to_render_exclusively(void *entity);
void objbase_create_dummy_entity();
void objbase_draw_entity_bound_rect(struct CEntity *entity, int col);
