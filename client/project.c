/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ui.h"
#include "msgbox.h"
#include "project.h"
#include <stdio.h>
#include <string.h>
#include <windows.h>

#define MAX_FILES 1000
#define NAME_LEN 50

static struct UI_WINDOW *window_project;
static struct UI_INPUT *in_newprojectname;
static struct UI_LIST *lst_projects;
static char open_project_name[INPUT_TEXTLEN + 1];
static char tmp_files[MAX_FILES][NAME_LEN];
static int numfiles;

static
void proj_updatelist()
{
	char *listdata[MAX_FILES];
	WIN32_FIND_DATAA data;
	HANDLE search;

	numfiles = 0;
	search = FindFirstFileA("samp-mapedit\\*.mep", &data);
	if (search != INVALID_HANDLE_VALUE) {
		do {
			memcpy(tmp_files[numfiles], data.cFileName, NAME_LEN);
			listdata[numfiles] = tmp_files[numfiles];
			tmp_files[numfiles][49] = 0;
			numfiles++;
		} while (FindNextFileA(search, &data));
		FindClose(search);
	}
	ui_lst_set_data(lst_projects, listdata, numfiles);
}

static
void cb_btn_project(struct UI_BUTTON *btn)
{
	ui_show_window(window_project);
}

static
void cb_createnew_name_err(int btn)
{
	ui_show_window(window_project);
}

static
void cb_btn_createnew(struct UI_BUTTON *btn)
{
	if (in_newprojectname->valuelen <= 0) {
		msg_message = "Empty_name_is_not_allowed";
		msg_title = "New_project";
		msg_btn1text = "Ok";
		msg_btn2text = NULL;
		msg_btn3text = NULL;
		msg_show(cb_createnew_name_err);
	} else {
		memcpy(open_project_name,
			in_newprojectname->value,
			INPUT_TEXTLEN + 1);
		ui_hide_window(window_project);
		prj_save();
	}
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

	lbl = ui_lbl_make("=_Project_=");
	lbl->_parent.span = 2;
	ui_wnd_add_child(main_menu, lbl);
	btn = ui_btn_make("Create/Open", cb_btn_project);
	btn->_parent.span = 2;
	ui_wnd_add_child(main_menu, btn);
	btn = ui_btn_make("Save", cb_btn_project);
	btn->_parent.span = 2;
	btn->enabled = 0;
	ui_wnd_add_child(main_menu, btn);

	window_project = ui_wnd_make(500.0f, 500.0f, "Project");
	window_project->columns = 3;

	lbl = ui_lbl_make("New:");
	ui_wnd_add_child(window_project, lbl);
	in_newprojectname = ui_in_make(NULL);
	ui_wnd_add_child(window_project, in_newprojectname);
	btn = ui_btn_make("Create", cb_btn_createnew);
	ui_wnd_add_child(window_project, btn);
	lbl = ui_lbl_make("Open:");
	ui_wnd_add_child(window_project, lbl);
	lst_projects = ui_lst_make(20, cb_lst_open);
	lst_projects->_parent.span = 2;
	ui_wnd_add_child(window_project, lst_projects);
	ui_wnd_add_child(window_project, NULL);
	btn = ui_btn_make("Open", cb_btn_open);
	btn->_parent.span = 2;
	ui_wnd_add_child(window_project, btn);

	ui_show_window(window_project);
	proj_updatelist();
}

void prj_dispose()
{
	ui_wnd_dispose(window_project);
}

void prj_open(char *name)
{
}

void prj_save()
{
	FILE *f;
	struct CCam *ccam;
	char projfile[15 + INPUT_TEXTLEN];
	char buf[1000];
	struct {
		int x, y, z;
	} *vec3i;

	ccam = &camera->cams[camera->activeCam];
	sprintf(projfile, "samp-mapedit\\%s.mep", open_project_name);
	if ((f = fopen(projfile, "w"))) {
		vec3i = (void*) &ccam->pos;
		fwrite(buf, sprintf(buf, "cam.pos.x %d\n", vec3i->x), 1, f);
		fwrite(buf, sprintf(buf, "cam.pos.y %d\n", vec3i->y), 1, f);
		fwrite(buf, sprintf(buf, "cam.pos.z %d\n", vec3i->z), 1, f);
		vec3i = (void*) &ccam->lookVector;
		fwrite(buf, sprintf(buf, "cam.rot.x %d\n", vec3i->x), 1, f);
		fwrite(buf, sprintf(buf, "cam.rot.y %d\n", vec3i->y), 1, f);
		fwrite(buf, sprintf(buf, "cam.rot.z %d\n", vec3i->z), 1, f);
		fclose(f);
	} else {
		sprintf(debugstring, "failed to write file %s", projfile);
		ui_push_debug_string();
	}
}
