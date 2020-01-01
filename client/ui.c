/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ui.h"

static float fresx, fresy;
static float canvasx, canvasy;
static float cursorx, cursory;

void ui_init()
{
	cursorx = (float) GAME_RESOLUTION_X / 2.0f;
	cursory = (float) GAME_RESOLUTION_Y / 2.0f;
}

void ui_init_frame()
{
	fresx = (float) GAME_RESOLUTION_X;
	fresy = (float) GAME_RESOLUTION_Y;
	canvasx = fresx / 640.0f;
	canvasy = fresy / 448.0f;
}

void ui_do_cursor()
{
	float textsize;
	struct Rect textbounds;

	cursorx += activeMouseState->x;
	cursory -= activeMouseState->y * fresx / fresy; /*TODO: this is off*/

	if (cursorx < 0.0f) {
		cursorx = 0.0f;
	}
	if (cursorx > fresx) {
		cursorx = fresx;
	}
	if (cursory < 0.0f) {
		cursory = 0.0f;
	}
	if (cursory > fresy) {
		cursory = fresy;
	}

	if (activeMouseState->lmb) {
		textsize = 1.2f;
	} else {
		textsize = 1.0f;
	}
	game_TextSetLetterSize(textsize, textsize);
	game_TextSetMonospace(1);
	game_TextSetColor(0xFFFFFFFF);
	game_TextSetShadowColor(0xFF000000);
	game_TextSetAlign(TEXT_ALIGN_CENTER);
	game_TextSetOutline(1);
	game_TextSetFont(2);
	game_TextGetSizeXY(&textbounds, textsize, textsize, "+");
	game_TextPrintString(
		cursorx,
		cursory - (textbounds.top - textbounds.bottom) / 2.0f,
		"+");
}
