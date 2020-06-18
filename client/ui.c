/* vim: set filetype=c ts=8 noexpandtab: */

#include "bulkedit.h"
#include "bulkeditui.h"
#include "common.h"
#include "client.h"
#include "detours.h"
#include "entity.h"
#include "game.h"
#include "ide.h"
#include "objbrowser.h"
#include "objects.h"
#include "objectsui.h"
#include "objectlistui.h"
#include "persistence.h"
#include "player.h"
#include "project.h"
#include "msgbox.h"
#include "racecp.h"
#include "racecpui.h"
#include "removebuildingeditor.h"
#include "samp.h"
#include "ui.h"
#include "vehicles.h"
#include "vehiclesui.h"
#include "vehicleseditor.h"
#include "vehnames.h"
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
void (*ui_exclusive_mode)() = NULL;
struct RwV3D *player_position_for_camera = &player_position;

static char active = 0;
static float originalHudScaleX, originalHudScaleY;
static float horizLookAngle, vertLookAngle;
static char need_camera_update, has_set_camera_once;
static int context_menu_active = 0;
static int trapped_in_ui;
static int speedmod = 3;
static float speeds[] = { 0.1f, 0.2f, 0.5f, 1.0f, 3.0f, 8.0f, 16.0f, 32.0f };

struct CEntity *clicked_entity;
struct CColPoint clicked_colpoint;
struct CEntity *hovered_entity;
struct CColPoint hovered_colpoint;
/**
Follows clicked_entity, but set to NULL as soon the context menu is closed.
*/
struct CEntity *highlighted_entity;

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
	persistence_set_cursorpos(cursorx, cursory);
}

static
void cb_chk_showwater(struct UI_CHECKBOX *chk)
{
	showWater = chk->checked;
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
	struct UI_CHECKBOX *chk;

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

	main_menu = ui_wnd_make(10.0f, 380.0f, "Main_Menu");
	main_menu->columns = 2;
	main_menu->closeable = 0;

	btn = ui_btn_make("Reload_client", cb_btn_reload);
	btn->_parent.span = 2;
	ui_wnd_add_child(main_menu, btn);

	chk = ui_chk_make("Show water", 1, cb_chk_showwater);
	chk->_parent._parent.span = 2;
	ui_wnd_add_child(main_menu, chk);
}

void ui_show_window(struct UI_WINDOW *wnd)
{
	active_window = wnd;
	ui_active_element = NULL;
	wnd->_parent._parent.proc_recalc_size(wnd);
	/*recalc_size queues layout update, so not needed here anymore*/
	objects_on_active_window_changed(wnd);
	objlistui_on_active_window_changed(wnd);
	vehiclesui_on_active_window_changed(wnd);
	racecpui_on_active_window_changed(wnd);
}

void ui_hide_window()
{
	active_window = NULL;
	ui_active_element = NULL;
	objects_on_active_window_changed(NULL);
	objlistui_on_active_window_changed(NULL);
}

struct UI_WINDOW *ui_get_active_window()
{
	return active_window;
}

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

void ui_draw_default_help_text()
{
	char buf[64];
	int i;

	game_TextSetAlign(CENTER);
	game_TextPrintStringFromBottom(fresx / 2.0f, fresy - 2.0f,
		"~w~press ~r~Y ~w~to reset camera");
	sprintf(buf, "Movement_speed_(scrollwheel):_-------");
	for (i = 0; i < speedmod; i++) {
		buf[30 + i] = '+';
	}
	game_TextPrintStringFromBottom(
		fresx / 2.0f,
		fresy - fontheight - 4.0f,
		buf);
	sprintf(buf,
		"x%.2f y%.2f z%.2f",
		camera->position.x,
		camera->position.y,
		camera->position.z);
	game_TextPrintStringFromBottom(
		fresx / 2.0f,
		fresy - fontheight - fontheight - 6.0f,
		buf);
}

void ui_draw_cursor()
{
	/* (inner|outer)(width|height)(radius|diam)*/
	float iwr, iwd, ihr, ihd;
	float owr, owd, ohr, ohd;
	float typex, detailx;

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

	if (hovered_entity && !ui_is_cursor_hovering_any_window()) {
		typex = fresx / 2.0f - 5.0f;
		detailx = fresx / 2.0f + 5.0f;
		if (ENTITY_IS_TYPE(hovered_entity, ENTITY_TYPE_VEHICLE)) {
			game_TextSetAlign(RIGHT);
			game_TextPrintString(typex, 3.0f, "~g~Vehicle");
			game_TextSetAlign(LEFT);
			game_TextPrintString(detailx, 3.0f, vehnames[hovered_entity->model - 400]);
		} else if (ENTITY_IS_TYPE(hovered_entity, ENTITY_TYPE_PED)) {
			game_TextSetAlign(RIGHT);
			game_TextPrintString(typex, 3.0f, "~b~Ped");
		} else if (ENTITY_IS_TYPE(hovered_entity, ENTITY_TYPE_OBJECT)) {
			game_TextSetAlign(RIGHT);
			game_TextPrintString(typex, 3.0f, "~y~Object");
			game_TextSetAlign(LEFT);
			if (modelNames[hovered_entity->model] != NULL) {
				game_TextPrintString(detailx, 3.0f, modelNames[hovered_entity->model]);
			}
		} else if (ENTITY_IS_TYPE(hovered_entity, ENTITY_TYPE_BUILDING) ||
			ENTITY_IS_TYPE(hovered_entity, ENTITY_TYPE_DUMMY))
		{
			game_TextSetAlign(RIGHT);
			game_TextPrintString(typex, 3.0f, "~r~Building");
			game_TextSetAlign(LEFT);
			if (modelNames[hovered_entity->model] != NULL) {
				game_TextPrintString(detailx, 3.0f, modelNames[hovered_entity->model]);
			}
		}
	}
}

void ui_update_camera_after_manual_position()
{
	camera->cams[camera->activeCam].pos = camera->position;
	camera->cams[camera->activeCam].lookVector = camera->lookVector;
	ui_update_camera();
}

void ui_update_camera()
{
	camera->lookAt = camera->position;
	camera->lookAt.x += camera->lookVector.x;
	camera->lookAt.y += camera->lookVector.y;
	camera->lookAt.z += camera->lookVector.z;
	game_CameraSetOnPoint(&camera->lookAt, CUT, 1);
}

void ui_store_camera()
{
	struct CCam *ccam;
	struct RwV3D lookVec;
	float xylen;

	ccam = &camera->cams[camera->activeCam];
	camera->position = ccam->pos;
	camera->lookVector = ccam->lookVector;
	lookVec = camera->lookVector;
	horizLookAngle = atan2f(lookVec.y, lookVec.x);
	xylen = sqrtf(lookVec.x * lookVec.x + lookVec.y * lookVec.y);
	vertLookAngle = atan2f(xylen, lookVec.z);
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
		objui_ui_activated();
		vehicles_ui_activated();
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
		objui_ui_deactivated();
		vehicles_ui_deactivated();
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
		context_menu->_parent._parent.proc_recalc_size(context_menu);
		main_menu->_parent._parent.proc_recalc_size(main_menu);
		if (bulkeditui_wnd) {
			bulkeditui_wnd->_parent._parent.proc_recalc_size(bulkeditui_wnd);
		}
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
	struct RwV3D lookVec;
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
		lookVec.z = cosf(vertLookAngle);
		xylen = sinf(vertLookAngle);
		lookVec.x = cosf(horizLookAngle) * xylen;
		lookVec.y = sinf(horizLookAngle) * xylen;
		camera->lookVector = lookVec;
		need_camera_update = 1;
	}
}

static
void ui_do_key_movement()
{
	float speed = 1.0f, angle, xylen;

	speed = speeds[speedmod];
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

	game_TextSetAlign(LEFT);
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

void ui_grablastkey()
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
	if (!(active_window != NULL &&
		UIPROC(active_window, proc_mousewheel, (void*) value)) &&
		!ui_wnd_mousewheel(main_menu, value) &&
		!ui_cnt_mousewheel(background_element, value))
	{
		speedmod += value;
		if (speedmod < 0) {
			speedmod = 0;
		} else if (speedmod > 7) {
			speedmod = 7;
		}
	}
}

void ui_get_entity_pointed_at(void **entity, struct CColPoint *colpoint)
{
	struct RwV3D target, from;

	from = camera->position;
	game_ScreenToWorld(&target, cursorx, cursory, 700.0f);
	if (!game_WorldIntersectEntity(&from, &target, colpoint, entity)) {
		*entity = NULL;
	}
}

void ui_get_clicked_position(struct RwV3D *pos)
{
	if (clicked_entity) {
		*pos = clicked_colpoint.pos;
	} else {
		game_ScreenToWorld(pos, bgclickx, bgclicky, 60.0f);
	}
}

static
void background_element_just_clicked()
{
	ui_get_entity_pointed_at(&clicked_entity, &clicked_colpoint);

	if (objui_on_background_element_just_clicked() &&
		player_on_background_element_just_clicked() &&
		vehiclesui_on_background_element_just_clicked() &&
		bulkeditui_on_background_element_just_clicked())
	{
		context_menu_active = 1;
		context_menu->_parent._parent.x = cursorx + 10.0f;
		context_menu->_parent._parent.y = cursory + 25.0f;
		context_menu->_parent.need_layout = 1;
		highlighted_entity = clicked_entity;
	}
}

void ui_do_exclusive_mode_basics(struct UI_WINDOW *wnd, int allow_camera_move)
{
	if (ui_element_being_clicked && ui_mouse_is_just_up) {
		wnd && ui_wnd_mouseup(wnd);
		ui_element_being_clicked = NULL;
	}
	if (activeMouseState->rmb && allow_camera_move) {
		ui_active_element = NULL;
		ui_do_mouse_movement();
		ui_do_key_movement();
		context_menu_active = 0;
		if (need_camera_update) {
			ui_update_camera();
			need_camera_update = 0;
		}
	} else {
		ui_do_cursor_movement();
	}
	ui_grablastkey();
	/*
	ui_last_key_down != 0 &&
		ui_active_element != NULL &&
		UIPROC(ui_active_element, proc_accept_key);*/
	if (ui_element_being_clicked == NULL && ui_mouse_is_just_down) {
		ui_active_element = NULL;
		wnd && ui_wnd_mousedown(wnd);
	}

	if (wnd) {
		UIPROC(wnd, proc_update);
		UIPROC(wnd, proc_draw);
	}

	ui_draw_debug_strings();
	ui_draw_cursor();
}

static
void ui_place_camera_behind_player()
{
	float rotation;
	float x, y;

	game_PedGetPos(player, NULL, &rotation);
	rotation = (rotation + 90.0f) * M_PI / 180.0f;
	x = 4.0f * cosf(rotation);
	y = 4.0f * sinf(rotation);
	camera->position.x = player_position_for_camera->x - x;
	camera->position.y = player_position_for_camera->y - y;
	camera->position.z = player_position_for_camera->z + 2.0f;
	camera->lookVector.x = x;
	camera->lookVector.y = y;
	camera->lookVector.z = -0.25f;
}

int ui_handle_keydown(int vk)
{
	if (rbe_handle_keydown(vk)) {
		return 1;
	}
	if (vk == VK_ESCAPE) {
		if (objbrowser_handle_esc()) {
			return 1;
		}
		if (active_window != NULL && active_window->proc_close(active_window)) {
			return 1;
		}
		/*
		Not sure if I want this to hide when pressing ESC
		if (bulkeditui_shown) {
			bulkeditui_shown = 0;
			return 1;
		}
		*/
	}
	return ui_active_element != NULL &&
		UIPROC(ui_active_element, proc_accept_keydown, (void*) vk);
}

int ui_handle_char(char c)
{
	return ui_active_element != NULL &&
		UIPROC(ui_active_element, proc_accept_char, (void*) c);
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
		ui_mouse_is_just_down =
			activeMouseState->lmb && !prevMouseState->lmb;
		ui_mouse_is_just_up =
			prevMouseState->lmb && !activeMouseState->lmb;

		vehicles_frame_update();

		if (highlighted_entity) {
			entity_draw_bound_rect(highlighted_entity, 0xFF);
		}

		bulkedit_draw_object_boxes();

		/*can't use clicked_entity, because it would change here while the user
		is still in the context menu*/
		ui_get_entity_pointed_at(&hovered_entity, &hovered_colpoint);

		if (ui_exclusive_mode != NULL) {
			ui_exclusive_mode();
			return;
		}

		game_PedSetPos(player, &player_position);

		/*this one should be _before_ update and input handling*/
		racecp_frame_update();

		if (ui_element_being_clicked && ui_mouse_is_just_up) {
			if ((context_menu_active &&
				ui_wnd_mouseup(context_menu)) ||
				(active_window != NULL && ui_wnd_mouseup(active_window)) ||
				(bulkeditui_shown && ui_wnd_mouseup(bulkeditui_wnd)) ||
				ui_wnd_mouseup(main_menu) ||
				ui_cnt_mouseup(background_element));
			context_menu_active = 0;
			highlighted_entity = NULL;
			if (ui_element_being_clicked == background_element) {
				background_element_just_clicked();
			}
			ui_element_being_clicked = NULL;
		}

		if (activeMouseState->rmb) {
			ui_do_mouse_movement();
			ui_do_key_movement();
			context_menu_active = 0;
			ui_active_element = NULL;
			highlighted_entity = NULL;
		} else {
			ui_do_cursor_movement();
		}

		ui_grablastkey();

		/*TODO: replace with WM_CHAR handling*/
		if (activeKeyState->standards[VK_Y] &&
			!currentKeyState->standards[VK_Y])
		{
			ui_place_camera_behind_player();
			need_camera_update = 1;
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
			if ((context_menu_active && ui_wnd_mousedown(context_menu)) ||
				(active_window != NULL && ui_wnd_mousedown(active_window)) ||
				(bulkeditui_shown && ui_wnd_mousedown(bulkeditui_wnd)) ||
				ui_wnd_mousedown(main_menu) ||
				ui_cnt_mousedown(background_element))
			{
				;
			} else {
				context_menu_active = 0;
				highlighted_entity = NULL;
			}
		}

		if (active_window != NULL) {
			ui_wnd_update(active_window);
		}
		if (context_menu_active) {
			ui_wnd_update(context_menu);
		}
		if (bulkeditui_shown) {
			ui_wnd_update(bulkeditui_wnd);
		}
		ui_wnd_update(main_menu);
		ui_cnt_update(background_element);

		objui_frame_update();
		objlistui_frame_update();
		vehiclesui_frame_update();

		ui_cnt_draw(background_element);
		ui_wnd_draw(main_menu);
		if (active_window != NULL) {
			UIPROC(active_window, proc_draw);
		}
		if (bulkeditui_shown) {
			UIPROC(bulkeditui_wnd, proc_draw);
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

		ui_draw_default_help_text();
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
	TRACE("ui_dispose");
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
		(active_window != NULL && ui_element_is_hovered((void*) active_window)) ||
		(context_menu_active && ui_element_is_hovered((void*) context_menu)) ||
		ui_element_is_hovered((void*) main_menu) ||
		(bulkeditui_shown && ui_element_is_hovered((void*) bulkeditui_wnd));
}

void ui_set_trapped_in_ui(int flag)
{
	trapped_in_ui = flag;
}

void ui_open_persistent_state()
{
	float cx, cy;

	if (persistence_get_cursorpos(&cx, &cy)) {
		cursorx = cx;
		cursory = cy;
		ui_activate();
	}
}
