/* vim: set filetype=c ts=8 noexpandtab: */

extern char checkpointDescriptions[MAX_RACECHECKPOINTS][251];
extern int numcheckpoints;

void racecp_resetall();
void racecp_prj_preload();
void racecp_frame_update();
void racecp_prj_save(/*FILE*/ void *f, char *buf);
int racecp_prj_load_line(char *buf);
