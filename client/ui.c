/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ui.h"
#include "vk.h"
#include "windows.h"
#include <math.h>
#include <string.h>

float fresx, fresy;
float canvasx, canvasy;
float cursorx, cursory;
float font_size_x, font_size_y;
int fontsize, fontratio;
float fontheight, buttonheight, fontpadx, fontpady;

static char active = 0;
static float originalHudScaleX, originalHudScaleY;
static float horizLookAngle, vertLookAngle;
static char key_w, key_a, key_s, key_d;
static char need_camera_update;

struct UI_CONTAINER *background_element = NULL;
struct UI_WINDOW *active_window = NULL;

void* ui_element_being_clicked;
int ui_mouse_is_just_down;
int ui_mouse_is_just_up;

static
void cb_btn_settings(struct UI_BUTTON *btn)
{
	ui_show_window(window_settings);
}

static
void cb_btn_cpsettings(struct UI_BUTTON *btn)
{
	ui_show_window(window_cpsettings);
}

static
void cb_btn_reload(struct UI_BUTTON *btn)
{
	reload_requested = 1;
}

static
void ui_recalculate_sizes()
{
	struct Rect textbounds;

	fresx = (float) GAME_RESOLUTION_X;
	fresy = (float) GAME_RESOLUTION_Y;

	canvasx = fresx / 640.0f;
	canvasy = fresy / 448.0f;

	game_TextGetSizeXY(&textbounds, font_size_x, font_size_y, "JQqd");
	fontheight = textbounds.top - textbounds.bottom;
	fontheight *= 0.8f; /*some ratio...*/
	if (fontsize < 0) {
		fontheight *= 1.0f + 0.05f * fontsize;
	}
	fontpadx = fontpady = fontheight * 0.1f;
	if (fontpadx < 4.0f) {
		fontpadx = 4.0f;
	}
	buttonheight = fontheight + 2.0f * fontpady;
}

void ui_init()
{
	struct UI_BUTTON *btn;

	key_w = VK_Z;
	key_a = VK_Q;
	key_s = VK_S;
	key_d = VK_D;
	ui_set_fontsize(-2, -4);
	cursorx = fresx / 2.0f;
	cursory = fresy / 2.0f;
	ui_elem_init(&dummy_element, DUMMY, 0.0f, 0.0f);
	dummy_element.pref_height = dummy_element.pref_width = 0.0f;
	ui_colpick_init();
	background_element = ui_cnt_make();
	btn = ui_btn_make(10.0f, 550.0f, "Settings", cb_btn_settings);
	ui_cnt_add_child(background_element, (struct UI_ELEMENT*) btn);
	btn = ui_btn_make(300.0f, 550.0f, "Edit_checkpoint", cb_btn_cpsettings);
	ui_cnt_add_child(background_element, (struct UI_ELEMENT*) btn);
	btn = ui_btn_make(10.0f, 600.0f, "Reload_client", cb_btn_reload);
	ui_cnt_add_child(background_element, (struct UI_ELEMENT*) btn);
	wnd_init();
	racecheckpoints[0].colABGR = 0xFFFF0000;
	racecheckpoints[0].free = 0;
	racecheckpoints[0].used = 1;
}

void ui_show_window(struct UI_WINDOW *wnd)
{
	if (active_window == NULL) {
		active_window = wnd;
		wnd->_parent.need_layout = 1;
	}
}

void ui_hide_window(struct UI_WINDOW *wnd)
{
	active_window = NULL;
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

	game_DrawRect(cursorx - owr, cursory - ohr, owd, ohd, 0xFF000000);
	game_DrawRect(cursorx - ohr, cursory - owr, ohd, owd, 0xFF000000);
	game_DrawRect(cursorx - iwr, cursory - ihr, iwd, ihd, 0xFFFFFFFF);
	game_DrawRect(cursorx - ihr, cursory - iwr, ihd, iwd, 0xFFFFFFFF);
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

void ui_set_fontsize(int fs, int fr)
{
	fontsize = fs;
	fontratio = fr;
	font_size_x = 0.9f + 0.025f * fr + fs * 0.1f;
	font_size_y = 1.0f - 0.025f * fr + fs * 0.1f;
	ui_default_font();
	ui_recalculate_sizes();
	if (background_element != NULL) {
		background_element->_parent.proc_recalc_size(
			background_element);
		if (active_window != NULL) {
			active_window->_parent._parent.proc_recalc_size(
				active_window);
		}
	}
}

void ui_default_font()
{
	game_TextSetLetterSize(font_size_x, font_size_y);
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

	if (fresx != GAME_RESOLUTION_X || fresy != GAME_RESOLUTION_Y) {
		ui_default_font();
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
		ui_default_font();

		ui_mouse_is_just_down =
			activeMouseState->lmb && !prevMouseState->lmb;
		ui_mouse_is_just_up =
			prevMouseState->lmb && !activeMouseState->lmb;

		if (ui_element_being_clicked && ui_mouse_is_just_up) {
			if ((active_window != NULL &&
				ui_wnd_mouseup(active_window)) ||
				ui_cnt_mouseup(background_element));
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
			if ((active_window != NULL &&
				ui_wnd_mousedown(active_window)) ||
				ui_cnt_mousedown(background_element));
		}

		if (active_window != NULL) {
			ui_wnd_update(active_window);
		}
		ui_cnt_update(background_element);

		if (racecheckpoints[0].free > 2) {
			racecheckpoints[0].free--;
		} else if (racecheckpoints[0].free == 2) {
			racecheckpoints[0].free = 0;
			racecheckpoints[0].used = 1;
		}

		ui_cnt_draw(background_element);
		if (active_window != NULL) {
			ui_wnd_draw(active_window);
		}

		ui_draw_cursor();

		if (ui_element_being_clicked == background_element) {
			game_ScreenToWorld(&v, cursorx, cursory, 40.0f);
			racecheckpoints[0].type = RACECP_TYPE_NORMAL;
			/*racecheckpoints[activeRCP].free = 0;*/
			/*racecheckpoints[activeRCP].used = 1;*/
			/*racecheckpoints[activeRCP].colABGR = 0xFFFF0000;*/
			racecheckpoints[0].fRadius = 5.0f;
			racecheckpoints[0].pos.x = v.x;
			racecheckpoints[0].pos.y = v.y;
			racecheckpoints[0].pos.z = v.z;
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
	ui_deactivate();
	ui_cnt_dispose(background_element);
	wnd_dispose();
}
