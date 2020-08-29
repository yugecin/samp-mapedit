/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "project.h"
#include "ui.h"
#include "textdraws.h"
#include <string.h>
#include <stdio.h>

#pragma pack(push,1)
struct TEXTDRAWDATA {
	char name[40];
	struct {
		char flags;
		float font_width;
		float font_height;
		int font_color;
		float box_width;
		float box_height;
		int box_color;
		char shadow_size;
		char outline_size;
		int background_color;
		char font;
		char selectable;
		float x;
		float y;
		short preview_model;
		float preview_rot_x;
		float preview_rot_y;
		float preview_rot_z;
		float preview_zoom;
		short preview_color_1;
		short preview_color_2;
		short text_length;
		char text[800];
	} textdraw;
} data;
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
	struct TEXTDRAWDATA data;
	FILE *text;
	char fname[100];
	int i;

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

	fwrite("TXT\x1", 4, 1, text);
	for (i = 0; i < numtextdraws; i++) {
		memset(data.name, 0, sizeof(data.name));
		strcpy(data.name, textdraw_name[i]);
		data.name[sizeof(data.name) - 1] = 0;
		data.textdraw.flags = 0;
		data.textdraw.flags |= (textdraws[i].byteBox != 0);
		data.textdraw.flags |= (textdraws[i].byteLeft != 0) << 1;
		data.textdraw.flags |= (textdraws[i].byteRight != 0) << 2;
		data.textdraw.flags |= (textdraws[i].byteCenter != 0) << 3;
		data.textdraw.flags |= (textdraws[i].byteProportional != 0) << 4;
		data.textdraw.font_width = textdraws[i].fLetterWidth;
		data.textdraw.font_height = textdraws[i].fLetterHeight;
		data.textdraw.font_color = textdraws[i].dwLetterColor;
		data.textdraw.box_width = textdraws[i].fBoxSizeX;
		data.textdraw.box_height = textdraws[i].fBoxSizeY;
		data.textdraw.box_color = textdraws[i].dwBoxColor;
		data.textdraw.shadow_size = textdraws[i].byteShadowSize;
		data.textdraw.outline_size = textdraws[i].byteOutline;
		data.textdraw.background_color = textdraws[i].dwShadowColor;
		data.textdraw.font = textdraws[i].iStyle;
		data.textdraw.selectable = textdraws[i].selectable;
		data.textdraw.x = textdraws[i].fX;
		data.textdraw.y = textdraws[i].fY;
		data.textdraw.preview_model = textdraws[i].sModel;
		data.textdraw.preview_rot_x = textdraws[i].fRot[0];
		data.textdraw.preview_rot_y = textdraws[i].fRot[1];
		data.textdraw.preview_rot_z = textdraws[i].fRot[2];
		data.textdraw.preview_zoom = textdraws[i].fZoom;
		data.textdraw.preview_color_1 = textdraws[i].sColor[0];
		data.textdraw.preview_color_2 = textdraws[i].sColor[1];
		data.textdraw.text_length = strlen(textdraw_text[i]);
		memset(data.textdraw.text, 0, sizeof(data.textdraw.text));
		strcpy(data.textdraw.text, textdraw_text[i]);
		fwrite(&data, sizeof(data), 1, text);
	}

	fclose(text);
}

void textdrawstorage_load()
{
	struct TEXTDRAWDATA data;
	FILE *text;
	int i;

	for (i = 0; i < numtextdraws; i++) {
		textdraw_enabled[i] = 0;
	}
	numtextdraws = 0;

	text = ts_fopen("rb");
	if (!text) {
		return;
	}

	if (!fread(&i, 4, 1, text) || i != 0x01545854) {
		sprintf(debugstring, "wrong text file version %p", i);
		ui_push_debug_string();
		goto ret;
	}
	while(fread(&data, sizeof(data), 1, text)) {
		textdraws[numtextdraws].byteBox = (data.textdraw.flags & 1) > 0;
		textdraws[numtextdraws].byteLeft = (data.textdraw.flags & 2) > 0;
		textdraws[numtextdraws].byteRight= (data.textdraw.flags & 4) > 0;
		textdraws[numtextdraws].byteCenter = (data.textdraw.flags & 8) > 0;
		textdraws[numtextdraws].byteProportional = (data.textdraw.flags & 16) > 0;
		textdraws[numtextdraws].fLetterWidth = data.textdraw.font_width;
		textdraws[numtextdraws].fLetterHeight = data.textdraw.font_height;
		textdraws[numtextdraws].dwLetterColor = data.textdraw.font_color;
		textdraws[numtextdraws].fBoxSizeX = data.textdraw.box_width;
		textdraws[numtextdraws].fBoxSizeY = data.textdraw.box_height;
		textdraws[numtextdraws].dwBoxColor = data.textdraw.box_color;
		textdraws[numtextdraws].byteShadowSize = data.textdraw.shadow_size;
		textdraws[numtextdraws].byteOutline = data.textdraw.outline_size;
		textdraws[numtextdraws].dwShadowColor = data.textdraw.background_color;
		textdraws[numtextdraws].iStyle = data.textdraw.font;
		textdraws[numtextdraws].selectable = data.textdraw.selectable;
		textdraws[numtextdraws].fX = data.textdraw.x;
		textdraws[numtextdraws].fY = data.textdraw.y;
		textdraws[numtextdraws].sModel = data.textdraw.preview_model;
		textdraws[numtextdraws].fRot[0] = data.textdraw.preview_rot_x;
		textdraws[numtextdraws].fRot[1] = data.textdraw.preview_rot_y;
		textdraws[numtextdraws].fRot[2] = data.textdraw.preview_rot_z;
		textdraws[numtextdraws].fZoom = data.textdraw.preview_zoom;
		textdraws[numtextdraws].sColor[0] = data.textdraw.preview_color_1;
		textdraws[numtextdraws].sColor[1] = data.textdraw.preview_color_2;
		memcpy(textdraw_text[numtextdraws], data.textdraw.text, sizeof(data.textdraw.text));
		strcpy(textdraw_name[numtextdraws], data.name);
		memcpy(textdraws[numtextdraws].szText, data.textdraw.text, sizeof(data.textdraw.text));
		game_TextConvertKeybindings(textdraws[numtextdraws].szText);
		textdraws[numtextdraws].__unk2 = -1;
		textdraws[numtextdraws].__unk3 = -1;
		textdraws[numtextdraws].probablyTextureIdForPreview = -1;
		numtextdraws++;
	}

ret:
	fclose(text);
}
