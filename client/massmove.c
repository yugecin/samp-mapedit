/* vim: set filetype=c ts=8 noexpandtab: */

#include "bulkedit.h"
#include "bulkeditui.h"
#include "common.h"
#include "game.h"
#include "gangzone.h"
#include "entity.h"
#include "ide.h"
#include "racecp.h"
#include "objects.h"
#include "objectseditor.h"
#include "ui.h"
#include "msgbox.h"
#include "vehicles.h"
#include <string.h>
#include <math.h>

static struct UI_WINDOW *massmove_wnd;
static struct UI_INPUT *in_x, *in_y, *in_z, *in_rot;

static
void cb_btn_mainmenu_massmove(struct UI_BUTTON *btn)
{
	ui_show_window(massmove_wnd);
}

static
void cb_btn_do(struct UI_BUTTON *btn)
{
	int layer;
	int i;
	struct RwV3D total;
	int count;
	struct RwV3D move;
	float moveangleDeg;
	float moveangle;
	float distance;
	float dx, dy;
	float angle;

	total.x = total.y = total.z = 0.0f;

	count = 0;
	for (layer = 0; layer < numlayers; layer++) {
		for (i = 0; i < layers[layer].numobjects; i++) {
			total.x += layers[layer].objects[i].pos.x;
			total.y += layers[layer].objects[i].pos.y;
			total.z += layers[layer].objects[i].pos.z;
			count++;
		}
	}

	for (i = 0; i < numvehicles; i++) {
		total.x += vehicles[i].pos.x;
		total.y += vehicles[i].pos.y;
		total.z += vehicles[i].pos.z;
		count++;
	}

	if (count == 0) {
		return;
	}

	total.x /= count;
	total.y /= count;
	total.z /= count;

	move.x = (float) atof(in_x->value);
	move.y = (float) atof(in_y->value);
	move.z = (float) atof(in_z->value);
	moveangleDeg = -(float) atof(in_rot->value);
	moveangle = moveangleDeg * M_PI / 180.0f;

	for (i = 0; i < numcheckpoints; i++) {
		if (moveangle != 0.0f) {
			dx = total.x - racecheckpoints[i].pos.x;
			dy = total.y - racecheckpoints[i].pos.y;
			distance = (float) sqrt(dx * dx + dy * dy);
			angle = (float) atan2(dy, dx);
			angle -= moveangle;
			racecheckpoints[i].pos.x = total.x - (float) cos(angle) * distance;
			racecheckpoints[i].pos.y = total.y - (float) sin(angle) * distance;
		}
		racecheckpoints[i].pos.x += move.x;
		racecheckpoints[i].pos.y += move.y;
		racecheckpoints[i].pos.z += move.z;
	}

	for (layer = 0; layer < numlayers; layer++) {
		for (i = 0; i < layers[layer].numobjects; i++) {
			if (moveangle != 0.0f) {
				dx = total.x - layers[layer].objects[i].pos.x;
				dy = total.y - layers[layer].objects[i].pos.y;
				distance = (float) sqrt(dx * dx + dy * dy);
				angle = (float) atan2(dy, dx);
				angle -= moveangle;
				layers[layer].objects[i].pos.x = total.x - (float) cos(angle) * distance;
				layers[layer].objects[i].pos.y = total.y - (float) sin(angle) * distance;
				layers[layer].objects[i].rot.z -= moveangleDeg;
			}
			layers[layer].objects[i].pos.x += move.x;
			layers[layer].objects[i].pos.y += move.y;
			layers[layer].objects[i].pos.z += move.z;
		}
	}

	for (i = 0; i < numvehicles; i++) {
		if (moveangle != 0.0f) {
			dx = total.x - vehicles[i].pos.x;
			dy = total.y - vehicles[i].pos.y;
			distance = (float) sqrt(dx * dx + dy * dy);
			angle = (float) atan2(dy, dx) - moveangle;
			vehicles[i].pos.x = total.x - (float) cos(angle) * distance;
			vehicles[i].pos.y = total.y - (float) sin(angle) * distance;
		}
		vehicles[i].pos.x += move.x;
		vehicles[i].pos.y += move.y;
		vehicles[i].pos.z += move.z;
		vehicles[i].heading -= moveangle;
	}

	for (i = 0; i < numgangzones; i++) {
		if (moveangle != 0.0f && i == 0) {
			sprintf(debugstring, "gangzones do not rotate yet!!");
			ui_push_debug_string();
		}
		gangzone_data[i].minx += move.x;
		gangzone_data[i].miny += move.y;
		gangzone_data[i].maxx += move.x;
		gangzone_data[i].maxy += move.y;
	}
}

void massmove_init()
{
	struct UI_BUTTON *btn;
	struct UI_LABEL *lbl;

	btn = ui_btn_make("Massmove", cb_btn_mainmenu_massmove);
	btn->_parent.span = 2;
	ui_wnd_add_child(main_menu, btn);

	massmove_wnd = ui_wnd_make(400.0f, 300.0f, "Massmove");
	massmove_wnd->columns = 2;

	ui_wnd_add_child(massmove_wnd, lbl = ui_lbl_make("Hide_all_layer_before_changing"));
	lbl->_parent.span = 2;
	ui_wnd_add_child(massmove_wnd, lbl = ui_lbl_make("Make_a_backup_first_or_be_a_lama"));
	lbl->_parent.span = 2;
	ui_wnd_add_child(massmove_wnd, lbl = ui_lbl_make("move_x"));
	ui_wnd_add_child(massmove_wnd, in_x = ui_in_make(NULL));
	ui_wnd_add_child(massmove_wnd, lbl = ui_lbl_make("move_y"));
	ui_wnd_add_child(massmove_wnd, in_y = ui_in_make(NULL));
	ui_wnd_add_child(massmove_wnd, lbl = ui_lbl_make("move_z"));
	ui_wnd_add_child(massmove_wnd, in_z = ui_in_make(NULL));
	ui_wnd_add_child(massmove_wnd, lbl = ui_lbl_make("rotate_z"));
	ui_wnd_add_child(massmove_wnd, in_rot = ui_in_make(NULL));
	ui_wnd_add_child(massmove_wnd, btn = ui_btn_make("Move", cb_btn_do));
	btn->_parent.span = 2;
}

void massmove_dispose()
{
	ui_wnd_dispose(massmove_wnd);
}
