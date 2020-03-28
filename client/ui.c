/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "client.h"
#include "game.h"
#include "objbrowser.h"
#include "objbase.h"
#include "objects.h"
#include "player.h"
#include "project.h"
#include "msgbox.h"
#include "samp.h"
#include "ui.h"
#include "vk.h"
#include "windows.h"
#include <math.h>
#include <string.h>
#include <windows.h>

float fresx, fresy;
float canvasx, canvasy;
float cursorx, cursory;
float bgclickx, bgclicky;
float font_size_x, font_size_y;
int fontsize, fontratio;
float fontheight, buttonheight, fontpadx, fontpady;
char key_w, key_a, key_s, key_d;
char directional_movement;

static char active = 0;
static float originalHudScaleX, originalHudScaleY;
static float horizLookAngle, vertLookAngle;
static char need_camera_update, has_set_camera_once;
static int context_menu_active = 0;
static int trapped_in_ui;

struct UI_CONTAINER *background_element = NULL;
struct UI_WINDOW *active_window = NULL;
struct UI_WINDOW *main_menu = NULL;
struct UI_WINDOW *context_menu = NULL;

#define DEBUG_STRING_POOL 10
#define DEBUG_STRING_LEN 256

char debugstr[DEBUG_STRING_POOL * DEBUG_STRING_LEN];
char *debugstring = debugstr;
char debugstringidx = 0;

void *ui_element_being_clicked;
void *ui_active_element;
char ui_last_char_down;
int ui_last_key_down;
int ui_mouse_is_just_down;
int ui_mouse_is_just_up;

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

void ui_push_debug_string()
{
	debugstringidx++;
	if (debugstringidx >= DEBUG_STRING_POOL) {
		debugstringidx = 0;
	}
	debugstring = debugstr + DEBUG_STRING_LEN * debugstringidx;
}

void ui_init()
{
	struct UI_BUTTON *btn;

	key_w = VK_Z;
	key_a = VK_Q;
	key_s = VK_S;
	key_d = VK_D;
	directional_movement = 1;
	has_set_camera_once = 0;
	ui_set_fontsize(UI_DEFAULT_FONT_SIZE, UI_DEFAULT_FONT_RATIO);
	cursorx = fresx / 2.0f;
	cursory = fresy / 2.0f;
	ui_elem_init(&dummy_element, UIE_DUMMY);
	dummy_element.pref_height = dummy_element.pref_width = 0.0f;
	ui_colpick_init();
	background_element = ui_cnt_make();

	context_menu = ui_wnd_make(10.0f, 500.0f, "Actions");
	context_menu->columns = 1;
	context_menu->closeable = 0;

	main_menu = ui_wnd_make(10.0f, 500.0f, "Main_Menu");
	main_menu->columns = 2;
	main_menu->closeable = 0;

	btn = ui_btn_make("Reload_client", cb_btn_reload);
	btn->_parent.span = 2;
	ui_wnd_add_child(main_menu, btn);
	
	racecheckpoints[0].colABGR = 0xFFFF0000;
	racecheckpoints[0].free = 0;
	racecheckpoints[0].used = 1;
}

void ui_show_window(struct UI_WINDOW *wnd)
{
	active_window = wnd;
	wnd->_parent._parent.proc_recalc_size(wnd);
	/*recalc_size queues layout update, so not needed here anymore*/
	objects_on_active_window_changed(wnd);
}

void ui_hide_window(struct UI_WINDOW *wnd)
{
	active_window = NULL;
	objects_on_active_window_changed(NULL);
}

struct UI_WINDOW *ui_get_active_window()
{
	return active_window;
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

void ui_update_camera()
{
	camera->lookAt = camera->position;
	camera->lookAt.x += camera->rotation.x;
	camera->lookAt.y += camera->rotation.y;
	camera->lookAt.z += camera->rotation.z;
	game_CameraSetOnPoint(&camera->lookAt, CUT, 1);
}

static
void ui_store_camera()
{
	struct CCam *ccam;
	struct RwV3D rot;
	float xylen;

	ccam = &camera->cams[camera->activeCam];
	camera->position = ccam->pos;
	camera->rotation = ccam->lookVector;
	rot = camera->rotation;
	horizLookAngle = atan2f(rot.y, rot.x);
	xylen = sqrtf(rot.x * rot.x + rot.y * rot.y);
	vertLookAngle = atan2f(xylen, rot.z);
}

static
void ui_activate()
{
	if (!active) {
		active = 1;
		game_PedGetPos(player, &player_position, NULL);
		game_FreezePlayer(1);
		if (!has_set_camera_once) {
			ui_store_camera();
			has_set_camera_once = 1;
		}
		need_camera_update = 1;
		*enableHudByOpcode = 0;
		originalHudScaleX = *hudScaleX;
		originalHudScaleY = *hudScaleY;
		/*note: scaling the hud might mess up projections*/
		*hudScaleX = 0.0009f;
		*hudScaleY = 0.0014f;
		ui_element_being_clicked = NULL;
		samp_break_chat_bar();
		objects_ui_activated();
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
		samp_restore_chat_bar();
		objects_ui_deactivated();
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
		main_menu->_parent._parent.proc_recalc_size(main_menu);
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
		xylen = directional_movement ? sinf(vertLookAngle) : 1.0f;
		camera->position.x += cosf(horizLookAngle) * xylen * speed;
		camera->position.y += sinf(horizLookAngle) * xylen * speed;
		if (directional_movement) {
			camera->position.z += cosf(vertLookAngle) * speed;
		}
		need_camera_update = 1;
	} else if (currentKeyState->standards[key_s]) {
		xylen = directional_movement ? sinf(vertLookAngle) : 1.0f;
		camera->position.x -= cosf(horizLookAngle) * xylen * speed;
		camera->position.y -= sinf(horizLookAngle) * xylen * speed;
		if (directional_movement) {
			camera->position.z -= cosf(vertLookAngle) * speed;
		}
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

void ui_draw_debug_strings()
{
	int i, j;

	i = debugstringidx;
	j = 0;
	do {
		game_TextPrintString(5.0f, 100.0f + j * fontheight,
			debugstr + DEBUG_STRING_LEN * i);
		j++;
		i++;
		if (i >= DEBUG_STRING_POOL) {
			i = 0;
		}
	} while (i != debugstringidx);
}

static
void grablastkey()
{
	short *active;
	short *current;
	int i;

	ui_last_char_down = 0;

	active = activeKeyState->standards;
	current = currentKeyState->standards;
	for (i = VK_A; i <= VK_Z; i++) {
		if (active[i] && !current[i]) {
			ui_last_key_down = i;
			i = 'A' + i - VK_A;
			/*for some reason CKeyState shift stuff don't work*/
			if (!(GetAsyncKeyState(VK_SHIFT) & 0x8000)) {
				i |= 0x20;
			}
			ui_last_char_down = i;
			return;
		}
	}

	if (activeKeyState->back && !currentKeyState->back) {
		ui_last_key_down = VK_BACK;
	} else if (activeKeyState->del && !currentKeyState->del) {
		ui_last_key_down = VK_DELETE;
	} else if (active[VK_SPACE] && !current[VK_SPACE]) {
		ui_last_key_down = VK_SPACE;
		ui_last_char_down = ' ';
	} else if (activeKeyState->up && !currentKeyState->up) {
		ui_last_key_down = VK_UP;
	} else if (activeKeyState->down && !currentKeyState->down) {
		ui_last_key_down = VK_DOWN;
	} else if (activeKeyState->left && !currentKeyState->left) {
		ui_last_key_down = VK_LEFT;
	} else if (activeKeyState->right && !currentKeyState->right) {
		ui_last_key_down = VK_RIGHT;
	} else {
		ui_last_key_down = 0;
	}
}

void ui_on_mousewheel(int value)
{
	if ((active_window != NULL &&
		ui_wnd_mousewheel(active_window, value)) ||
		ui_wnd_mousewheel(main_menu, value) ||
		ui_cnt_mousewheel(background_element, value));
}

void ui_get_entity_pointed_at(void **entity, struct CColPoint *colpoint)
{
	struct RwV3D target, from;

	from = camera->position;
	game_ScreenToWorld(&target, cursorx, cursory, 300.0f);
	if (!game_IntersectBuildingObject(&from, &target, colpoint, entity)) {
		*entity = NULL;
	}
}

static
void background_element_just_clicked()
{
	struct CColPoint cp;
	void *entity;

	ui_get_entity_pointed_at(&entity, &cp);

	if (objects_on_background_element_just_clicked(&cp, entity) &&
		prj_on_background_element_just_clicked(&cp, entity))
	{
		context_menu_active = 1;
		context_menu->_parent._parent.x = cursorx + 10.0f;
		context_menu->_parent._parent.y = cursory + 25.0f;
		context_menu->_parent.need_layout = 1;
	}
}

void ui_render()
{
	int activate_key_pressed;

	ui_default_font();
	if (fresx != GAME_RESOLUTION_X || fresy != GAME_RESOLUTION_Y) {
		ui_recalculate_sizes();
		/*TODO: recalculate element sizes*/
	}

	activate_key_pressed = prevKeyState->standards[VK_R] &&
		!currentKeyState->standards[VK_R];
	if (!active && activate_key_pressed) {
		ui_activate();
		activate_key_pressed = 0;
	}

	if (active) {
		game_PedSetPos(player, &player_position);

		ui_mouse_is_just_down =
			activeMouseState->lmb && !prevMouseState->lmb;
		ui_mouse_is_just_up =
			prevMouseState->lmb && !activeMouseState->lmb;

		if (ui_element_being_clicked && ui_mouse_is_just_up) {
			if ((context_menu_active &&
				ui_wnd_mouseup(context_menu)) ||
				(active_window != NULL &&
				ui_wnd_mouseup(active_window)) ||
				ui_wnd_mouseup(main_menu) ||
				ui_cnt_mouseup(background_element));
			context_menu_active = 0;
			if (ui_element_being_clicked == background_element) {
				background_element_just_clicked();
			}
			ui_element_being_clicked = NULL;
		}

		if (activeMouseState->rmb) {
			ui_do_mouse_movement();
			ui_do_key_movement();
			context_menu_active = 0;
		} else {
			ui_do_cursor_movement();
		}

		grablastkey();

		if (ui_last_key_down != 0 &&
			ui_active_element != NULL &&
			((struct UI_ELEMENT*) ui_active_element)->
			proc_accept_key((struct UI_ELEMENT*) ui_active_element))
		{
			;
		} else if (activeKeyState->standards[VK_Y] &&
			!currentKeyState->standards[VK_Y])
		{
			game_CameraRestoreWithJumpCut();
			/*need to store camera but camera is only applied
			on the next frame(?), so make sure camera doesn't update
			now and check for Y key release*/
			need_camera_update = 0;
		} else  if (currentKeyState->standards[VK_Y] &&
			!activeKeyState->standards[VK_Y])
		{
			ui_store_camera();
		} else if (activate_key_pressed && !trapped_in_ui) {
			ui_deactivate();
			goto justdeactivated;
		}

		if (need_camera_update) {
			ui_update_camera();
			need_camera_update = 0;
		}

		if (ui_element_being_clicked == NULL && ui_mouse_is_just_down) {
			ui_active_element = NULL;
			if ((context_menu_active &&
				ui_wnd_mousedown(context_menu)) ||
				(active_window != NULL &&
				ui_wnd_mousedown(active_window)) ||
				ui_wnd_mousedown(main_menu) ||
				ui_cnt_mousedown(background_element))
			{
				;
			} else {
				context_menu_active = 0;
			}
		}

		if (active_window != NULL) {
			ui_wnd_update(active_window);
		}
		if (context_menu_active) {
			ui_wnd_update(context_menu);
		}
		ui_wnd_update(main_menu);
		ui_cnt_update(background_element);

		objects_frame_update();
		objbase_frame_update();
		objbrowser_frame_update();

		if (racecheckpoints[0].free > 2) {
			racecheckpoints[0].free--;
		} else if (racecheckpoints[0].free == 2) {
			racecheckpoints[0].free = 0;
			racecheckpoints[0].used = 1;
		}

		ui_cnt_draw(background_element);
		ui_wnd_draw(main_menu);
		if (active_window != NULL) {
			ui_wnd_draw(active_window);
		}
		if (context_menu_active) {
			ui_wnd_draw(context_menu);
		}

		ui_draw_debug_strings();

		ui_draw_cursor();

		if (ui_element_being_clicked == background_element) {
			bgclickx = cursorx;
			bgclicky = cursory;
		}

		game_TextSetAlign(CENTER);
		game_TextPrintStringFromBottom(fresx / 2.0f, fresy - 2.0f,
			"~w~press ~r~Y ~w~to reset camera");
	} else {
justdeactivated:
		originalHudScaleX = *hudScaleX;
		originalHudScaleY = *hudScaleY;
		*hudScaleX = 0.0009f;
		*hudScaleY = 0.0014f;
		ui_draw_debug_strings();
		game_TextSetAlign(CENTER);
		game_TextPrintStringFromBottom(fresx / 2.0f, fresy - 2.0f,
			"~w~press ~r~R ~w~to activate UI");
		*hudScaleX = originalHudScaleX;
		*hudScaleY = originalHudScaleY;
	}
}

void ui_dispose()
{
	ui_deactivate();
	ui_cnt_dispose(background_element);
	ui_wnd_dispose(main_menu);
}

void ui_prj_save(FILE *f, char *buf)
{
	struct {
		int x, y, z;
	} *vec3i;
	union {
		int i;
		float f;
	} value;

	vec3i = (void*) &camera->position;
	fwrite(buf, sprintf(buf, "cam.pos.x %d\n", vec3i->x), 1, f);
	fwrite(buf, sprintf(buf, "cam.pos.y %d\n", vec3i->y), 1, f);
	fwrite(buf, sprintf(buf, "cam.pos.z %d\n", vec3i->z), 1, f);
	value.f = horizLookAngle;
	fwrite(buf, sprintf(buf, "cam.rot.x %d\n", value.i), 1, f);
	value.f = vertLookAngle;
	fwrite(buf, sprintf(buf, "cam.rot.y %d\n", value.i), 1, f);
}

int ui_prj_load_line(char *buf)
{
	int *p;
	union {
		int i;
		float f;
	} value;

	if (strncmp("cam.", buf, 4) == 0) {
		if (strncmp("pos.", buf + 4, 4) == 0) {
			p = (int*) &camera->position + buf[8] - 'x';
			*p = atoi(buf + 10);
		} else if (strncmp("rot.", buf + 4, 4) == 0) {
			value.i = atoi(buf + 10);
			if (value.f == value.f) { /*nan check*/
				if (buf[8] == 'x') {
					horizLookAngle = value.f;
				} else {
					vertLookAngle = value.f;
				}
			}
		}
		return 1;
	}
	return 0;
}

void ui_prj_postload()
{
	activeMouseState->x = 1.0f; /*to force update*/
	ui_do_mouse_movement();
}

int ui_is_cursor_hovering_any_window()
{
	return
		(active_window != NULL &&
			ui_element_is_hovered((void*) active_window)) ||
		(context_menu_active &&
			ui_element_is_hovered((void*) context_menu)) ||
		ui_element_is_hovered((void*) main_menu);
}

void ui_set_trapped_in_ui(int flag)
{
	trapped_in_ui = flag;
}
