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
void make_sure_caret_is_in_bounds(struct UI_INPUT *in)
{
	int i, min, max, lasti;
	float w, maxwidth, caretwidth;
	char c;

	if (in->displayvaluestart < in->displayvalue) {
		in->displayvaluestart = in->displayvalue;
	}
	for (i = 0; i < in->valuelen; i++) {
		in->displayvalue[i] = value_to_display_char(in->value[i]);
	}
	in->displayvalue[in->valuelen] = 0;
	if (in->cursorpos < in->displayvaluestart - in->displayvalue) {
		in->displayvaluestart = in->displayvalue + in->cursorpos;
		in->caretoffsetx = 0.0f;
	}
	maxwidth = in->_parent.width - fontpadx * 2.0f;
	caretwidth = game_TextGetSizeX("I", 0, 0);
checkagain:
	c = in->displayvalue[in->cursorpos];
	in->displayvalue[in->cursorpos] = 0;
	in->caretoffsetx = game_TextGetSizeX(in->displayvaluestart, 0, 0);
	in->displayvalue[in->cursorpos] = c;
	while (in->caretoffsetx > maxwidth - caretwidth) {
		in->displayvaluestart++;
		if (*in->displayvaluestart) {
			goto checkagain;
		}
	}

	w = game_TextGetSizeX(in->displayvaluestart, 0, 0);
	if (w > maxwidth) {
		min = in->displayvaluestart - in->displayvalue;
		max = in->valuelen;
		lasti = 0;
		do {
			i = (min + max) / 2;
			if (i == lasti) {
				break;
			}
			lasti = i;
			c = in->displayvalue[i];
			in->displayvalue[i] = 0;
			w = game_TextGetSizeX(in->displayvaluestart, 0, 0);
			in->displayvalue[i] = c;
			if (w > maxwidth) {
				max = i;
			} else {
				min = i;
			}
		} while (min != max);
		in->displayvalue[i] = 0;
	}
}

static
int ui_in_accept_key(struct UI_INPUT *in)
{
	int i;

	if (ui_last_key_down == VK_BACK) {
		/*TODO: hold ctrl (doesn't work with CKeyState)*/
		if (in->cursorpos > 0) {
			for (i = in->cursorpos - 1; i < in->valuelen; i++) {
				in->value[i] = in->value[i + 1];
			}
			in->cursorpos--;
			in->valuelen--;
			/*don't stick cursor to left border*/
			i = in->cursorpos;
			i -= in->displayvaluestart - in->displayvalue;
			if (i < 4) {
				in->displayvaluestart--;
				/*value will be checked on caret update*/
			}
		}
	} else if (ui_last_key_down == VK_DELETE) {
		if (in->cursorpos < in->valuelen) {
			for (i = in->cursorpos; i < in->valuelen; i++) {
				in->value[i] = in->value[i + 1];
			}
			in->valuelen--;
		}
	} else if (ui_last_key_down == VK_LEFT) {
		if (in->cursorpos > 0) {
			in->cursorpos--;
			/*don't stick cursor to left border*/
			i = in->cursorpos;
			i -= in->displayvaluestart - in->displayvalue;
			if (i < 4) {
				in->displayvaluestart--;
				/*value will be checked on caret update*/
			}
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
			in->value[i] = in->value[i - 1];
		}
		in->value[in->cursorpos] = ui_last_char_down;
		in->value[in->valuelen] = 0;
		in->cursorpos++;
	} else {
		return 1;
	}

	in->caretanimbasetime = *timeInGame;
	make_sure_caret_is_in_bounds(in);
	if (in->cb != NULL) {
		in->cb(in);
	}
	return 1;
}

static float lastwidth = -1.0f;

static
void ui_in_post_layout(struct UI_INPUT *in)
{
	/*this also gets called on reposition,
	but then caret calcs is not needed*/
	if (lastwidth != in->_parent.width) {
		lastwidth = in->_parent.width;
		make_sure_caret_is_in_bounds(in);
	}
}

struct UI_INPUT *ui_in_make(inputcb *cb)
{
	struct UI_INPUT *in;

	in = malloc(sizeof(struct UI_INPUT));
	ui_elem_init(in, UIE_INPUT);
	in->_parent.pref_width = 300.0f;
	in->_parent.proc_dispose = (ui_method*) ui_in_dispose;
	in->_parent.proc_draw = (ui_method*) ui_in_draw;
	in->_parent.proc_mousedown = (ui_method*) ui_in_mousedown;
	in->_parent.proc_mouseup = (ui_method*) ui_in_mouseup;
	in->_parent.proc_recalc_size = (ui_method*) ui_in_recalc_size;
	in->_parent.proc_accept_key = (ui_method*) ui_in_accept_key;
	in->_parent.proc_post_layout = (ui_method*) ui_in_post_layout;
	in->cb = cb;
	in->value[0] = 0;
	in->displayvalue[0] = 0;
	in->cursorpos = 0;
	in->valuelen = 0;
	in->caretoffsetx = 0.0f;
	in->displayvaluestart = in->displayvalue;
	in->caretanimbasetime = 0;
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
	} else if (ui_element_is_hovered(elem)) {
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
	if (ui_active_element == in &&
		(*timeInGame - in->caretanimbasetime) % 500 < 250)
	{
		game_TextSetColor(0xFFFFFF00);
		game_TextPrintString(
			in->_parent.x + fontpadx + in->caretoffsetx,
			in->_parent.y + fontpady,
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
			in->caretanimbasetime = *timeInGame;
		}
		return 1;
	}
	return 0;
}
