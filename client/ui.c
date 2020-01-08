/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ui.h"
#include "vk.h"
#include <math.h>
#include <string.h>

float fresx, fresy;
float canvasx, canvasy;
float cursorx, cursory;
float fontheight, buttonheight, fontpad;

static char active = 0;
static float originalHudScaleX, originalHudScaleY;
static float horizLookAngle, vertLookAngle;
static char key_w, key_a, key_s, key_d;
static char need_camera_update;

static struct UI_BUTTON *btn_foliage;
static struct UI_BUTTON *btn_reload;
static struct UI_COLORPICKER *colpick;
static struct UI_ELEMENT _background_element;

struct UI_ELEMENT *background_element = &_background_element;

void* ui_element_being_clicked;
int ui_mouse_is_just_down;
int ui_mouse_is_just_up;

static int activeRCP = 0;

static unsigned char foliageCall[5] = { 0, 0, 0, 0, 0 };

static
void cb_btn_foliage()
{
	void* position;

	if (*((unsigned char*) 0x53C159) == 0x90) {
		memcpy((void*) 0x53C159, foliageCall, 5);
		foliageCall[0] = 0;
	} else {
		memcpy(foliageCall, (void*) 0x53C159, 5);
		memset((void*) 0x53C159, 0x90, 5);

		/*remove existing foliage*/
		position = (void*) 0xB6F03C; /*_camera.__parent.m_pCoords*/
		if (position == NULL) {
			/*_camera.__parent.placement*/
			position = (void*) 0xB6F02C;
		}
		((void (__cdecl *)(void*,int)) 0x5DC510)(position, 0);
	}
}

static
void cb_btn_reload()
{
	reload_requested = 1;
}

static
void cb_colpick()
{
	racecheckpoints[activeRCP].colABGR = colpick->last_selected_colorABGR;
	racecheckpoints[activeRCP].used = 1;
	racecheckpoints[activeRCP].free = 3;
}

static
void ui_recalculate_sizes()
{
	struct Rect textbounds;

	fresx = (float) GAME_RESOLUTION_X;
	fresy = (float) GAME_RESOLUTION_Y;

	canvasx = fresx / 640.0f;
	canvasy = fresy / 448.0f;

	game_TextGetSizeXY(&textbounds, 1.0f, 1.0f, "JQqd");
	fontheight = textbounds.top - textbounds.bottom;
	buttonheight = fontheight * 1.25f;
	fontpad = (buttonheight - fontheight) / 2;
}

void ui_init()
{
	_background_element.type = BACKGROUND;
	key_w = VK_Z;
	key_a = VK_Q;
	key_s = VK_S;
	key_d = VK_D;
	ui_default_font();
	ui_recalculate_sizes(); /*needed for ui element text measurements*/
	ui_colpick_init();
	btn_foliage = ui_btn_make(10.0f, 500.0f, "Foliage", cb_btn_foliage);
	btn_reload = ui_btn_make(10.0f, 600.0f, "Reload_client", cb_btn_reload);
	colpick = ui_colpick_make(500.0f, 500.0f, 100.0f, cb_colpick);
	racecheckpoints[activeRCP].colABGR = 0xFFFF0000;
	racecheckpoints[activeRCP].free = 0;
	racecheckpoints[activeRCP].used = 1;
	cursorx = fresx / 2.0f;
	cursory = fresy / 2.0f;
}

void ui_draw_rect(float x, float y, float w, float h, int argb)
{
	struct IM2DVERTEX verts[] = {
		{x, y + h, 0, 0x40555556, argb, 1.0f, 0.0f},
		{x + w, y + h, 0, 0x40555556, argb, 1.0f, 0.0f},
		{x, y, 0, 0x40555556, argb, 1.0f, 0.0f},
		{x + w, y, 0, 0x40555556, argb, 1.0f, 0.0f},
	};
	game_RwIm2DPrepareRender();
	game_RwIm2DRenderPrimitive(4, verts, 4);
}

void ui_draw_element_rect(struct UI_ELEMENT *element, int argb)
{
	ui_draw_rect(
		element->x, element->y, element->width, element->height, argb);
}

static
void ui_do_cursor_movement()
{
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

static
void ui_draw_cursor()
{
	/*TODO because this is drawn with rects, it draws below textdraws...*/
	/* (inner|outer)(width|height)(radius|diam)*/
	float iwr, iwd, ihr, ihd;
	float owr, owd, ohr, ohd;

	if (activeMouseState->lmb) {
		iwr = 2.0f; iwd = 4.0f;
		owr = 4.0f, owd = 8.0f;
		ihr = 8.0f, ihd = 16.0f;
		ohr = 10.0f, ohd = 20.0f;
	} else {
		iwr = 1.0f; iwd = 2.0f;
		owr = 3.0f, owd = 6.0f;
		ihr = 6.0f, ihd = 12.0f;
		ohr = 8.0f, ohd = 16.0f;
	}

	ui_draw_rect(cursorx - owr, cursory - ohr, owd, ohd, 0xFF000000);
	ui_draw_rect(cursorx - ohr, cursory - owr, ohd, owd, 0xFF000000);
	ui_draw_rect(cursorx - iwr, cursory - ihr, iwd, ihd, 0xFFFFFFFF);
	ui_draw_rect(cursorx - ihr, cursory - iwr, ihd, iwd, 0xFFFFFFFF);
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
		rot = camera->rotation;
		need_camera_update = 1;
		horizLookAngle = atan2f(rot.y, rot.x);
		xylen = sqrtf(rot.x * rot.x + rot.y * rot.y);
		vertLookAngle = atan2f(xylen, rot.z);
		*enableHudByOpcode = 0;
		originalHudScaleX = *hudScaleX;
		originalHudScaleY = *hudScaleY;
		/*note: scaling the hud might mess up projections*/
		*hudScaleX = 0.0009f;
		*hudScaleY = 0.0014f;
		ui_element_being_clicked = NULL;
	}
}

static
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

void ui_default_font()
{
	game_TextSetLetterSize(1.0f, 1.0f);
	game_TextSetMonospace(0);
	game_TextSetColor(0xFFFFFFFF);
	game_TextSetShadowColor(0xFF000000);
	game_TextSetAlign(LEFT);
	game_TextSetOutline(1);
	game_TextSetFont(1);
}

static
void ui_do_mouse_movement()
{
	struct RwV3D rot;
	float mx, my, xylen;

	mx = activeMouseState->x;
	my = activeMouseState->y;
	if (mx != 0.0f || my != 0.0f) {
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

struct RwV3D textloc;

void ui_render()
{
	struct RwV3D v;

	ui_default_font();

	if (fresx != GAME_RESOLUTION_X || fresy != GAME_RESOLUTION_Y) {
		ui_recalculate_sizes();
		/*TODO: recalculate element sizes*/
	}


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
		ui_mouse_is_just_down =
			activeMouseState->lmb && !prevMouseState->lmb;
		ui_mouse_is_just_up =
			prevMouseState->lmb && !activeMouseState->lmb;

		if (ui_element_being_clicked && ui_mouse_is_just_up) {
			if (ui_btn_mouseup(btn_foliage) ||
				ui_btn_mouseup(btn_reload) ||
				ui_colpick_mouseup(colpick));
			ui_element_being_clicked = NULL;
		}

		if (activeMouseState->rmb) {
			ui_do_mouse_movement();
			ui_do_key_movement();
		} else {
			ui_do_cursor_movement();
		}

		if (need_camera_update) {
			ui_update_camera();
			need_camera_update = 0;
		}

		game_ScreenToWorld(&textloc, fresx / 2.0f, fresy / 2.0f, 20.0f);

		if (ui_element_being_clicked == NULL && ui_mouse_is_just_down) {
			if (!ui_btn_mousedown(btn_foliage) &&
				!ui_btn_mousedown(btn_reload) &&
				!ui_colpick_mousedown(colpick))
			{
				ui_element_being_clicked = background_element;
			}
		}

		ui_colpick_update(colpick);

		if (racecheckpoints[activeRCP].free > 2) {
			racecheckpoints[activeRCP].free--;
		} else if (racecheckpoints[activeRCP].free == 2) {
			racecheckpoints[activeRCP].free = 0;
			racecheckpoints[activeRCP].used = 1;
		}

		ui_colpick_draw(colpick);
		ui_btn_draw(btn_foliage);
		ui_btn_draw(btn_reload);

		ui_draw_cursor();

		if (ui_element_being_clicked == background_element) {
			game_ScreenToWorld(&v, cursorx, cursory, 40.0f);
			racecheckpoints[activeRCP].type = RACECP_TYPE_NORMAL;
			/*racecheckpoints[activeRCP].free = 0;*/
			/*racecheckpoints[activeRCP].used = 1;*/
			/*racecheckpoints[activeRCP].colABGR = 0xFFFF0000;*/
			racecheckpoints[activeRCP].fRadius = 5.0f;
			racecheckpoints[activeRCP].pos.x = v.x;
			racecheckpoints[activeRCP].pos.y = v.y;
			racecheckpoints[activeRCP].pos.z = v.z;
		}
	}

	game_WorldToScreen(&v, &textloc);
	if (v.z > 0.0f) {
		game_TextSetLetterSize(1.0f, 1.0f);
		game_TextSetMonospace(1);
		game_TextSetColor(0xFFFFFFFF);
		game_TextSetShadowColor(0xFF000000);
		game_TextSetAlign(CENTER);
		game_TextSetOutline(1);
		game_TextSetFont(2);
		game_TextPrintString(v.x, v.y, "here");
	}
}

void ui_dispose()
{
	if (foliageCall[0]) {
		cb_btn_foliage();
	}
	ui_deactivate();
	ui_btn_dispose(btn_foliage);
	ui_btn_dispose(btn_reload);
	ui_colpick_dispose(colpick);
}
