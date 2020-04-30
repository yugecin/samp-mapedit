/* vim: set filetype=c ts=8 noexpandtab: */

#define CONTAINER_MAX_CHILD_COUNT 132

struct UI_CONTAINER {
	struct UI_ELEMENT _parent;
	struct UI_ELEMENT *children[CONTAINER_MAX_CHILD_COUNT];
	int childcount;
	int need_layout;
};

struct UI_CONTAINER *ui_cnt_make();
void ui_cnt_dispose(struct UI_CONTAINER *cnt);
void ui_cnt_update(struct UI_CONTAINER *cnt);
void ui_cnt_draw(struct UI_CONTAINER *cnt);
int ui_cnt_mousedown(struct UI_CONTAINER *cnt);
int ui_cnt_mouseup(struct UI_CONTAINER *cnt);
int ui_cnt_mousewheel(struct UI_CONTAINER *cnt, int value);
void ui_cnt_add_child(struct UI_CONTAINER *cnt, void *child);
void ui_cnt_remove_child(struct UI_CONTAINER *cnt, void *child);
void ui_cnt_recalc_size(struct UI_CONTAINER *cnt);
