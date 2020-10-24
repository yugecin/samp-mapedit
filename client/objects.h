/* vim: set filetype=c ts=8 noexpandtab: */

#define MAX_LAYERS 25
#define MAX_OBJECTS 1000
/*250 and every remove can have a LOD (and nobody should remove that much)*/
#define MAX_REMOVES 500

#define OBJECT_STATUS_CREATED 0
#define OBJECT_STATUS_CREATING 1
#define OBJECT_STATUS_WAITING 2
#define OBJECT_STATUS_HIDDEN 3

#define OBJECT_MAX_MATERIALS 16

struct OBJECT {
	void *sa_object;
	int sa_handle;
	int samp_objectid;
	int model;
	struct RwV3D pos; /*only used during creation*/
	struct RwV3D rot; /*only used during creation/project load*/
	char status;
	char num_materials;
	char material_type[OBJECT_MAX_MATERIALS];
	char material_index[OBJECT_MAX_MATERIALS];
	struct {
		short model;
		char txdname_len;
		char txdname[200];
		char texture_len;
		char texture[200];
		int color;
	} material_texture[OBJECT_MAX_MATERIALS];
};

struct REMOVEDBUILDING {
	struct RwV3D origin;
	float radiussq;
	short model;
	/*0 when none*/
	short lodmodel;
	char *description;
};

struct OBJECTLAYER {
	char name[50 + 1];
	int color;
	int show;
	struct OBJECT objects[MAX_OBJECTS];
	int numobjects;
	struct REMOVEDBUILDING removes[MAX_REMOVES];
	int numremoves;
	float stream_in_radius;
	float stream_out_radius;
	float drawdistance;
};

void objects_update();
/*
pos and rot must be set (rot can be null)
*/
void objects_mkobject(struct OBJECT *object);
void objects_mkobject_dontcreate(struct OBJECT *object);
void objects_server_object_created(struct MSG_OBJECT_CREATED *msg);
void objects_client_object_created(object, sa_object, sa_handle);
void objects_object_rotation_changed(int sa_handle);
void objects_create_dummy_entity();
void objects_activate_layer(int idx);
void objects_init();
void objects_dispose();
void objects_prj_save(FILE *f, char *buf);
int objects_prj_load_line(char *buf);
void objects_layer_destroy_objects(struct OBJECTLAYER *layer);
void objects_delete_layer(struct OBJECTLAYER *layer);
void objects_prj_preload();
void objects_prj_postload();
void objects_open_persistent_state();
void objects_clearlayers();
struct OBJECT *objects_find_by_sa_handle(int sa_handle);
/*changes the active layer to the layer the object is in*/
struct OBJECT *objects_find_by_sa_object(void *sa_object);
/*object can be NULL if not cloning from a mapped object but from a world building*/
void objects_clone(struct OBJECT *object, struct CEntity *entity);
void objects_delete_obj(struct OBJECT *obj);
struct OBJECTLAYER *objects_layer_for_object(struct OBJECT *obj);
void objects_set_material_text(
	struct OBJECT *object,
	char materialindex,
	char texturesize,
	char *font,
	char fontsize,
	char bold,
	int textcol,
	int bgcol,
	char align,
	char *text);
void objects_set_material(
	struct OBJECT *object,
	short modelid,
	char materialindex,
	char *txdname,
	char *texturename,
	int materialcolor);

extern struct OBJECTLAYER layers[MAX_LAYERS];
extern struct OBJECTLAYER *active_layer;
extern int numlayers;
extern struct OBJECT manipulateObject;
extern struct CEntity *manipulateEntity;
extern int lastMadeObjectModel;
void objects_show_creation_progress();
