/* vim: set filetype=c ts=8 noexpandtab: */

int objbrowser_object_created(struct OBJECT *object);
void objbrowser_show(struct RwV3D *positionToCreate);
void objbrowser_init();
void objbrowser_dispose();
void objbrowser_frame_update();
struct OBJECT *objbrowser_object_by_handle(int sa_handle);
