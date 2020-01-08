/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ui.h"
#include <string.h>

struct UI_WINDOW *ui_wnd_make(float x, float y, char *title)
{
	struct UI_WINDOW *wnd;
	int textlenandzero;

	textlenandzero = strlen(title) + 1;
	wnd = malloc(sizeof(struct UI_BUTTON));
	wnd->_parent.type = WINDOW;
	wnd->childcount = 0;
	wnd->title = malloc(sizeof(char) * textlenandzero);
	wnd->_parent.proc_draw = (ui_method*) ui_wnd_draw;
	wnd->_parent.proc_dispose = (ui_method*) ui_wnd_dispose;
	wnd->_parent.proc_mousedown = (ui_method*) ui_wnd_mousedown;
	wnd->_parent.proc_mouseup = (ui_method*) ui_wnd_mouseup;
	memcpy(wnd->title, title, textlenandzero);
	ui_wnd_recalc_size(wnd);
	return wnd;
}

void ui_wnd_dispose(struct UI_WINDOW *wnd)
{
	struct UI_ELEMENT *child;
	int i;

	for (i = 0; i < wnd->childcount; i++) {
		child = wnd->children[i];
		child->proc_dispose(child);
	}

	free(wnd->title);
	free(wnd);
}

void ui_wnd_draw(struct UI_WINDOW *wnd)
{
	struct UI_ELEMENT *child;
	int i;

	for (i = 0; i < wnd->childcount; i++) {
		child = wnd->children[i];
		child->proc_draw(child);
	}
}

void ui_wnd_recalc_size(struct UI_WINDOW *wnd)
{
}

int ui_wnd_mousedown(struct UI_WINDOW *wnd)
{
	struct UI_ELEMENT *child;
	int i, ret;

	if (ui_element_is_hovered(&wnd->_parent)) {
		for (i = 0; i < wnd->childcount; i++) {
			child = wnd->children[i];
			if ((ret = child->proc_mousedown(child))) {
				return ret;
			}
		}
		return 1;
	}
	return 0;
}

int ui_wnd_mouseup(struct UI_WINDOW *wnd)
{
	struct UI_ELEMENT *child;
	int i, ret;

	for (i = 0; i < wnd->childcount; i++) {
		child = wnd->children[i];
		if ((ret = child->proc_mouseup(child))) {
			return ret;
		}
	}
	return 0;
}

void ui_wnd_add_child(struct UI_WINDOW *wnd, struct UI_ELEMENT *child)
{
	if (wnd->childcount < WINDOW_MAX_CHILD_COUNT) {
		wnd->children[wnd->childcount++] = child;
	}
}
