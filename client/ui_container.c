/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "ui.h"

struct UI_CONTAINER *ui_cnt_make()
{
	struct UI_CONTAINER *cnt;

	cnt = malloc(sizeof(struct UI_CONTAINER));
	ui_elem_init(cnt, BACKGROUND, 0.0f, 0.0f);
	cnt->_parent.type = BACKGROUND;
	cnt->_parent.width = fresx;
	cnt->_parent.height = fresy;
	cnt->_parent.proc_dispose = (ui_method*) ui_cnt_dispose;
	cnt->_parent.proc_update = (ui_method*) ui_cnt_update;
	cnt->_parent.proc_draw = (ui_method*) ui_cnt_draw;
	cnt->_parent.proc_mousedown = (ui_method*) ui_cnt_mousedown;
	cnt->_parent.proc_mouseup = (ui_method*) ui_cnt_mouseup;
	cnt->childcount = 0;
	cnt->need_layout = 1;
	return cnt;
}

void ui_cnt_dispose(struct UI_CONTAINER *cnt)
{
	struct UI_ELEMENT *child;
	int i;

	for (i = 0; i < cnt->childcount; i++) {
		child = cnt->children[i];
		child->proc_dispose(child);
	}

	free(cnt);
}

void ui_cnt_update(struct UI_CONTAINER *cnt)
{
	struct UI_ELEMENT *child;
	int i;

	for (i = 0; i < cnt->childcount; i++) {
		child = cnt->children[i];
		child->proc_update(child);
	}
	if (cnt->need_layout) {
		for (i = 0; i < cnt->childcount; i++) {
			child = cnt->children[i];
			child->width = child->pref_width;
			child->height = child->pref_height;
		}
		cnt->need_layout = 0;
	}
}

void ui_cnt_draw(struct UI_CONTAINER *cnt)
{
	struct UI_ELEMENT *child;
	int i;

	for (i = 0; i < cnt->childcount; i++) {
		child = cnt->children[i];
		child->proc_draw(child);
	}
}

int ui_cnt_mousedown(struct UI_CONTAINER *cnt)
{
	struct UI_ELEMENT *child;
	int i, ret;

	if (ui_element_is_hovered(&cnt->_parent)) {
		for (i = 0; i < cnt->childcount; i++) {
			child = cnt->children[i];
			if ((ret = child->proc_mousedown(child))) {
				return ret;
			}
		}
		ui_element_being_clicked = cnt;
		return 1;
	}
	return 0;
}

int ui_cnt_mouseup(struct UI_CONTAINER *cnt)
{
	struct UI_ELEMENT *child;
	int i, ret;

	for (i = 0; i < cnt->childcount; i++) {
		child = cnt->children[i];
		if ((ret = child->proc_mouseup(child))) {
			return ret;
		}
	}
	return 0;
}

void ui_cnt_add_child(struct UI_CONTAINER *cnt, struct UI_ELEMENT *child)
{
	if (cnt->childcount < CONTAINER_MAX_CHILD_COUNT) {
		cnt->children[cnt->childcount++] = child;
		child->parent = cnt;
		cnt->need_layout = 1;
	}
}
