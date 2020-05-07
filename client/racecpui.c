/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ui.h"
#include "objects.h"
#include "msgbox.h"
#include "racecp.h"
#include "racecpui.h"
#include "sockets.h"
#include "../shared/serverlink.h"
#include <stdio.h>
#include <math.h>

enum manipulateobject_mode {
	MODE_NONE,
	MODE_ORIGIN,
	MODE_RADIUS,
	MODE_DIRECTION,
};

enum manipulateobject_mode movemode;

static struct UI_WINDOW *window_cpsettings;

static struct UI_BUTTON *btn_mainmenu_cplist;
static struct UI_BUTTON *btn_contextmenu_mkracecp;
static struct RADIOBUTTONGROUP *rdbgroup_cptype;
static struct UI_COLORPICKER *cp_colpick;
static struct UI_INPUT *in_coord_x;
static struct UI_INPUT *in_coord_y;
static struct UI_INPUT *in_coord_z;
static struct UI_INPUT *in_radius;
static ui_method *proc_cpsettings_draw;

static int editingCheckpoint;

static
void update_inputs_origin()
{
	char buf[20];

	sprintf(buf, "%.4f", racecheckpoints[editingCheckpoint].pos.x);
	ui_in_set_text(in_coord_x, buf);
	sprintf(buf, "%.4f", racecheckpoints[editingCheckpoint].pos.y);
	ui_in_set_text(in_coord_y, buf);
	sprintf(buf, "%.4f", racecheckpoints[editingCheckpoint].pos.z);
	ui_in_set_text(in_coord_z, buf);
}

static
void update_inputs_radius()
{
	char buf[20];

	sprintf(buf, "%.4f", racecheckpoints[editingCheckpoint].fRadius);
	ui_in_set_text(in_radius, buf);
}

static
void cb_btn_mkracecp(struct UI_BUTTON *btn)
{
	struct RwV3D posToCreate;
	float x, y;

	if (numcheckpoints == MAX_RACECHECKPOINTS) {
		msg_title = "Race_cp";
		msg_message = "Max_race_checkpoints_reached";
		msg_btn1text = "Ok";
		msg_show(NULL);
		return;
	}

	if (clicked_entity) {
		posToCreate = clicked_colpoint.pos;
	} else {
		x = fresx / 2.0f;
		y = fresy / 2.0f;
		game_ScreenToWorld(&posToCreate, x, y, 40.0f);
	}

	movemode = MODE_NONE;
	editingCheckpoint = numcheckpoints;
	racecheckpoints[editingCheckpoint].used = 1;
	racecheckpoints[editingCheckpoint].free = 0;
	racecheckpoints[editingCheckpoint].pos = posToCreate;
	racecheckpoints[editingCheckpoint].fRadius = 2.0f;
	racecheckpoints[editingCheckpoint].arrowDirection.x = 10.0f;
	racecheckpoints[editingCheckpoint].arrowDirection.y = 0.0f;
	racecheckpoints[editingCheckpoint].arrowDirection.z = 0.0f;
	racecheckpoints[editingCheckpoint].colABGR = cp_colpick->last_selected_colorABGR;
	ui_rdb_click_match_userdata(rdbgroup_cptype, (void*) 0);
	numcheckpoints++;
	update_inputs_origin();
	update_inputs_radius();

	ui_show_window(window_cpsettings);
}

static
void cb_btn_move(struct UI_BUTTON *btn)
{
	struct MSG_NC nc;

	movemode = MODE_ORIGIN;
	game_ObjectSetPos(manipulateEntity, &racecheckpoints[editingCheckpoint].pos);
	game_ObjectSetHeadingRad(manipulateEntity, 0.0f);
	nc._parent.id = MAPEDIT_MSG_NATIVECALL;
	nc._parent.data = 0;
	nc.nc = NC_SetObjectRot;
	nc.params.asint[1] = manipulateObject.samp_objectid;
	nc.params.asflt[2] = 0.0f;
	nc.params.asflt[3] = 0.0f;
	nc.params.asflt[4] = 0.0f;
	sockets_send(&nc, sizeof(nc));
	nc._parent.id = MAPEDIT_MSG_NATIVECALL;
	nc._parent.data = 0;
	nc.nc = NC_EditObject;
	nc.params.asint[1] = 0;
	nc.params.asint[2] = manipulateObject.samp_objectid;
	sockets_send(&nc, sizeof(nc));
}

static
void cb_btn_move_radius(struct UI_BUTTON *btn)
{
	struct MSG_NC nc;
	struct RwV3D pos;

	movemode = MODE_RADIUS;
	pos = racecheckpoints[editingCheckpoint].pos;
	pos.x += racecheckpoints[editingCheckpoint].fRadius;
	game_ObjectSetPos(manipulateEntity, &pos);
	game_ObjectSetHeadingRad(manipulateEntity, 0.0f);
	nc._parent.id = MAPEDIT_MSG_NATIVECALL;
	nc._parent.data = 0;
	nc.nc = NC_SetObjectRot;
	nc.params.asint[1] = manipulateObject.samp_objectid;
	nc.params.asflt[2] = 0.0f;
	nc.params.asflt[3] = 0.0f;
	nc.params.asflt[4] = 0.0f;
	sockets_send(&nc, sizeof(nc));
	nc._parent.id = MAPEDIT_MSG_NATIVECALL;
	nc._parent.data = 0;
	nc.nc = NC_EditObject;
	nc.params.asint[1] = 0;
	nc.params.asint[2] = manipulateObject.samp_objectid;
	sockets_send(&nc, sizeof(nc));
}

static
void cb_in_origin(struct UI_INPUT *in)
{
	*(&racecheckpoints[editingCheckpoint].pos.x + (int) in->_parent.userdata) = (float) atof(in->value);
}

static
void cb_in_radius(struct UI_INPUT *in)
{
	racecheckpoints[editingCheckpoint].fRadius = (float) atof(in->value);
	racecheckpoints[editingCheckpoint].used = 0;
	racecheckpoints[editingCheckpoint].free = 1;
}

static
void cb_btn_cplist(struct UI_BUTTON *btn)
{
}

static
void cb_rdbgroup_cptype(struct UI_RADIOBUTTON *rdb)
{
	racecheckpoints[editingCheckpoint].type = (char) rdb->_parent._parent.userdata;
}

static
void cb_cp_cpcol(struct UI_COLORPICKER *colpick)
{
	racecheckpoints[editingCheckpoint].colABGR = colpick->last_selected_colorABGR;
	racecheckpoints[editingCheckpoint].used = 0;
	racecheckpoints[editingCheckpoint].free = 1;
}

static
void cb_btn_cpreset(struct UI_BUTTON *btn)
{
	struct RwV3D pos;

	/*TODO: this does horrible things to the arrow etc*/
	pos = racecheckpoints[editingCheckpoint].pos;
	memset(racecheckpoints + editingCheckpoint, 0, sizeof(struct CRaceCheckpoint));
	racecheckpoints[editingCheckpoint].pos = pos;
	racecheckpoints[editingCheckpoint].arrowDirection.x = 0.0f;
	racecheckpoints[editingCheckpoint].arrowDirection.y = 0.0f;
	racecheckpoints[editingCheckpoint].arrowDirection.z = 0.0f;
	racecheckpoints[editingCheckpoint].colABGR = 0xFFFF0000;
	racecheckpoints[editingCheckpoint].used = 1;
	racecheckpoints[editingCheckpoint].free = 0;
	racecheckpoints[editingCheckpoint].fRadius = 10.0f;
	ui_rdb_click_match_userdata(rdbgroup_cptype, (void*) 0);
}

static
int draw_window_cpsettings(struct UI_ELEMENT *wnd)
{
	struct RwV3D pos;
	float dx, dy, oldrad;

	if (movemode == MODE_ORIGIN) {
		if (ui_active_element == in_coord_x ||
			ui_active_element == in_coord_y ||
			ui_active_element == in_coord_z)
		{
			movemode = MODE_NONE;
			goto skip;
		}
		game_ObjectGetPos(manipulateEntity, &racecheckpoints[editingCheckpoint].pos);
		update_inputs_origin();
	} else if (movemode == MODE_RADIUS) {
		if (ui_active_element == in_radius) {
			movemode = MODE_NONE;
			goto skip;
		}
		game_ObjectGetPos(manipulateEntity, &pos);
		dx = pos.x - racecheckpoints[editingCheckpoint].pos.x;
		dy = pos.y - racecheckpoints[editingCheckpoint].pos.y;
		oldrad = racecheckpoints[editingCheckpoint].fRadius;
		racecheckpoints[editingCheckpoint].fRadius = (float) sqrt(dx * dx + dy * dy);
		if (fabs(oldrad - racecheckpoints[editingCheckpoint].fRadius) > 0.01f) {
			racecheckpoints[editingCheckpoint].used = 0;
			racecheckpoints[editingCheckpoint].free = 1;
		}
		update_inputs_radius();
	}
skip:
	return proc_cpsettings_draw(wnd);
}

void racecpui_init()
{
	struct UI_BUTTON *btn;
	struct UI_LABEL *lbl;
	struct UI_RADIOBUTTON *rdb;

	/*context menu entry*/
	btn = ui_btn_make("Make_race_CP", cb_btn_mkracecp);
	ui_wnd_add_child(context_menu, btn);
	btn->enabled = 0;
	btn_contextmenu_mkracecp = btn;

	/*main menu entry*/
	lbl = ui_lbl_make("=_Race_CPs_=");
	lbl->_parent.span = 2;
	ui_wnd_add_child(main_menu, lbl);
	btn = ui_btn_make("List", cb_btn_cplist);
	btn->_parent.span = 2;
	ui_wnd_add_child(main_menu, btn);
	btn->enabled = 0;
	btn_mainmenu_cplist = btn;

	/*checkpoint settings window*/
	window_cpsettings = ui_wnd_make(9000.0f, 500.0f, "Checkpoint");
	window_cpsettings->columns = 4;
	proc_cpsettings_draw = window_cpsettings->_parent._parent.proc_draw;
	window_cpsettings->_parent._parent.proc_draw = draw_window_cpsettings;
	ui_wnd_add_child(window_cpsettings, ui_lbl_make("Origin:"));
	in_coord_x = ui_in_make(cb_in_origin);
	in_coord_x->_parent.span = 3;
	in_coord_x->_parent.userdata = (void*) 0;
	ui_wnd_add_child(window_cpsettings, in_coord_x);
	ui_wnd_add_child(window_cpsettings, NULL);
	in_coord_y = ui_in_make(cb_in_origin);
	in_coord_y->_parent.span = 3;
	in_coord_y->_parent.userdata = (void*) 1;
	ui_wnd_add_child(window_cpsettings, in_coord_y);
	ui_wnd_add_child(window_cpsettings, NULL);
	in_coord_z = ui_in_make(cb_in_origin);
	in_coord_z->_parent.span = 3;
	in_coord_z->_parent.userdata = (void*) 2;
	ui_wnd_add_child(window_cpsettings, in_coord_z);
	ui_wnd_add_child(window_cpsettings, NULL);
	btn = ui_btn_make("Move", cb_btn_move);
	btn->_parent.span = 3;
	ui_wnd_add_child(window_cpsettings, btn);
	ui_wnd_add_child(window_cpsettings, ui_lbl_make("Radius:"));
	in_radius = ui_in_make(cb_in_radius);
	in_radius->_parent.span = 3;
	ui_wnd_add_child(window_cpsettings, in_radius);
	ui_wnd_add_child(window_cpsettings, NULL);
	btn = ui_btn_make("Move", cb_btn_move_radius);
	btn->_parent.span = 3;
	ui_wnd_add_child(window_cpsettings, btn);
	ui_wnd_add_child(window_cpsettings, ui_lbl_make("Type:"));
	rdbgroup_cptype = ui_rdbgroup_make(cb_rdbgroup_cptype);
	rdb = ui_rdb_make("0._Arrow", rdbgroup_cptype, 1);
	rdb->_parent._parent.span = 3;
	rdb->_parent._parent.userdata = 0;
	ui_wnd_add_child(window_cpsettings, rdb);
	ui_wnd_add_child(window_cpsettings, NULL);
	rdb = ui_rdb_make("1._Finish", rdbgroup_cptype, 0);
	rdb->_parent._parent.span = 3;
	rdb->_parent._parent.userdata = (void*) 1;
	ui_wnd_add_child(window_cpsettings, rdb);
	ui_wnd_add_child(window_cpsettings, NULL);
	rdb = ui_rdb_make("2._Normal", rdbgroup_cptype, 0);
	rdb->_parent._parent.span = 3;
	rdb->_parent._parent.userdata = (void*) 2;
	ui_wnd_add_child(window_cpsettings, rdb);
	ui_wnd_add_child(window_cpsettings, NULL);
	rdb = ui_rdb_make("3._Air_normal", rdbgroup_cptype, 0);
	rdb->_parent._parent.span = 3;
	rdb->_parent._parent.userdata = (void*) 3;
	ui_wnd_add_child(window_cpsettings, rdb);
	ui_wnd_add_child(window_cpsettings, NULL);
	rdb = ui_rdb_make("4._Air_finish", rdbgroup_cptype, 0);
	rdb->_parent._parent.span = 3;
	rdb->_parent._parent.userdata = (void*) 4;
	ui_wnd_add_child(window_cpsettings, rdb);
	ui_wnd_add_child(window_cpsettings, NULL);
	rdb = ui_rdb_make("5._Air_rotate", rdbgroup_cptype, 0);
	rdb->_parent._parent.span = 3;
	rdb->_parent._parent.userdata = (void*) 5;
	ui_wnd_add_child(window_cpsettings, rdb);
	ui_wnd_add_child(window_cpsettings, NULL);
	rdb = ui_rdb_make("6._Air_up_down_nothing", rdbgroup_cptype, 0);
	rdb->_parent._parent.span = 3;
	rdb->_parent._parent.userdata = (void*) 6;
	ui_wnd_add_child(window_cpsettings, rdb);
	ui_wnd_add_child(window_cpsettings, NULL);
	rdb = ui_rdb_make("7._Air_up_down_1", rdbgroup_cptype, 0);
	rdb->_parent._parent.span = 3;
	rdb->_parent._parent.userdata = (void*) 7;
	ui_wnd_add_child(window_cpsettings, rdb);
	ui_wnd_add_child(window_cpsettings, NULL);
	rdb = ui_rdb_make("8._Air_up_down_2", rdbgroup_cptype, 0);
	rdb->_parent._parent.span = 3;
	rdb->_parent._parent.userdata = (void*) 8;
	ui_wnd_add_child(window_cpsettings, rdb);
	lbl = ui_lbl_make("Color:");
	ui_wnd_add_child(window_cpsettings, lbl);
	cp_colpick = ui_colpick_make(100.0f, cb_cp_cpcol);
	cp_colpick->_parent.span = 3;
	ui_wnd_add_child(window_cpsettings, cp_colpick);
	btn = ui_btn_make("Reset", cb_btn_cpreset);
	btn->_parent.span = 4;
	ui_wnd_add_child(window_cpsettings, btn);
}

void racecpui_dispose()
{
	ui_wnd_dispose(window_cpsettings);
}

void racecpui_prj_postload()
{
	btn_mainmenu_cplist->enabled = 1;
	btn_contextmenu_mkracecp->enabled = 1;
}
