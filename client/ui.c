/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ui.h"

static float fresx, fresy;
static float canvasx, canvasy;
static float cursorx, cursory;
static char active = 0;

void ui_init()
{
	cursorx = (float) GAME_RESOLUTION_X / 2.0f;
	cursory = (float) GAME_RESOLUTION_Y / 2.0f;
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
	game_TextSetAlign(CENTER);
	game_TextSetOutline(1);
	game_TextSetFont(2);
	game_TextGetSizeXY(&textbounds, textsize, textsize, "+");
	game_TextPrintString(
		cursorx,
		cursory - (textbounds.top - textbounds.bottom) / 2.0f,
		"+");
}

void ui_render()
{
	struct CCam *ccam;

	fresx = (float) GAME_RESOLUTION_X;
	fresy = (float) GAME_RESOLUTION_Y;
	canvasx = fresx / 640.0f;
	canvasy = fresy / 448.0f;

	/*
	if (game_InputWasKeyPressed(0x41)) {
	*/
	/*0x52 VK_R*/
	if (currentKeyState->standards[0x52] &&
		!prevKeyState->standards[0x52])
	{
		if (active ^= 1) {
			game_FreezePlayer(1);
			ccam = &camera->cams[camera->activeCam];
			camera->position = ccam->pos;
			camera->rotation = ccam->lookVector;
			camera->lookAt = camera->position;
			camera->lookAt.x += camera->rotation.x;
			camera->lookAt.y += camera->rotation.y;
			camera->lookAt.z += camera->rotation.z;
			game_CameraSetOnPoint(&camera->lookAt, CUT, 1);
		} else {
			game_FreezePlayer(0);
			game_CameraRestore();
		}
	}

	if (active) {
		ui_do_cursor();
	}
}
