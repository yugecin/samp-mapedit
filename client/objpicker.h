/* vim: set filetype=c ts=8 noexpandtab: */

int objpick_object_created(struct OBJECT *object);
void objpick_show(struct RwV3D *positionToCreate);
void objpick_init();
void objpick_dispose();
struct OBJECT *objpick_object_by_handle(int sa_handle);
