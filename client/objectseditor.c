/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ide.h"
#include "msgbox.h"
#include "objects.h"
#include "objbrowser.h"
#include "bulkedit.h"
#include "bulkeditui.h"
#include "objectseditor.h"
#include "objecttextures.h"
#include "ui.h"
#include "sockets.h"
#include "../shared/serverlink.h"

static struct UI_WINDOW *wnd;
static struct UI_INPUT *in_coord_x;
static struct UI_INPUT *in_coord_y;
static struct UI_INPUT *in_coord_z;
static struct UI_INPUT *in_rot_x;
static struct UI_INPUT *in_rot_y;
static struct UI_INPUT *in_rot_z;
static struct UI_BUTTON *btn_add_to_bulkedit;
static struct UI_BUTTON *btn_remove_from_bulkedit;
static struct OBJECT *editingObject;

static char lbl_txt_model[50];
static char updateFromMoving;
static ui_method *proc_draw_window_objedit;
static struct RwV3D initialPos, initialRot;

static
void update_inputs()
{
	struct RwV3D vec;
	char buf[20];

	game_ObjectGetPos(editingObject->sa_object, &vec);
	sprintf(buf, "%.4f", vec.x);
	ui_in_set_text(in_coord_x, buf);
	sprintf(buf, "%.4f", vec.y);
	ui_in_set_text(in_coord_y, buf);
	sprintf(buf, "%.4f", vec.z);
	ui_in_set_text(in_coord_z, buf);
	game_ObjectGetRot(editingObject->sa_object, &vec);
	sprintf(buf, "%.4f", vec.x);
	ui_in_set_text(in_rot_x, buf);
	sprintf(buf, "%.4f", vec.y);
	ui_in_set_text(in_rot_y, buf);
	sprintf(buf, "%.4f", vec.z);
	ui_in_set_text(in_rot_z, buf);
}

static
void cb_in_coords(struct UI_INPUT *in)
{
	struct RwV3D pos;
	struct MSG_NC nc;

	pos.x = (float) atof(in_coord_x->value);
	pos.y = (float) atof(in_coord_y->value);
	pos.z = (float) atof(in_coord_z->value);

	nc._parent.id = MAPEDIT_MSG_NATIVECALL;
	nc.nc = NC_SetObjectPos;
	nc.params.asint[1] = editingObject->samp_objectid;
	nc.params.asflt[2] = pos.x;
	nc.params.asflt[3] = pos.y;
	nc.params.asflt[4] = pos.z;

	game_ObjectSetPos(editingObject->sa_object, &pos);
	bulkedit_update();

	sockets_send(&nc, sizeof(nc));
}

static
void cb_in_rotation(struct UI_INPUT *in)
{
	struct RwV3D rot;
	struct MSG_NC nc;

	rot.x = (float) atof(in_rot_x->value);
	rot.y = (float) atof(in_rot_y->value);
	rot.z = (float) atof(in_rot_z->value);

	nc._parent.id = MAPEDIT_MSG_NATIVECALL;
	nc.nc = NC_SetObjectRot;
	nc.params.asint[1] = editingObject->samp_objectid;
	nc.params.asflt[2] = rot.x;
	nc.params.asflt[3] = rot.y;
	nc.params.asflt[4] = rot.z;

	rot.x *= M_PI / 180.0f;
	rot.y *= M_PI / 180.0f;
	rot.z *= M_PI / 180.0f;
	game_ObjectSetRotRad(editingObject->sa_object, &rot);
	bulkedit_update();

	sockets_send(&nc, sizeof(nc));
}

static
void cb_btn_textures(struct UI_BUTTON *btn)
{
	objecttextures_show(editingObject);
}

static
void cb_btn_objectseditor_move(struct UI_BUTTON *btn)
{
	objedit_move();
}

static
void cb_btn_undo_move(struct UI_BUTTON *btn)
{
	struct RwV3D rot;
	struct MSG_NC nc;

	game_ObjectSetPos(editingObject->sa_object, &initialPos);

	nc._parent.id = MAPEDIT_MSG_NATIVECALL;
	nc.nc = NC_SetObjectRot;
	nc.params.asint[1] = editingObject->samp_objectid;
	nc.params.asflt[2] = initialRot.x;
	nc.params.asflt[3] = initialRot.y;
	nc.params.asflt[4] = initialRot.z;

	rot = initialRot;
	rot.x *= M_PI / 180.0f;
	rot.y *= M_PI / 180.0f;
	rot.z *= M_PI / 180.0f;
	game_ObjectSetRotRad(editingObject->sa_object, &rot);
	bulkedit_update();

	sockets_send(&nc, sizeof(nc));
	update_inputs();
}

static
void cb_btn_tp_to_camera(struct UI_BUTTON *btn)
{
	char buf[20];

	sprintf(buf, "%.4f", camera->position.x);
	ui_in_set_text(in_coord_x, buf);
	sprintf(buf, "%.4f", camera->position.y);
	ui_in_set_text(in_coord_y, buf);
	sprintf(buf, "%.4f", camera->position.z);
	ui_in_set_text(in_coord_z, buf);
	cb_in_coords(NULL);
}

static
void cb_msg_delete_confirm(int opt)
{
	if (opt == MSGBOX_RESULT_1) {
		bulkedit_revert();
		objects_delete_obj(editingObject);
	} else {
		ui_show_window(wnd);
	}
}

static
void cb_btn_delete(struct UI_BUTTON *btn)
{
	msg_title = "Object";
	msg_message = "Delete_object?";
	msg_message2 = "This_cannot_be_undone!";
	msg_btn1text = "Yes";
	msg_btn2text = "No";
	msg_show(cb_msg_delete_confirm);
}

static
void objedit_update_bulkedit_buttons()
{
	btn_add_to_bulkedit->enabled =
		!(btn_remove_from_bulkedit->enabled = bulkedit_is_in_list(editingObject));
}

static
void cb_btn_add_to_bulkedit(struct UI_BUTTON *btn)
{
	if (bulkedit_add(editingObject)) {
		bulkedit_begin(editingObject);
	}
	objedit_update_bulkedit_buttons();
}

static
void cb_btn_remove_from_bulkedit(struct UI_BUTTON *btn)
{
	if (bulkedit_remove(editingObject)) {
		bulkedit_revert();
		bulkedit_end();
	}
	objedit_update_bulkedit_buttons();
}

static
void cb_btn_clone(struct UI_BUTTON *btn)
{
	objects_clone(editingObject, editingObject->sa_object);
}

static
void cb_btn_view_in_object_browser(struct UI_BUTTON *btn)
{
	struct RwV3D pos;

	game_ObjectGetPos(editingObject->sa_object, &pos);
	ui_hide_window();
	objbrowser_highlight_model(editingObject->model);
	objbrowser_show(&pos);
}

static
int objedit_proc_close(struct UI_WINDOW *wnd)
{
	if (updateFromMoving) {
		updateFromMoving = 0;
		return 1;
	}
	bulkedit_commit();
	editingObject = NULL;
	ui_hide_window();
	return 1;
}

struct OBJECT *objedit_get_editing_object()
{
	return editingObject;
}

void objedit_show(struct OBJECT *obj)
{
	game_ObjectGetPos(obj->sa_object, &initialPos);
	game_ObjectGetRot(obj->sa_object, &initialRot);
	editingObject = obj;
	updateFromMoving = 0;
	update_inputs();
	ui_show_window(wnd);
	objedit_update_bulkedit_buttons();
	bulkedit_begin(obj);
	if (modelNames[obj->model]) {
		strcpy(lbl_txt_model, modelNames[obj->model]);
	} else {
		sprintf(lbl_txt_model, "!%05d:_unknown", obj->model);
	}
}

void objedit_move()
{
	struct MSG_NC nc;

	updateFromMoving = 1;
	nc._parent.id = MAPEDIT_MSG_NATIVECALL;
	nc.nc = NC_EditObject;
	nc.params.asint[1] = 0;
	nc.params.asint[2] = editingObject->samp_objectid;
	sockets_send(&nc, sizeof(nc));
}

static
int draw_window_objedit(struct UI_ELEMENT *wnd)
{
	if (updateFromMoving) {
		if (ui_active_element == in_coord_x ||
			ui_active_element == in_coord_y ||
			ui_active_element == in_coord_z ||
			ui_active_element == in_rot_x ||
			ui_active_element == in_rot_y ||
			ui_active_element == in_rot_z)
		{
			updateFromMoving = 0;
		} else {
			update_inputs();
		}
		bulkedit_update();
	}
	return proc_draw_window_objedit(wnd);
}

void objedit_init()
{
	struct UI_BUTTON *btn;

	wnd = ui_wnd_make(5000.0f, 300.0f, "Object");
	wnd->columns = 2;
	proc_draw_window_objedit = wnd->_parent._parent.proc_draw;
	wnd->_parent._parent.proc_draw = draw_window_objedit;
	wnd->proc_close = objedit_proc_close;

	lbl_txt_model[0] = 0;
	ui_wnd_add_child(wnd, ui_lbl_make("Model:"));
	ui_wnd_add_child(wnd, ui_lbl_make(lbl_txt_model));
	ui_wnd_add_child(wnd, NULL);
	ui_wnd_add_child(wnd, ui_btn_make("View_in_object_browser", cb_btn_view_in_object_browser));
	ui_wnd_add_child(wnd, ui_lbl_make("Pos:"));
	in_coord_x = ui_in_make(cb_in_coords);
	ui_wnd_add_child(wnd, in_coord_x);
	ui_wnd_add_child(wnd, NULL);
	in_coord_y = ui_in_make(cb_in_coords);
	ui_wnd_add_child(wnd, in_coord_y);
	ui_wnd_add_child(wnd, NULL);
	in_coord_z = ui_in_make(cb_in_coords);
	ui_wnd_add_child(wnd, in_coord_z);
	ui_wnd_add_child(wnd, ui_lbl_make("Rot:"));
	in_rot_x = ui_in_make(cb_in_rotation);
	ui_wnd_add_child(wnd, in_rot_x);
	ui_wnd_add_child(wnd, NULL);
	in_rot_y = ui_in_make(cb_in_rotation);
	ui_wnd_add_child(wnd, in_rot_y);
	ui_wnd_add_child(wnd, NULL);
	in_rot_z = ui_in_make(cb_in_rotation);
	ui_wnd_add_child(wnd, in_rot_z);
	btn = ui_btn_make("Textures", cb_btn_textures);
	btn->_parent.span = 2;
	ui_wnd_add_child(wnd, btn);
	btn = ui_btn_make("Move", cb_btn_objectseditor_move);
	btn->_parent.span = 2;
	ui_wnd_add_child(wnd, btn);
	btn = ui_btn_make("Undo_move", cb_btn_undo_move);
	btn->_parent.span = 2;
	ui_wnd_add_child(wnd, btn);
	btn = ui_btn_make("Tp_to_camera", cb_btn_tp_to_camera);
	btn->_parent.span = 2;
	ui_wnd_add_child(wnd, btn);
	btn = ui_btn_make("Delete", cb_btn_delete);
	btn->_parent.span = 2;
	ui_wnd_add_child(wnd, btn);
	btn_add_to_bulkedit = ui_btn_make("Add_to_bulk_edit", cb_btn_add_to_bulkedit);
	btn_add_to_bulkedit->_parent.span = 2;
	ui_wnd_add_child(wnd, btn_add_to_bulkedit);
	btn_remove_from_bulkedit = ui_btn_make("Remove_from_bulk_edit", cb_btn_remove_from_bulkedit);
	btn_remove_from_bulkedit->_parent.span = 2;
	ui_wnd_add_child(wnd, btn_remove_from_bulkedit);
	btn = ui_btn_make("Clone", cb_btn_clone);
	btn->_parent.span = 2;
	ui_wnd_add_child(wnd, btn);
}

void objedit_dispose()
{
	ui_wnd_dispose(wnd);
}
