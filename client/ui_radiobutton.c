/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ui.h"
#include <string.h>

static
void ui_rdb_set_checked(struct UI_RADIOBUTTON *rdb, int state)
{
	rdb->_parent.text[1] = '_' + ('x' - '_') * state;
	ui_btn_recalc_size((struct UI_BUTTON*) rdb);
}

static
void ui_rdbgroup_remove(
	struct RADIOBUTTONGROUP *group, struct UI_RADIOBUTTON *rdb)
{
	int i;

	for (i = 0; i < group->buttoncount; i++) {
		if (group->buttons[i] == rdb) {
			if (i != group->buttoncount - 1) {
				group->buttons[i] =
					group->buttons[group->buttoncount - 1];
			}
			group->buttoncount--;
			if (group->buttoncount == 0) {
				free(group);
			}
			return;
		}
	}
}

static
void ui_rdbgroup_add(
	struct RADIOBUTTONGROUP *group, struct UI_RADIOBUTTON *rdb, int check)
{
	int i;

	/*LIMIT: this may crash if limit was reached*/
	if (group->buttoncount < RADIOGROUP_MAX_CHILDREN) {
		rdb->group = group;
		group->buttons[group->buttoncount] = rdb;
		if (check) {
			for (i = 0; i < group->buttoncount; i++) {
				ui_rdb_set_checked(group->buttons[i], 0);
			}
			ui_rdb_set_checked(rdb, 1);
			group->activebutton = rdb;
		}
		group->buttoncount++;
	}
}

static
void ui_rdbgroup_clicked(
	struct RADIOBUTTONGROUP *group, struct UI_RADIOBUTTON *rdb)
{
	int i;

	for (i = 0; i < group->buttoncount; i++) {
		ui_rdb_set_checked(group->buttons[i], 0);
	}
	ui_rdb_set_checked(rdb, 1);
	group->activebutton = rdb;
	group->proc_change(group, rdb->data);
}

struct RADIOBUTTONGROUP *ui_rdbgroup_make(rdbcb *proc_change)
{
	struct RADIOBUTTONGROUP *grp;

	grp = malloc(sizeof(struct RADIOBUTTONGROUP));
	grp->activebutton = NULL;
	grp->buttoncount = 0;
	grp->proc_change = proc_change;
	return grp;
}

struct UI_RADIOBUTTON *ui_rdb_make(
	float x, float y, char *text,
	struct RADIOBUTTONGROUP *group, int check)
{
	struct UI_RADIOBUTTON *rdb;
	int textlenandzero;

	textlenandzero = 4 + strlen(text) + 1;
	rdb = malloc(sizeof(struct UI_RADIOBUTTON));
	ui_elem_init(rdb, RADIOBUTTON, x, y);
	rdb->_parent._parent.proc_dispose = (ui_method*) ui_rdb_dispose;
	rdb->_parent._parent.proc_update = (ui_method*) ui_rdb_update;
	/*draw for radiobutton can be the button proc*/
	rdb->_parent._parent.proc_draw = (ui_method*) ui_btn_draw;
	/*mousedown for radiobutton can be the button proc*/
	rdb->_parent._parent.proc_mousedown = (ui_method*) ui_btn_mousedown;
	rdb->_parent._parent.proc_mouseup = (ui_method*) ui_rdb_mouseup;
	rdb->_parent.text = malloc(sizeof(char) * textlenandzero);
	rdb->_parent.text[0] = '[';
	/*1 is set below with the ui_rdbgroup_add call*/
	rdb->_parent.text[2] = ']';
	rdb->_parent.text[3] = '_';
	memcpy(rdb->_parent.text + 4, text, textlenandzero - 4);
	rdb->data = NULL;
	ui_rdbgroup_add(group, rdb, check);
	ui_btn_recalc_size((struct UI_BUTTON*) rdb);
	return rdb;
}

void ui_rdb_dispose(struct UI_RADIOBUTTON *rdb)
{
	ui_rdbgroup_remove(rdb->group, rdb);
	ui_btn_dispose((struct UI_BUTTON*) rdb);
}

void ui_rdb_update(struct UI_RADIOBUTTON *rdb)
{
}

int ui_rdb_mouseup(struct UI_RADIOBUTTON *rdb)
{
	if (ui_element_being_clicked == rdb) {
		if (ui_element_is_hovered(&rdb->_parent._parent)) {
			ui_rdbgroup_clicked(rdb->group, rdb);
		}
		return 1;
	}
	return 0;
}
