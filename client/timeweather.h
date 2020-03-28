/* vim: set filetype=c ts=8 noexpandtab: */

void timeweather_init();
void timeweather_dispose();
void timeweather_save(/*FILE*/ void *f, char *buf);
int timeweather_load_line(char *buf);
void timeweather_preload();
void timeweather_postload();
