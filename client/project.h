/* vim: set filetype=c ts=8 noexpandtab: */

void prj_init();
void prj_do_show_window();
void prj_dispose();
void prj_open_by_name(char *name);
void prj_close();
void prj_save();
void prj_open_persistent_state();
/**
@return 0 when context menu should be suppressed
*/
int prj_on_background_element_just_clicked(
	struct CColPoint* colpoint, void *entity);
