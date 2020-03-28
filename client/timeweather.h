/* vim: set filetype=c ts=8 noexpandtab: */

void timeweather_init();
void timeweather_dispose();
void timeweather_prj_save(/*FILE*/ void *f, char *buf);
int timeweather_prj_load_line(char *buf);
void timeweather_prj_preload();
void timeweather_prj_postload();
