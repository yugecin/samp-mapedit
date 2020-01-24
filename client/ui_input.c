/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ui.h"
#include "vk.h"
#include <string.h>

static
char value_to_display_char(char value)
{
	if (value == ' ') {
		return '_';
	}
	return value;
}

static
int ui_in_accept_key(struct UI_INPUT *in)
{
	int i;
	char c;

	if (ui_last_key_down == VK_BACK) {
		if (in->cursorpos > 0) {
			for (i = in->cursorpos - 1; i < in->valuelen; i++) {
				in->value[i] = in->value[i + 1];
				in->displayvalue[i] = in->displayvalue[i + 1];
			}
			in->cursorpos--;
			in->valuelen--;
		}
	} else if (ui_last_key_down == VK_DELETE) {
		if (in->cursorpos < in->valuelen) {
			for (i = in->cursorpos; i < in->valuelen; i++) {
				in->value[i] = in->value[i + 1];
				in->displayvalue[i] = in->displayvalue[i + 1];
			}
			in->valuelen--;
		}
	} else if (ui_last_key_down == VK_LEFT) {
		if (in->cursorpos > 0) {
			in->cursorpos--;
		}
	} else if (ui_last_key_down == VK_RIGHT) {
		if (in->cursorpos < in->valuelen) {
			in->cursorpos++;
		}
	} else if (ui_last_char_down != 0 &&
		in->cursorpos < INPUT_TEXTLEN &&
		in->valuelen < INPUT_TEXTLEN)
	{
		in->valuelen++;
		for (i = in->valuelen; i > in->cursorpos; i--) {
			c = in->value[i - 1];
			in->displayvalue[i] = value_to_display_char(c);
			in->value[i] = c;
		}
		in->displayvalue[in->cursorpos] = ui_last_char_down;
		in->value[in->cursorpos] = ui_last_char_down;
		in->displayvalue[in->valuelen] = 0;
		in->value[in->valuelen] = 0;
		in->cursorpos++;
	}
	return 1;
}

struct UI_INPUT *ui_in_make(inputcb *cb)
{
	struct UI_INPUT *in;

	in = malloc(sizeof(struct UI_INPUT));
	ui_elem_init(in, UIE_INPUT);
	in->_parent.pref_width = 400.0f;
	in->_parent.proc_dispose = (ui_method*) ui_in_dispose;
	in->_parent.proc_draw = (ui_method*) ui_in_draw;
	in->_parent.proc_mousedown = (ui_method*) ui_in_mousedown;
	in->_parent.proc_mouseup = (ui_method*) ui_in_mouseup;
	in->_parent.proc_recalc_size = (ui_method*) ui_in_recalc_size;
	in->_parent.proc_accept_key = (ui_method*) ui_in_accept_key;
	in->cb = cb;
	in->value[0] = 0;
	in->displayvalue[0] = 0;
	in->cursorpos = 0;
	in->valuelen = 0;
	in->displayvaluestart = in->displayvalue;
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
	char tmpchr;
	float cursorx;

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
		in->displayvaluestart);
	if (ui_active_element == in && *timeInGame % 500 > 250) {
		tmpchr = in->displayvalue[in->cursorpos];
		in->displayvalue[in->cursorpos] = 0;
		cursorx = game_TextGetSizeX(in->displayvaluestart, 0, 0);
		in->displayvalue[in->cursorpos] = tmpchr;
		game_TextSetColor(0xFFFFFF00);
		game_TextPrintString(
			in->_parent.x + fontpadx + cursorx,
			in->_parent.y + fontpady + 1.0f,
			"I");
		game_TextSetColor(-1);
	}
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
