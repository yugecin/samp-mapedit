/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "ui.h"
#include <string.h>

static struct UI_WINDOW *window_project;

static
void cb_btn_project(struct UI_BUTTON *btn)
{
	ui_show_window(window_project);
}

static
void cb_in_projectname(struct UI_INPUT *in)
{
}

static
void cb_btn_createnew(struct UI_BUTTON *btn)
{
}

static
void cb_lst_open(struct UI_LIST *lst)
{
}

static
void cb_btn_open(struct UI_BUTTON *btn)
{
}

void prj_init()
{
	struct UI_BUTTON *btn;
	struct UI_LABEL *lbl;
	struct UI_INPUT *in;
	struct UI_LIST *lst;

	btn = ui_btn_make("Project", cb_btn_project);
	btn->_parent.x = 10.0f;
	btn->_parent.y = 650.0f;
	ui_cnt_add_child(background_element, btn);

	window_project = ui_wnd_make(500.0f, 500.0f, "Project");
	window_project->columns = 3;

	lbl = ui_lbl_make("New:");
	ui_wnd_add_child(window_project, lbl);
	in = ui_in_make(cb_in_projectname);
	ui_wnd_add_child(window_project, in);
	btn = ui_btn_make("Create", cb_btn_createnew);
	ui_wnd_add_child(window_project, btn);
	lbl = ui_lbl_make("Open:");
	ui_wnd_add_child(window_project, lbl);
	lst = ui_lst_make(20, cb_lst_open);
	lst->_parent.span = 2;
	ui_wnd_add_child(window_project, lst);
	ui_wnd_add_child(window_project, NULL);
	btn = ui_btn_make("Open", cb_btn_open);
	btn->_parent.span = 2;
	ui_wnd_add_child(window_project, btn);
}

void prj_dispose()
{
}
