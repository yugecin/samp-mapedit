/* vim: set filetype=c ts=8 noexpandtab: */

void racecp_init();
void racecp_dispose();
void racecp_prj_save(/*FILE*/ void *f, char *buf);
int racecp_prj_load_line(char *buf);
void racecp_prj_postload();
