/* vim: set filetype=c ts=8 noexpandtab: */

extern struct RwV3D player_position;

void player_prj_preload();
void player_prj_save(/*FILE*/ void *f, char *buf);
int player_prj_load_line(char *buf);
void player_init();
int player_on_background_element_just_clicked(colpoint, entity);
