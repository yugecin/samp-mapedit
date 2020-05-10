/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ui.h"
#include "msgbox.h"
#include "racecp.h"
#include "racecpeditor.h"
#include "racecpui.h"

static struct UI_BUTTON *btn_mainmenu_cplist;
static struct UI_BUTTON *btn_contextmenu_mkracecp;

static
void cb_btn_mkracecp(struct UI_BUTTON *btn)
{
	struct RwV3D posToCreate;

	if (numcheckpoints == MAX_RACECHECKPOINTS) {
		msg_title = "Race_cp";
		msg_message = "Max_race_checkpoints_reached";
		msg_btn1text = "Ok";
		msg_show(NULL);
		return;
	}

	ui_get_clicked_position(&posToCreate);
	checkpointDescriptions[numcheckpoints][0] = 0;
	racecheckpoints[numcheckpoints].used = 0;
	racecheckpoints[numcheckpoints].free = 1;
	racecheckpoints[numcheckpoints].type = 0;
	racecheckpoints[numcheckpoints].pos = posToCreate;
	racecheckpoints[numcheckpoints].fRadius = 2.0f;
	racecheckpoints[numcheckpoints].arrowDirection.x = 1.0f;
	racecheckpoints[numcheckpoints].arrowDirection.y = 0.0f;
	racecheckpoints[numcheckpoints].arrowDirection.z = 0.0f;
	racecheckpoints[numcheckpoints].colABGR = 0xFF0000FF;
	racecpeditor_edit_checkpoint(numcheckpoints);
	numcheckpoints++;
}

static
void cb_btn_cplist(struct UI_BUTTON *btn)
{
}

void racecpui_init()
{
	struct UI_BUTTON *btn;
	struct UI_LABEL *lbl;

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
}

void racecpui_dispose()
{
}

void racecpui_prj_postload()
{
	btn_mainmenu_cplist->enabled = 1;
	btn_contextmenu_mkracecp->enabled = 1;
}
