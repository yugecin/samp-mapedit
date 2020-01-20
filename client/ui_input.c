/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ui.h"
#include <string.h>

static
int ui_in_accept_key(struct UI_INPUT *in)
{
	in->value[0] = ui_last_key_down;
	in->value[1] = 0;
	return 1;
}

struct UI_INPUT *ui_in_make(inputcb *cb)
{
	struct UI_INPUT *in;

	in = malloc(sizeof(struct UI_INPUT));
	ui_elem_init(in, UIE_INPUT);
	in->_parent.pref_width = 200.0f;
	in->_parent.proc_dispose = (ui_method*) ui_in_dispose;
	in->_parent.proc_draw = (ui_method*) ui_in_draw;
	in->_parent.proc_mousedown = (ui_method*) ui_in_mousedown;
	in->_parent.proc_mouseup = (ui_method*) ui_in_mouseup;
	in->_parent.proc_recalc_size = (ui_method*) ui_in_recalc_size;
	in->_parent.proc_accept_key = (ui_method*) ui_in_accept_key;
	in->cb = cb;
	in->value[0] = 0;
	ui_in_recalc_size(in);
	return in;
}

void ui_in_dispose(struct UI_INPUT *in)
{
	free(in);
}

void ui_in_draw(struct UI_INPUT *in)
{
	struct UI_ELEMENT *elem;
	int col;

	elem = (struct UI_ELEMENT*) in;
	col = 0xAAFF0000;
	if (ui_active_element == in) {
		col = 0xEE00FF00;
	} else if (ui_element_is_hovered(&in->_parent)) {
		if (ui_element_being_clicked == NULL) {
			col = 0xEEFF0000;
		} else if (ui_element_being_clicked == in) {
			col = 0xEE0000FF;
		}
	}
	ui_element_draw_background(elem, col);
	game_DrawRect(elem->x + 2, elem->y + 2,
		elem->width - 4, elem->height - 4, 0xFF000000);
	game_TextPrintString(
		in->_parent.x + fontpadx,
		in->_parent.y + fontpady,
		in->value);
}

void ui_in_recalc_size(struct UI_INPUT *in)
{
	in->_parent.pref_height = buttonheight;
	/*tell container it needs to redo its layout*/
	if (in->_parent.parent != NULL) {
		in->_parent.parent->need_layout = 1;
	}
}

int ui_in_mousedown(struct UI_INPUT *in)
{
	if (ui_element_is_hovered(&in->_parent)) {
		return (int) (ui_element_being_clicked = in);
	}
	return 0;
}

int ui_in_mouseup(struct UI_INPUT *in)
{
	if (ui_element_being_clicked == in) {
		if (ui_element_is_hovered(&in->_parent)) {
			ui_active_element = in;
		}
		return 1;
	}
	return 0;
}
