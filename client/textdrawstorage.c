/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "project.h"
#include "ui.h"
#include "textdraws.h"
#include <string.h>
#include <stdio.h>

#pragma pack(push,1)
union TEXTDRAWDATA {
	char buf[200];
	int i;
	struct {
		short model;
		struct RwV3D pos;
		float heading;
		char col[2];
	} vehicle;
};
#pragma pack(pop)

static
void getfname(char *buf)
{
	sprintf(buf, "samp-mapedit\\%s_textdraws.text", prj_get_name());
}

static
FILE *ts_fopen(char *mode)
{
	FILE *file;
	char fname[100];

	getfname(fname);
	file = fopen(fname, mode);

	if (!file && mode[0] == 'w') {
		sprintf(debugstring, "failed to write file %s", fname);
		ui_push_debug_string();
	}

	return file;
}

void textdrawstorage_save()
{
	union TEXTDRAWDATA data;
	FILE *text;
	char fname[100];

	if (!numtextdraws) {
		text = ts_fopen("rb");
		if (text) {
			fclose(text);
			getfname(fname);
			remove(fname);
		}
		return;
	}

	text = ts_fopen("wb");
	if (!text) {
		return;
	}

	fclose(text);
}

void textdrawstorage_load()
{
	union TEXTDRAWDATA data;
	FILE *text;

	text = ts_fopen("rb");
	if (!text) {
		return;
	}

	fclose(text);
}
