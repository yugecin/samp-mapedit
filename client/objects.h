/* vim: set filetype=c ts=8 noexpandtab: */

void objects_init();
void objects_dispose();
void objects_prj_save(/*FILE*/ void *f, char *buf);
int objects_prj_load_line(char *buf);
void objects_prj_preload();
void objects_prj_postload();
void objects_server_object_created(struct MSG_OBJECT_CREATED *msg);
