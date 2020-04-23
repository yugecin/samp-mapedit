/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ui.h"
#include "msgbox.h"
#include "objbase.h"
#include "objects.h"
#include "persistence.h"
#include "player.h"
#include "project.h"
#include "racecp.h"
#include "removedbuildings.h"
#include "removedbuildingsui.h"
#include "timeweather.h"
#include <stdio.h>
#include <string.h>
#include <windows.h>

#define MAX_FILES 1000
#define NAME_LEN 50
#define FILE_LEN NAME_LEN + 15

static struct UI_WINDOW *window_project;
static struct UI_INPUT *in_newprojectname;
static struct UI_LIST *lst_projects;
static struct UI_BUTTON *btn_main_save, *btn_open;
static struct UI_LABEL *lbl_current;
static char open_project_name[INPUT_TEXTLEN + 1];
static char tmp_files[MAX_FILES][NAME_LEN];
static int numfiles;

static
void cb_show_project_window(int btn)
{
	ui_show_window(window_project);
}

static
void mk_project_filename(char *buf, char *project_name)
{
	sprintf(buf, "samp-mapedit\\%s.mep", project_name);
}

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
			/*remove extension*/
			tmp_files[numfiles][strlen(tmp_files[numfiles])-4] = 0;
			numfiles++;
		} while (FindNextFileA(search, &data) && numfiles < 1000);
		FindClose(search);
	}
	ui_lst_set_data(lst_projects, listdata, numfiles);
}

void prj_save()
{
	FILE *f;
	char buf[1000];

	mk_project_filename(buf, open_project_name);
	if ((f = fopen(buf, "w"))) {
		ui_prj_save(f, buf);
		racecp_prj_save(f, buf);
		objects_prj_save(f, buf);
		player_prj_save(f, buf);
		timeweather_prj_save(f, buf);
		fclose(f);
	} else {
		sprintf(debugstring, "failed to write file %s", buf);
		ui_push_debug_string();
	}
}

static
void prj_preload()
{
	TRACE("prj_preload");
	objects_prj_preload();
	player_prj_preload();
	timeweather_prj_preload();
	rb_undo_all();
}

static
void prj_postload()
{
	TRACE("prj_postload");
	ui_prj_postload();
	objects_prj_postload();
	racecp_prj_postload();
	timeweather_prj_postload();
	rb_do_all();
	rbui_refresh_list();
	btn_main_save->enabled = 1;
	lbl_current->text = open_project_name;
	persistence_set_project_to_load(open_project_name);
}

/**
Closes file when done.
*/
static
void prj_open_by_file(FILE *file)
{
	int pos, i;
	char buf[512];

	TRACE("prj_open_by_file");
	prj_preload();

	pos = 0;
nextline:
	memset(buf, 0, sizeof(buf));
	fseek(file, pos, SEEK_SET);
	if (fread(buf, 1, sizeof(buf) - 1, file) == 0) {
		goto done;
	}
	ui_prj_load_line(buf) ||
		racecp_prj_load_line(buf) ||
		objects_prj_load_line(buf) ||
		player_prj_load_line(buf) ||
		timeweather_prj_load_line(buf);
	i = 0;
	while (buf[i] != 0 && buf[i] != '\n') {
		i++;
	}
	pos += i + 2;
	goto nextline;
done:
	fclose(file);

	prj_postload();
}

void prj_open_by_name(char *name)
{
	FILE *file;
	char filename[FILE_LEN];

	mk_project_filename(filename, name);
	if (file = fopen(filename, "r")) {
		memcpy(open_project_name, name, INPUT_TEXTLEN + 1);
		prj_open_by_file(file);
	} else {
		msg_message = "Failed_to_open_project";
		msg_title = "Open_project";
		msg_btn1text = "Ok";
		msg_show(cb_show_project_window);
		proj_updatelist();
	}
}

static
void cb_btn_refreshlist(struct UI_BUTTON *btn)
{
	proj_updatelist();
}

static
void cb_btn_project(struct UI_BUTTON *btn)
{
	ui_show_window(window_project);
}

static
void cb_btn_createnew(struct UI_BUTTON *btn)
{
	char projectfile[FILE_LEN];
	FILE *file;

	if (in_newprojectname->valuelen <= 0) {
		msg_message = "Empty_name_is_not_allowed";
		msg_title = "New_project";
		msg_btn1text = "Ok";
		msg_show(cb_show_project_window);
	} else {
		memcpy(open_project_name,
			in_newprojectname->value,
			INPUT_TEXTLEN + 1);
		ui_hide_window();
		btn_main_save->enabled = 1;
		mk_project_filename(projectfile, open_project_name);
		if (file = fopen(projectfile, "r")) {
			prj_open_by_file(file);
		} else {
			prj_save();
			prj_preload();
			prj_postload();
		}
		proj_updatelist();
	}
}

static
void cb_lst_open(struct UI_LIST *lst)
{
	btn_open->enabled = lst->selectedindex != -1;
}

static
void cb_btn_open(struct UI_BUTTON *btn)
{
	int idx;

	idx = lst_projects->selectedindex;
	if (idx != -1) {
		ui_hide_window();
		prj_open_by_name(tmp_files[idx]);
	}
}

static
void cb_btn_save(struct UI_BUTTON *btn)
{
	prj_save();
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
	btn_main_save = ui_btn_make("Save", cb_btn_save);
	btn_main_save->_parent.span = 2;
	btn_main_save->enabled = 0;
	ui_wnd_add_child(main_menu, btn_main_save);

	window_project = ui_wnd_make(500.0f, 500.0f, "Project");
	window_project->columns = 3;

	ui_wnd_add_child(window_project, NULL);
	lbl_current = ui_lbl_make("no project opened");
	lbl_current->_parent.span = 2;
	ui_wnd_add_child(window_project, lbl_current);
	lbl = ui_lbl_make("New:");
	ui_wnd_add_child(window_project, lbl);
	in_newprojectname = ui_in_make(NULL);
	ui_wnd_add_child(window_project, in_newprojectname);
	btn = ui_btn_make("Create", cb_btn_createnew);
	ui_wnd_add_child(window_project, btn);
	lbl = ui_lbl_make("Open:");
	ui_wnd_add_child(window_project, lbl);
	btn = ui_btn_make("Refresh list", cb_btn_refreshlist);
	btn->_parent.span = 2;
	ui_wnd_add_child(window_project, btn);
	ui_wnd_add_child(window_project, NULL);
	lst_projects = ui_lst_make(20, cb_lst_open);
	lst_projects->_parent.span = 2;
	ui_wnd_add_child(window_project, lst_projects);
	ui_wnd_add_child(window_project, NULL);
	btn_open = ui_btn_make("Open", cb_btn_open);
	btn_open->_parent.span = 2;
	btn_open->enabled = 0;
	ui_wnd_add_child(window_project, btn_open);
}

void prj_do_show_window()
{
	ui_show_window(window_project);
	proj_updatelist();
}

void prj_dispose()
{
	TRACE("prj_dispose");
	ui_wnd_dispose(window_project);
}

void prj_open_persistent_state()
{
	char *project_name;

	project_name = persistence_get_project_to_load();
	if (project_name != NULL) {
		prj_open_by_name(project_name);
		ui_hide_window();
	}
}
