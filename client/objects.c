/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ui.h"
#include "objects.h"
#include "sockets.h"
#include "../shared/serverlink.h"

static
void cb_btn_mkobject(struct UI_BUTTON *btn)
{
	struct MSG_NC nc;
	float x, y, z;

	x = camera->position.x + 100.0f * camera->rotation.x;
	y = camera->position.y + 100.0f * camera->rotation.y;
	z = camera->position.z + 100.0f * camera->rotation.z;

	nc._parent.id = MAPEDIT_MSG_NATIVECALL;
	nc._parent.data = 0; /*TODO*/
	nc.nc = NC_CreateObject;
	nc.params.asint[1] = 3279;
	nc.params.asflt[2] = x;
	nc.params.asflt[3] = y;
	nc.params.asflt[4] = z;
	nc.params.asflt[5] = 0.0f;
	nc.params.asflt[6] = 0.0f;
	nc.params.asflt[7] = 0.0f;
	nc.params.asflt[8] = 500.0f;
	sockets_send(&nc, sizeof(nc));
}

void objects_init()
{
	struct UI_BUTTON *btn;
	struct UI_LABEL *lbl;
	struct MSG msg;

	msg.id = MAPEDIT_MSG_RESETOBJECTS;
	sockets_send(&msg, sizeof(msg));

	/*context menu entry*/
	btn = ui_btn_make("Make_Object", cb_btn_mkobject);
	ui_wnd_add_child(context_menu, btn);

	/*main menu entry*/
	lbl = ui_lbl_make("=_Objects_=");
	lbl->_parent.span = 2;
	ui_wnd_add_child(main_menu, lbl);
}

void objects_dispose()
{
}
