/* vim: set filetype=c ts=8 noexpandtab: */

void persistence_init();
char *persistence_get_project_to_load();
void persistence_set_project_to_load();
char persistence_get_object_layerid();
void persistence_set_object_layerid(char layerid);
void persistence_set_cursorpos(float x, float y);
int persistence_get_cursorpos(float *x, float *y);
