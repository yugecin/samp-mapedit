/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "ui.h"
#include <stdio.h>

static
void cb_btn_mkracecp(struct UI_BUTTON *btn)
{
}

static
void cb_btn_cplist(struct UI_BUTTON *btn)
{
}

void racecp_init()
{
	struct UI_BUTTON *btn;
	struct UI_LABEL *lbl;

	btn = ui_btn_make("Make_race_CP", cb_btn_mkracecp);
	ui_wnd_add_child(context_menu, btn);

	lbl = ui_lbl_make("=_Race_CPs_=");
	lbl->_parent.span = 2;
	ui_wnd_add_child(main_menu, lbl);
	btn = ui_btn_make("List", cb_btn_cplist);
	btn->_parent.span = 2;
	ui_wnd_add_child(main_menu, btn);
}

void racecp_dispose()
{
}

void racecp_prj_save(FILE *f, char *buf)
{
}

int racecp_prj_load_line(char *buf)
{
	return 0;
}

void racecp_prj_postload()
{
}
