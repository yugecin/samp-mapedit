/* vim: set filetype=c ts=8 noexpandtab: */

void objects_init();
void objects_dispose();
void objects_prj_save(/*FILE*/ void *f, char *buf);
int objects_prj_load_line(char *buf);
void objects_prj_preload();
void objects_prj_postload();
/**
@return 0 when context menu should be suppressed
*/
int objects_on_background_element_just_clicked(
	struct CColPoint* colpoint, void *entity);
void objects_on_active_window_changed(struct UI_WINDOW *wnd);
int objects_is_currently_selecting_object();
void objects_ui_activated();
void objects_ui_deactivated();
void objects_frame_update();
int objects_handle_esc();
void objects_open_persistent_state();
void objects_select_entity(void *entity);
void objects_clearlayers();
void objects_show_select_layer_first_msg();
int objects_object_created(struct OBJECT *object);
struct OBJECT *objects_find_by_sa_handle(int sa_handle);
struct OBJECT *objects_find_by_sa_object(void *sa_object);

#define MAX_LAYERS 10

struct OBJECTLAYER {
	char name[50 + 1];
	int color;
	struct OBJECT objects[MAX_OBJECTS];
	int numobjects;
	struct REMOVEDBUILDING removes[MAX_REMOVES];
	int numremoves;
};

extern struct OBJECTLAYER layers[MAX_LAYERS];
extern int numlayers;
extern struct OBJECTLAYER *active_layer;
