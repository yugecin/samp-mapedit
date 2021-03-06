/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ui.h"
#include <string.h>

#define GRABX_VAL_CLOSEBTN -1000
#define GRABX_VAL_NOGRAB -2000

int wnd_proc_close(struct UI_WINDOW *wnd)
{
	ui_hide_window();
	return 1;
}

struct UI_WINDOW *ui_wnd_make(float x, float y, char *title)
{
	struct UI_WINDOW *wnd;

	wnd = malloc(sizeof(struct UI_WINDOW));
	ui_elem_init(wnd, UIE_WINDOW);
	wnd->_parent._parent.x = x;
	wnd->_parent._parent.y = y;
	wnd->_parent._parent.proc_dispose = (ui_method*) ui_wnd_dispose;
	wnd->_parent._parent.proc_update = (ui_method*) ui_wnd_update;
	wnd->_parent._parent.proc_draw = (ui_method*) ui_wnd_draw;
	wnd->_parent._parent.proc_mousedown = (ui_method*) ui_wnd_mousedown;
	wnd->_parent._parent.proc_mouseup = (ui_method*) ui_wnd_mouseup;
	wnd->_parent._parent.proc_mousewheel = (ui_method1*) ui_wnd_mousewheel;
	/*recalcsize proc can be container proc*/
	wnd->_parent._parent.proc_recalc_size = (ui_method*) ui_cnt_recalc_size;
	wnd->_parent.childcount = 0;
	wnd->_parent.need_layout = 1;
	wnd->proc_close = wnd_proc_close;
	wnd->proc_shown = ui_elem_dummy_proc;
	wnd->columns = 1;
	wnd->title = title;
	wnd->closeable = 1;
	return wnd;
}

static int is_closebtn_hovered(struct UI_WINDOW *wnd)
{
	struct UI_ELEMENT *elem;

	elem = (struct UI_ELEMENT*) wnd;
	return elem->x + elem->width - buttonheight < cursorx &&
		cursorx <= elem->x + elem->width &&
		elem->y - buttonheight < cursory &&
		cursory <= elem->y;
}

void ui_wnd_dispose(struct UI_WINDOW *wnd)
{
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
			tmp = child->pref_width;
			/*add fontpadx for first and last cols, half otherwise*/
			if (col == 0) {
				tmp += fontpadx;
			} else {
				tmp += fontpadx / 2.0f;
			}
			if (col == colcount - 1) {
				tmp += fontpadx;
			} else {
				tmp += fontpadx / 2.0f;
			}
			if (tmp > colwidths[col]) {
				colwidths[col] = tmp;
			}
			col++;
		} else {
			span = child->span;
			cw = (fontpadx * 2.0f + child->pref_width);
			if (col == 0) {
				cw += fontpadx / 2.0f;
			}
			if (col + span == colcount) {
				cw += fontpadx / 2.0f;
			}
			cw /= span;
			while (span-- > 0) {
				tmp = cw;
				if ((span == child->span - 1 && col == 0) ||
					(span == 0 && col == colcount - 1))
				{
					tmp += fontpadx / 2.0f;
				}
				if (tmp > colwidths[col]) {
					colwidths[col] = tmp;
				}
				col++;
			}
		}
		tmp = fontpady * 2.0f + child->pref_height;
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
	if (tmp < 20.0f) {
		tmp2 = (100.0f - tmp) / colcount;
		for (i = 0; i < colcount; i++) {
			colwidths[i] += tmp2;
		}
		tmp = 20.0f;
	}
	wnd->_parent._parent.width = tmp;
	tmp = 0.0f;
	for (i = 0; i < rowcount; i++) {
		tmp += rowheights[i];
	}
	if (tmp < 20.0f) {
		tmp2 = (20.0f - tmp) / rowcount;
		for (i = 0; i < rowcount; i++) {
			rowheights[i] += tmp2;
		}
		tmp = 20.0f;
	}
	wnd->_parent._parent.height = tmp;

	col = 0;
	row = 0;
	colx = wnd->_parent._parent.x;
	rowy = wnd->_parent._parent.y;
	for (e = 0; e < wnd->_parent.childcount; e++) {
		child = wnd->_parent.children[e];
		tmp = -fontpadx;
		if (col == 0) {
			child->x = colx + fontpadx;
			tmp -= fontpadx / 2.0f;
		} else {
			child->x = colx + fontpadx / 2.0f;
		}
		child->y = rowy + fontpady;
		span = child->span;
		while (span-- > 0) {
			tmp += colwidths[col];
			colx += colwidths[col];
			if (col == colcount - 1) {
				tmp -= fontpadx / 2.0f;
			}
			col++;
		}
		child->width = tmp;
		child->height = rowheights[row] - fontpady * 2.0f;

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
	float x, y, dx, dy;
	int i;

	for (i = 0; i < wnd->_parent.childcount; i++) {
		child = wnd->_parent.children[i];
		child->proc_update(child);
	}
	if (ui_element_being_clicked == wnd &&
		wnd->grabx != GRABX_VAL_CLOSEBTN &&
		wnd->grabx != GRABX_VAL_NOGRAB)
	{
		x = cursorx - wnd->grabx;
		y = cursory - wnd->graby;
		if (x + wnd->_parent._parent.width > fresx) {
			x = fresx - wnd->_parent._parent.width;
		}
		if (x < 0.0f) {
			x = 0.0f;
		}
		if (y + wnd->_parent._parent.height > fresy) {
			y = fresy - wnd->_parent._parent.height;
		}
		if (y < buttonheight) {
			y = buttonheight;
		}
		dx = x - wnd->_parent._parent.x;
		dy = y - wnd->_parent._parent.y;
		wnd->_parent._parent.x = x;
		wnd->_parent._parent.y = y;
		if (!wnd->_parent.need_layout) {
			for (i = 0; i < wnd->_parent.childcount; i++) {
				child = wnd->_parent.children[i];
				child->x += dx;
				child->y += dy;
				child->proc_post_layout(child);
			}
		}
	}
	if (wnd->_parent.need_layout) {
		update_layout(wnd);
		child = (struct UI_ELEMENT*) wnd;
		x = child->x;
		y = child->y;
		dx = 0;
		dy = 0;
		if (child->x + child->width > fresx) {
			x = fresx - child->width;
			dx = x - child->x;
		}
		if (child->y + child->height > fresy) {
			y = fresy - child->height;
			dy = y - child->y;
		}
		if (x < 0) {
			dx -= x;
			x = 0;
		}
		if (y < buttonheight) {
			dy -= y - buttonheight;
			y = buttonheight;
		}
		if (dx != 0 || dy != 0) {
			child->x = x;
			child->y = y;
			for (i = 0; i < wnd->_parent.childcount; i++) {
				child = wnd->_parent.children[i];
				child->x += dx;
				child->y += dy;
			}
		}
		wnd->_parent.need_layout = 0;
		for (i = 0; i < wnd->_parent.childcount; i++) {
			child = wnd->_parent.children[i];
			child->proc_post_layout(child);
		}
	}
}

void ui_wnd_draw(struct UI_WINDOW *wnd)
{
	int closecol;
	struct UI_ELEMENT *elem = (struct UI_ELEMENT*) wnd;

	ui_element_draw_background(elem, 0x88000000);
	game_DrawRect(elem->x, elem->y, elem->width, -buttonheight, 0xcc000000);
	if (wnd->closeable) {
		if (is_closebtn_hovered(wnd)) {
			if (ui_element_being_clicked == wnd) {
				closecol = 0xffbb1111;
			} else {
				closecol = 0xccbb1111;
			}
		} else {
			closecol = 0x99bb1111;
		}
		game_DrawRect(elem->x + elem->width, elem->y,
			-buttonheight, -buttonheight, closecol);
	}
	game_TextSetAlign(LEFT);
	game_TextPrintString(elem->x + fontpadx,
		elem->y - buttonheight + fontpady,
		wnd->title);
	ui_cnt_draw((struct UI_CONTAINER*) wnd);
}

int ui_wnd_mousedown(struct UI_WINDOW *wnd)
{
	struct UI_ELEMENT *elem;
	int res;

	elem = (void*) wnd;
	wnd->grabx = GRABX_VAL_NOGRAB;
	if (elem->x <= cursorx &&
		cursorx < elem->x + elem->width &&
		elem->y - buttonheight <= cursory &&
		cursory < elem->y)
	{
		if (wnd->closeable && is_closebtn_hovered(wnd)) {
			wnd->grabx = GRABX_VAL_CLOSEBTN;
		} else {
			wnd->grabx = cursorx - wnd->_parent._parent.x;
			wnd->graby = cursory - wnd->_parent._parent.y;
		}
		return (int) (ui_element_being_clicked = wnd);
	}
	if ((res = ui_cnt_mousedown((struct UI_CONTAINER*) wnd))) {
		return res;
	}
	return 0;
}

int ui_wnd_mouseup(struct UI_WINDOW *wnd)
{
	if (wnd->closeable &&
		wnd->grabx == GRABX_VAL_CLOSEBTN &&
		is_closebtn_hovered(wnd))
	{
		wnd->proc_close(wnd);
		return 1;
	}
	return ui_cnt_mouseup((struct UI_CONTAINER*) wnd);
}

int ui_wnd_mousewheel(struct UI_WINDOW *wnd, int value)
{
	return ui_cnt_mousewheel((struct UI_CONTAINER*) wnd, value);
}

void ui_wnd_add_child(struct UI_WINDOW *wnd, void *child)
{
	ui_cnt_add_child((struct UI_CONTAINER*) wnd, child);
}

void ui_wnd_remove_child(struct UI_WINDOW *wnd, void *child)
{
	ui_cnt_remove_child((struct UI_CONTAINER*) wnd, child);
}
