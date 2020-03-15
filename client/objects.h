/* vim: set filetype=c ts=8 noexpandtab: */

void objects_init();
void objects_dispose();
void objects_prj_save(/*FILE*/ void *f, char *buf);
int objects_prj_load_line(char *buf);
void objects_prj_preload();
void objects_prj_postload();

#define MAX_LAYERS 10

struct OBJECTLAYER {
	char name[50 + 1];
	int color;
	struct OBJECT objects[MAX_OBJECTS];
	int numobjects;
	char needupdate;
};

extern struct OBJECTLAYER *active_layer;
