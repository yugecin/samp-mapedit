/* vim: set filetype=c ts=8 noexpandtab: */

struct RACECP {
	char used;
};

extern struct RACECP racecpdata[MAX_RACECHECKPOINTS];
extern int numcheckpoints;

void racecp_resetall();
void racecp_frame_update();
void racecp_prj_save(/*FILE*/ void *f, char *buf);
int racecp_prj_load_line(char *buf);
