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
	ui_elem_init(wnd, WINDOW, x, y);
	wnd->_parent._parent.proc_dispose = (ui_method*) ui_wnd_dispose;
	wnd->_parent._parent.proc_update = (ui_method*) ui_wnd_update;
	wnd->_parent._parent.proc_draw = (ui_method*) ui_wnd_draw;
	wnd->_parent._parent.proc_mousedown = (ui_method*) ui_wnd_mousedown;
	wnd->_parent._parent.proc_mouseup = (ui_method*) ui_wnd_mouseup;
	wnd->_parent.childcount = 0;
	wnd->_parent.need_layout = 1;
	wnd->columns = 1;
	wnd->title = malloc(sizeof(char) * textlenandzero);
	memcpy(wnd->title, title, textlenandzero);
	return wnd;
}

void ui_wnd_dispose(struct UI_WINDOW *wnd)
{
	free(wnd->title);
	ui_cnt_dispose((struct UI_CONTAINER*) wnd);
}

static
void update_layout(struct UI_WINDOW *wnd)
{
	struct UI_ELEMENT *child;
	float tmp, tmp2;
	int e, i, col, row, colcount, rowcount;
	float colwidths[20], rowheights[100], cw, colx, rowy;
	int span;

	memset(colwidths, 0, sizeof(colwidths));
	memset(rowheights, 0, sizeof(rowheights));

	colcount = wnd->columns;
	rowcount = 1;
	col = 0;
	row = 0;
	for (e = 0; e < wnd->_parent.childcount; e++) {
		child = wnd->_parent.children[e];
		if (child->span == 1) {
			colwidths[col] = fontpad * 2.0f + child->pref_width;
			col++;
		} else {
			cw = (fontpad * 2.0f + child->pref_width) / child->span;
			span = child->span;
			while (span-- > 0) {
				tmp = cw;
				if (span == child->span - 1 || span == 0) {
					tmp += fontpad;
				}
				if (tmp > colwidths[col]) {
					colwidths[col] = tmp;
				}
				col++;
			}
		}
		tmp = fontpad * 2.0f + child->pref_height;
		if (tmp > rowheights[row]) {
			rowheights[row] = tmp;
		}
		if (col >= colcount) {
			row++;
			col = 0;
		}
	}
	rowcount = row;
	if (col != 0) {
		rowcount++;
	}

	tmp = 0.0f;
	for (i = 0; i < colcount; i++) {
		tmp += colwidths[i];
	}
	if (tmp < 100.0f) {
		tmp2 = (100.0f - tmp) / colcount;
		for (i = 0; i < colcount; i++) {
			colwidths[i] += tmp2;
		}
		tmp = 100.0f;
	}
	wnd->_parent._parent.width = tmp;
	tmp = 0.0f;
	for (i = 0; i < rowcount; i++) {
		tmp += rowheights[i];
	}
	if (tmp < 100.0f) {
		tmp2 = (100.0f - tmp) / rowcount;
		for (i = 0; i < rowcount; i++) {
			rowheights[i] += tmp2;
		}
		tmp = 100.0f;
	}
	wnd->_parent._parent.height = tmp;

	col = 0;
	row = 0;
	colx = wnd->_parent._parent.x;
	rowy = wnd->_parent._parent.y;
	for (e = 0; e < wnd->_parent.childcount; e++) {
		child = wnd->_parent.children[e];
		child->x = colx + fontpad;
		child->y = rowy + fontpad;
		tmp = fontpad * -2.0f;
		span = child->span;
		while (span-- > 0) {
			tmp += colwidths[col];
			colx += colwidths[col];
			col++;
		}
		child->width = tmp;
		child->height = rowheights[row] - fontpad * 2.0f;

		if (col >= colcount) {
			col = 0;
			colx = wnd->_parent._parent.x;
			rowy += rowheights[row];
			row++;
		}
	}
}

void ui_wnd_update(struct UI_WINDOW *wnd)
{
	struct UI_ELEMENT *child;
	int i;

	for (i = 0; i < wnd->_parent.childcount; i++) {
		child = wnd->_parent.children[i];
		child->proc_update(child);
	}
	if (ui_element_being_clicked == wnd) {
		wnd->_parent._parent.x = cursorx - wnd->grabx;
		wnd->_parent._parent.y = cursory - wnd->graby;
		wnd->_parent.need_layout = 1;
		/*should just update the positions though...*/
	}
	if (wnd->_parent.need_layout) {
		update_layout(wnd);
		wnd->_parent.need_layout = 0;
	}
}

void ui_wnd_draw(struct UI_WINDOW *wnd)
{
	struct UI_ELEMENT *elem = (struct UI_ELEMENT*) wnd;

	ui_element_draw_background(elem, 0x88000000);
	game_DrawRect(elem->x, elem->y, elem->width, -buttonheight, 0xcc000000);
	game_TextPrintString(elem->x + fontpad,
		elem->y - buttonheight + fontpad,
		wnd->title);
	ui_cnt_draw((struct UI_CONTAINER*) wnd);
}

int ui_wnd_mousedown(struct UI_WINDOW *wnd)
{
	int res;

	if ((res = ui_cnt_mousedown((struct UI_CONTAINER*) wnd))) {
		/*container can be clicked, but window should not be*/
		if (ui_element_being_clicked == wnd) {
			ui_element_being_clicked = NULL;
		}
		return res;
	}
	if (wnd->_parent._parent.x <= cursorx &&
		cursorx < wnd->_parent._parent.x + wnd->_parent._parent.width &&
		wnd->_parent._parent.y - buttonheight <= cursory &&
		cursory < wnd->_parent._parent.y)
	{
		wnd->grabx = cursorx - wnd->_parent._parent.x;
		wnd->graby = cursory - wnd->_parent._parent.y;
		return (int) (ui_element_being_clicked = wnd);
	}
	return 0;
}

int ui_wnd_mouseup(struct UI_WINDOW *wnd)
{
	return ui_cnt_mouseup((struct UI_CONTAINER*) wnd);
}

void ui_wnd_add_child(struct UI_WINDOW *wnd, void *child)
{
	ui_cnt_add_child((struct UI_CONTAINER*) wnd, child);
}
