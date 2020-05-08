/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "racecp.h"
#include <stdio.h>

char checkpointDescriptions[MAX_RACECHECKPOINTS][INPUT_TEXTLEN + 1];
int numcheckpoints;

void racecp_resetall()
{
	int i;

	for (i = 0; i < MAX_RACECHECKPOINTS; i++) {
		racecheckpoints[i].used = 0;
		racecheckpoints[i].free = 1;
	}
}

void racecp_frame_update()
{
	int i;

	for (i = 0; i < numcheckpoints; i++) {
		racecheckpoints[i].used = 1;
		racecheckpoints[i].free = 0;	
	}
}

void racecp_prj_save(FILE *f, char *buf)
{
}

int racecp_prj_load_line(char *buf)
{
	return 0;
}
