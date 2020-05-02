/* vim: set filetype=c ts=8 noexpandtab: */

#include "carcols.h"
#include "common.h"
#include "game.h"
#include "msgbox.h"
#include "objects.h"
#include "ui.h"
#include "sockets.h"
#include "vehicles.h"
#include "vehicleseditor.h"
#include "../shared/serverlink.h"

static struct UI_WINDOW *wnd;
static struct UI_WINDOW *wnd_colpick;
static struct VEHICLE *editingVehicle;
static char *editingColor;
static void (*callback)();

static
void cb_btn_random_default_color(struct UI_BUTTON *btn)
{
	char *cols;

	cols = vehicles_get_rand_color(editingVehicle->model);
	editingVehicle->col[0] = *cols;
	editingVehicle->col[1] = *(cols + 1);
	vehicles_update_color(editingVehicle);
}

static
void cb_btn_random_color(struct UI_BUTTON *btn)
{
	editingVehicle->col[0] = randMax65535() % 127;
	editingVehicle->col[1] = randMax65535() % 127;
	vehicles_update_color(editingVehicle);
}

static
void cb_btn_move(struct UI_BUTTON *btn)
{
	struct MSG_NC nc;

	nc._parent.id = MAPEDIT_MSG_NATIVECALL;
	nc._parent.data = 0;
	nc.nc = NC_EditObject;
	nc.params.asint[1] = 0;
	nc.params.asint[2] = manipulateObject.samp_objectid;
	sockets_send(&nc, sizeof(nc));
}

static
void cb_msg_delete_confirm(int opt)
{
	if (opt == MSGBOX_RESULT_1) {
		ui_hide_window();
		vehicles_delete(editingVehicle);
	}
}

static
void cb_btn_delete(struct UI_BUTTON *btn)
{
	msg_title = "Vehicle";
	msg_message = "Delete_vehicle?";
	msg_message2 = "This_cannot_be_undone!";
	msg_btn1text = "Yes";
	msg_btn2text = "No";
	msg_show(cb_msg_delete_confirm);
}

static
void cb_btn_clone(struct UI_BUTTON *btn)
{
	struct VEHICLE *newveh;

	newveh = vehicles_create(editingVehicle->model, &editingVehicle->pos);
	newveh->heading = editingVehicle->heading;
	newveh->col[0] = editingVehicle->col[0];
	newveh->col[1] = editingVehicle->col[1];
	vehicles_update_color(newveh);
	vehedit_show(newveh, callback);
}

static
void cb_btn_close(struct UI_BUTTON *btn)
{
	if (callback != NULL) {
		callback();
	} else {
		ui_hide_window();
	}
}

static
void cb_btn_editcol(struct UI_BUTTON *btn)
{
	editingColor = editingVehicle->col + (int) btn->_parent.userdata;
	ui_show_window(wnd_colpick);
}

void vehedit_frame_update()
{
	if (ui_get_active_window() != wnd &&
		ui_get_active_window() != wnd_colpick)
	{
		return;
	}
	game_ObjectGetPos(manipulateEntity, &editingVehicle->pos);
	game_ObjectGetHeadingRad(manipulateEntity, &editingVehicle->heading);
}

void vehedit_show(struct VEHICLE *veh, void (*cb)())
{
	editingVehicle = veh;
	game_ObjectSetPos(manipulateEntity, &veh->pos);
	game_ObjectSetHeadingRad(manipulateEntity, veh->heading);
	ui_show_window(wnd);
	callback = cb;
}

static
void draw_color_btn(struct UI_ELEMENT *e, int col)
{
	if (ui_element_is_hovered(e)) {
		ui_element_draw_background(e, -1);
		game_DrawRect(
			e->x + 2,
			e->y + 2,
			e->width - 4,
			e->height - 4,
			col);
	} else {
		ui_element_draw_background(e, col);
	}
}

static
void proc_draw_livecolor(struct UI_ELEMENT *e)
{
	int col;

	col = actualcarcols[editingVehicle->col[(int) e->userdata]];
	draw_color_btn(e, col);
}

static
void proc_draw_color(struct UI_ELEMENT *e)
{
	draw_color_btn(e, actualcarcols[(int) e->userdata]);
}

static
void cb_btn_colpick_col(struct UI_BUTTON *btn)
{
	*editingColor = (char) btn->_parent.userdata;
	vehicles_update_color(editingVehicle);
}

static
void cb_btn_colpick_close(struct UI_BUTTON *btn)
{
	ui_show_window(wnd);
}

void vehedit_init()
{
	struct UI_LABEL *lbl;
	struct UI_BUTTON *btn;
	int i;

	wnd = ui_wnd_make(5000.0f, 300.0f, "Vehicle");
	wnd->columns = 3;
	wnd->closeable = 0;

	lbl = ui_lbl_make("Colors:");
	ui_wnd_add_child(wnd, lbl);
	btn = ui_btn_make(NULL, cb_btn_editcol);
	btn->_parent.proc_draw = (ui_method*) proc_draw_livecolor;
	btn->_parent.userdata = (void*) 0;
	ui_wnd_add_child(wnd, btn);
	btn = ui_btn_make(NULL, cb_btn_editcol);
	btn->_parent.proc_draw = (ui_method*) proc_draw_livecolor;
	btn->_parent.userdata = (void*) 1;
	ui_wnd_add_child(wnd, btn);
	btn = ui_btn_make("Random_default_color", cb_btn_random_default_color);
	btn->_parent.span = 3;
	ui_wnd_add_child(wnd, btn);
	btn = ui_btn_make("Random_color", cb_btn_random_color);
	btn->_parent.span = 3;
	ui_wnd_add_child(wnd, btn);
	btn = ui_btn_make("Move", cb_btn_move);
	btn->_parent.span = 3;
	ui_wnd_add_child(wnd, btn);
	btn = ui_btn_make("Delete", cb_btn_delete);
	btn->_parent.span = 3;
	ui_wnd_add_child(wnd, btn);
	btn = ui_btn_make("Clone", cb_btn_clone);
	btn->_parent.span = 3;
	ui_wnd_add_child(wnd, btn);
	btn = ui_btn_make("Close", cb_btn_close);
	btn->_parent.span = 3;
	ui_wnd_add_child(wnd, btn);

	wnd_colpick = ui_wnd_make(400.0f, 300.0f, "Vehicle_colors");
	wnd_colpick->columns = 8;
	wnd_colpick->closeable = 0;

	for (i = 0; i < 128; i++) {
		btn = ui_btn_make(NULL, cb_btn_colpick_col);
		btn->_parent.pref_width = 20.0f;
		btn->_parent.pref_height = 20.0f;
		btn->_parent.proc_recalc_size = ui_elem_dummy_proc;
		btn->_parent.userdata = (void*) i;
		btn->_parent.proc_draw = (ui_method*) proc_draw_color;
		ui_wnd_add_child(wnd_colpick, btn);
	}
	btn = ui_btn_make("Close", cb_btn_colpick_close);
	btn->_parent.span = 8;
	ui_wnd_add_child(wnd_colpick, btn);
}

void vehedit_dispose()
{
	ui_wnd_dispose(wnd);
	ui_wnd_dispose(wnd_colpick);
}
