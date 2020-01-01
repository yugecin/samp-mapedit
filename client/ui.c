/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ui.h"
#include "vk.h"
#include <math.h>

static float fresx, fresy;
static float canvasx, canvasy;
static float cursorx, cursory;
static char active = 0;
static float originalHudScaleX, originalHudScaleY;
static float horizLookAngle, vertLookAngle;
static char key_w, key_a, key_s, key_d;
static char need_camera_update;

void ui_init()
{
	cursorx = (float) GAME_RESOLUTION_X / 2.0f;
	cursory = (float) GAME_RESOLUTION_Y / 2.0f;
	key_w = VK_Z;
	key_a = VK_Q;
	key_s = VK_S;
	key_d = VK_D;
}

void ui_do_cursor()
{
	float textsize;
	struct Rect textbounds;

	if (!activeMouseState->rmb) {
		cursorx += activeMouseState->x;
		/*TODO: this is off*/
		cursory -= activeMouseState->y * fresx / fresy;

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

static
void ui_update_camera()
{
	camera->lookAt = camera->position;
	camera->lookAt.x += camera->rotation.x;
	camera->lookAt.y += camera->rotation.y;
	camera->lookAt.z += camera->rotation.z;
	game_CameraSetOnPoint(&camera->lookAt, CUT, 1);
}

static
void ui_activate()
{
	struct CCam *ccam;
	struct RwV3D rot;
	float xylen;

	if (!active) {
		active = 1;
		game_FreezePlayer(1);
		ccam = &camera->cams[camera->activeCam];
		camera->position = ccam->pos;
		camera->rotation = ccam->lookVector;
		ui_update_camera();
		rot = camera->rotation;
		horizLookAngle = atan2f(rot.y, rot.x);
		xylen = sqrtf(rot.x * rot.x + rot.y * rot.y);
		vertLookAngle = atan2f(xylen, rot.z);
		*enableHudByOpcode = 0;
		originalHudScaleX = *hudScaleX;
		originalHudScaleY = *hudScaleY;
		*hudScaleX = 0.0009f;
		*hudScaleY = 0.0014f;
	}
}

void ui_deactivate()
{
	if (active) {
		active = 0;
		game_FreezePlayer(0);
		game_CameraRestore();
		*enableHudByOpcode = 1;
		*hudScaleX = originalHudScaleX;
		*hudScaleY = originalHudScaleY;
	}
}

static
void ui_do_mouse_movement()
{
	struct RwV3D rot;
	float mx, my, xylen;

	mx = activeMouseState->x;
	my = activeMouseState->y;
	if (activeMouseState->rmb && (mx != 0.0f || my != 0.0f)) {
		horizLookAngle -= mx / 200.0f;
		vertLookAngle -= my / 200.0f;
		if (vertLookAngle < 0.001f) {
			vertLookAngle = 0.001f;
		} else if (vertLookAngle > M_PI - 0.001f) {
			vertLookAngle = M_PI - 0.001f;
		}
		rot.z = cosf(vertLookAngle);
		xylen = sinf(vertLookAngle);
		rot.x = cosf(horizLookAngle) * xylen;
		rot.y = sinf(horizLookAngle) * xylen;
		camera->rotation = rot;
		need_camera_update = 1;
	}
}

static
void ui_do_key_movement()
{
	float speed = 1.0f, angle, xylen;

	if (currentKeyState->standards[key_w]) {
		xylen = sinf(vertLookAngle);
		camera->position.x += cosf(horizLookAngle) * xylen * speed;
		camera->position.y += sinf(horizLookAngle) * xylen * speed;
		camera->position.z += cosf(vertLookAngle) * speed;
		need_camera_update = 1;
	} else if (currentKeyState->standards[key_s]) {
		xylen = sinf(vertLookAngle);
		camera->position.x -= cosf(horizLookAngle) * xylen * speed;
		camera->position.y -= sinf(horizLookAngle) * xylen * speed;
		camera->position.z -= cosf(vertLookAngle) * speed;
		need_camera_update = 1;
	}

	if (currentKeyState->standards[key_a]) {
		angle = horizLookAngle - M_PI2;
		camera->position.x -= cosf(angle) * speed;
		camera->position.y -= sinf(angle) * speed;
		need_camera_update = 1;
	} else if (currentKeyState->standards[key_d]) {
		angle = horizLookAngle - M_PI2;
		camera->position.x += cosf(angle) * speed;
		camera->position.y += sinf(angle) * speed;
		need_camera_update = 1;
	}

	if (currentKeyState->lshift) {
		camera->position.z += speed / 2.5f;
		need_camera_update = 1;
	} else if (currentKeyState->lctrl) {
		camera->position.z -= speed / 2.5f;
		need_camera_update = 1;
	}
}

void ui_render()
{
	fresx = (float) GAME_RESOLUTION_X;
	fresy = (float) GAME_RESOLUTION_Y;
	canvasx = fresx / 640.0f;
	canvasy = fresy / 448.0f;

	if (currentKeyState->standards[VK_R] &&
		!prevKeyState->standards[VK_R])
	{
		if (active) {
			ui_deactivate();
		} else {
			ui_activate();
		}
	}

	if (active) {
		need_camera_update = 0;
		ui_do_mouse_movement();
		ui_do_key_movement();
		ui_do_cursor();
		if (need_camera_update) {
			ui_update_camera();
		}
	}
}
