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
	wnd = malloc(sizeof(struct UI_WINDOW));
	wnd->_parent._parent.type = WINDOW;
	wnd->_parent._parent.proc_dispose = (ui_method*) ui_wnd_dispose;
	wnd->_parent._parent.proc_update = (ui_method*) ui_wnd_update;
	wnd->_parent._parent.proc_draw = (ui_method*) ui_wnd_draw;
	wnd->_parent._parent.proc_mousedown = (ui_method*) ui_wnd_mousedown;
	wnd->_parent._parent.proc_mouseup = (ui_method*) ui_wnd_mouseup;
	wnd->_parent.childcount = 0;
	wnd->_parent.need_layout = 1;
	wnd->title = malloc(sizeof(char) * textlenandzero);
	memcpy(wnd->title, title, textlenandzero);
	return wnd;
}

void ui_wnd_dispose(struct UI_WINDOW *wnd)
{
	free(wnd->title);
	ui_cnt_dispose((struct UI_CONTAINER*) wnd);
}

void ui_wnd_update(struct UI_WINDOW *wnd)
{
	struct UI_ELEMENT *child;
	int i;

	for (i = 0; i < wnd->_parent.childcount; i++) {
		child = wnd->_parent.children[i];
		child->proc_update(child);
	}
	if (wnd->_parent.need_layout) {
		wnd->_parent.need_layout = 0;
	}
}

void ui_wnd_draw(struct UI_WINDOW *wnd)
{
	ui_cnt_draw((struct UI_CONTAINER*) wnd);
}

int ui_wnd_mousedown(struct UI_WINDOW *wnd)
{
	return ui_cnt_mousedown((struct UI_CONTAINER*) wnd);
}

int ui_wnd_mouseup(struct UI_WINDOW *wnd)
{
	return ui_cnt_mouseup((struct UI_CONTAINER*) wnd);
}

void ui_wnd_add_child(struct UI_WINDOW *wnd, struct UI_ELEMENT *child)
{
	ui_cnt_add_child((struct UI_CONTAINER*) wnd, child);
}
