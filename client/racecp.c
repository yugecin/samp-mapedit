/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "project.h"
#include "racecp.h"
#include "ui.h"
#include <stdio.h>
#include <string.h>

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

void racecp_prj_preload()
{
	numcheckpoints = 0;
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
	union {
		float f;
		int i;
	} data;
	int i;
	FILE *textfile;

	if (numcheckpoints) {
		sprintf(buf, "samp-mapedit\\%s_cps.txt", prj_get_name());
		textfile = fopen(buf, "w");
		if (!textfile) {
			sprintf(debugstring, "failed to open file %s for writing", buf);
			ui_push_debug_string();
		}
	}

	for (i = 0; i < numcheckpoints; i++) {
		fwrite(buf, sprintf(buf, "cp.%c.name %s\n", i + '0', checkpointDescriptions[i]), 1, f);
		fwrite(buf, sprintf(buf, "cp.%c.type %d\n", i + '0', racecheckpoints[i].type), 1, f);
		data.f = racecheckpoints[i].fRadius;
		fwrite(buf, sprintf(buf, "cp.%c.rad %d\n", i + '0', data.i), 1, f);
		data.f = racecheckpoints[i].pos.x;
		fwrite(buf, sprintf(buf, "cp.%c.p.x %d\n", i + '0', data.i), 1, f);
		data.f = racecheckpoints[i].pos.y;
		fwrite(buf, sprintf(buf, "cp.%c.p.y %d\n", i + '0', data.i), 1, f);
		data.f = racecheckpoints[i].pos.z;
		fwrite(buf, sprintf(buf, "cp.%c.p.z %d\n", i + '0', data.i), 1, f);
		data.f = racecheckpoints[i].arrowDirection.x;
		fwrite(buf, sprintf(buf, "cp.%c.a.x %d\n", i + '0', data.i), 1, f);
		data.f = racecheckpoints[i].arrowDirection.y;
		fwrite(buf, sprintf(buf, "cp.%c.a.y %d\n", i + '0', data.i), 1, f);
		data.f = racecheckpoints[i].arrowDirection.z;
		fwrite(buf, sprintf(buf, "cp.%c.a.z %d\n", i + '0', data.i), 1, f);
		fwrite(buf, sprintf(buf, "cp.%c.col %d\n", i + '0', racecheckpoints[i].colABGR, data.i), 1, f);
		fwrite(buf, sprintf(buf,
			"type %d %.4f %.4f %.4f radius %.1f dir %.4f %.4f %.4f // %s\n",
			racecheckpoints[i].type,
			racecheckpoints[i].pos.x,
			racecheckpoints[i].pos.y,
			racecheckpoints[i].pos.z,
			racecheckpoints[i].fRadius,
			racecheckpoints[i].arrowDirection.x,
			racecheckpoints[i].arrowDirection.y,
			racecheckpoints[i].arrowDirection.z,
			checkpointDescriptions[i]
		), 1, textfile);
	}
	fwrite(buf, sprintf(buf, "numcps %d\n", numcheckpoints), 1, f);

	if (textfile) {
		fclose(textfile);
	}
}

int racecp_prj_load_line(char *buf)
{
	union {
		float f;
		int i;
	} data;
	int idx, i, j;

	if (strncmp("numcps ", buf, 7) == 0) {
		numcheckpoints = atoi(buf + 7);
		return 1;
	}

	if (strncmp("cp.", buf, 3)) {
		return 0;
	}

	idx = buf[3] - '0';
	if (idx < 0 || MAX_RACECHECKPOINTS <= idx) {
		return 1;
	}

	if (strncmp(".col ", buf + 4, 5) == 0) {
		racecheckpoints[idx].colABGR = atoi(buf + 9);
		return 1;
	}

	if (strncmp(".type ", buf + 4, 6) == 0) {
		racecheckpoints[idx].type = atoi(buf + 10);
		if (racecheckpoints[idx].type < RACECP_TYPE_AIRROT) {
			racecheckpoints[idx].rotationSpeed = 0;
		} else {
			racecheckpoints[idx].rotationSpeed = 5;
		}
		return 1;
	}

	if (strncmp(".rad", buf + 4, 4) == 0) {
		data.i = atoi(buf + 8);
		racecheckpoints[idx].fRadius = data.f;
		return 1;
	}

	if (strncmp(".p.", buf + 4, 3) == 0) {
		data.i = atoi(buf + 9);
		*(&racecheckpoints[idx].pos.x + buf[7] - 'x') = data.f;
		return 1;
	}

	if (strncmp(".a.", buf + 4, 3) == 0) {
		data.i = atoi(buf + 9);
		*(&racecheckpoints[idx].arrowDirection.x + buf[7] - 'x') = data.f;
		return 1;
	}

	if (strncmp(".name ", buf + 4, 6) == 0) {
		i = 10;
		j = 0;
		while (j < sizeof(checkpointDescriptions[idx])) {
			if (buf[i] == 0 || buf[i] == '\n') {
				j++;
				break;
			}
			checkpointDescriptions[idx][j] = buf[i];
			j++;
			i++;
		}
		checkpointDescriptions[idx][j - 1] = 0;
	}

	return 1;
}
