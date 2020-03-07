/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ui.h"
#include <string.h>

struct UI_LABEL *ui_lbl_make(char *text)
{
	struct UI_LABEL *lbl;

	lbl = malloc(sizeof(struct UI_LABEL));
	ui_elem_init(lbl, UIE_LABEL);
	lbl->_parent.proc_dispose = (ui_method*) ui_lbl_dispose;
	lbl->_parent.proc_draw = (ui_method*) ui_lbl_draw;
	lbl->_parent.proc_recalc_size = (ui_method*) ui_lbl_recalc_size;
	lbl->text = text;
	ui_lbl_recalc_size(lbl);
	return lbl;
}

void ui_lbl_dispose(struct UI_LABEL *lbl)
{
	free(lbl);
}

void ui_lbl_draw(struct UI_LABEL *lbl)
{
	game_TextPrintString(
		lbl->_parent.x + fontpadx,
		lbl->_parent.y + fontpady,
		lbl->text);
}

void ui_lbl_recalc_size(struct UI_LABEL *lbl)
{
	float textw;

	lbl->_parent.pref_height = buttonheight;
	if (lbl->text == NULL) {
		lbl->_parent.pref_width = 10.0f;
	} else {
		/*width calculations stop at whitespace, should whitespace be
		replaced with underscores for this size calculations?*/
		textw = game_TextGetSizeX(lbl->text, 0, 0);
		lbl->_parent.pref_width = textw + fontpadx * 2.0f;
	}
	if (lbl->_parent.parent != NULL) {
		lbl->_parent.parent->need_layout = 1;
	}
}
