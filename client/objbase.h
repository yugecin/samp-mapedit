/* vim: set filetype=c ts=8 noexpandtab: */

#define MAX_OBJECTS 1000
#define rpCLUMP 2

struct OBJECT {
	void *sa_object;
	int sa_handle;
	int samp_objectid;
	int model;
	float temp_x; /*only used during creation*/
	char justcreated;
};

void objbase_init();
void objbase_dispose();
struct OBJECT *objects_find_by_sa_handle(int sa_handle);
struct OBJECT *objbase_mkobject(struct OBJECTLAYER*, int, struct RwV3D*);
void objbase_server_object_created(struct MSG_OBJECT_CREATED *msg);
void objbase_select_entity(void *entity);
void objbase_do_hover();
