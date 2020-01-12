/* vim: set filetype=c ts=8 noexpandtab: */

#define RADIOGROUP_MAX_CHILDREN 30

typedef void (rdbcb)(struct UI_RADIOBUTTON *rdb);

struct UI_RADIOBUTTON {
	struct UI_BUTTON _parent;
	struct RADIOBUTTONGROUP *group;
};

struct RADIOBUTTONGROUP {
	struct UI_RADIOBUTTON *buttons[RADIOGROUP_MAX_CHILDREN];
	struct UI_RADIOBUTTON *activebutton;
	int buttoncount;
	rdbcb *proc_change;
};

/**
Returned group does not need to be disposed (only if it has children)!

It will be disposed when all its containing children are disposed.
*/
struct RADIOBUTTONGROUP *ui_rdbgroup_make(rdbcb *proc_change);
struct UI_RADIOBUTTON *ui_rdb_make(
	char *text, struct RADIOBUTTONGROUP *group, int check);
void ui_rdb_dispose(struct UI_RADIOBUTTON *rdb);
void ui_rdb_update(struct UI_RADIOBUTTON *rdb);
int ui_rdb_mouseup(struct UI_RADIOBUTTON *rdb);
