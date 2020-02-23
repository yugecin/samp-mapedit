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
static struct UI_BUTTON *btn_main_save, *btn_open;
static char open_project_name[INPUT_TEXTLEN + 1];
static char open_project_file[INPUT_TEXTLEN + 15];
static char tmp_files[MAX_FILES][NAME_LEN];
static int numfiles;

static
void update_project_filename()
{
	sprintf(open_project_file, "samp-mapedit\\%s.mep", open_project_name);
}

void prj_save()
{
	FILE *f;
	char buf[1000];
	float rot;
	struct {
		int x, y, z;
	} *vec3i;
	struct RwV3D p, *pp = &p;

	update_project_filename();
	if ((f = fopen(open_project_file, "w"))) {
		ui_prj_save(f, buf);
		game_PedGetPos(player, (struct RwV3D**) &vec3i, &rot);
		fwrite(buf, sprintf(buf, "playa.pos.x %d\n", vec3i->x), 1, f);
		fwrite(buf, sprintf(buf, "playa.pos.y %d\n", vec3i->y), 1, f);
		fwrite(buf, sprintf(buf, "playa.pos.z %d\n", vec3i->z), 1, f);
		fwrite(buf, sprintf(buf, "playa.rot %f\n", rot), 1, f);
		game_PedGetPos(player, &pp, &rot);
		fclose(f);
	} else {
		sprintf(debugstring,
			"failed to write file %s",
			open_project_file);
		ui_push_debug_string();
	}
}

/**
Closes file when done.
*/
static
void prj_open_by_file(FILE *file)
{
	int pos, i;
	char buf[512];
	union {
		int i;
		float f;
		int *p;
	} value;
	struct RwV3D playapos;

	pos = 0;
nextline:
	memset(buf, 0, sizeof(buf));
	fseek(file, pos, SEEK_SET);
	if (fread(buf, 1, sizeof(buf) - 1, file) == 0) {
		goto done;
	}
	if (!ui_prj_load_line(buf)) {
		if (strncmp("playa.", buf, 6) == 0) {
			if (strncmp("pos.", buf + 6, 4) == 0) {
				value.p = (int*) &playapos + buf[10] - 'x';
				*value.p = atoi(buf + (i = 12));
			} else if (strncmp("rot ", buf + 6, 4) == 0) {
				value.i = atoi(buf + (i = 10));
				game_PedSetRot(player, value.f);
			}
		} else {
			i = 0;
		}
	}
	while (buf[i] != 0 && buf[i] != '\n') {
		i++;
	}
	pos += i + 2;
	goto nextline;
done:
	fclose(file);

	game_PedSetPos(player, &playapos);
	ui_prj_postload();
	btn_main_save->enabled = 1;
}

void prj_open_by_name(char *name)
{
	FILE *file;

	memcpy(open_project_name, name, INPUT_TEXTLEN + 1);
	update_project_filename();
	if (file = fopen(open_project_file, "r")) {
		prj_open_by_file(file);
	}
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
void cb_createnew_name_err(int btn)
{
	ui_show_window(window_project);
}

static
void cb_btn_createnew(struct UI_BUTTON *btn)
{
	FILE *file;

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
		update_project_filename();
		ui_hide_window(window_project);
		btn_main_save->enabled = 1;
		proj_updatelist();
		if (file = fopen(open_project_file, "r")) {
			prj_open_by_file(file);
		} else {
			prj_save();
		}
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
		ui_hide_window(window_project);
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

	ui_show_window(window_project);
	proj_updatelist();
}

void prj_dispose()
{
	ui_wnd_dispose(window_project);
}
