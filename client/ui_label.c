/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ui.h"
#include <string.h>

struct UI_LABEL *ui_lbl_make(float x, float y, char *text)
{
	struct UI_LABEL *lbl;
	int textlenandzero;

	textlenandzero = strlen(text) + 1;
	lbl = malloc(sizeof(struct UI_LABEL));
	ui_elem_init(lbl, LABEL, x, y);
	lbl->_parent.proc_dispose = (ui_method*) ui_lbl_dispose;
	lbl->_parent.proc_update = (ui_method*) ui_lbl_update;
	lbl->_parent.proc_draw = (ui_method*) ui_lbl_draw;
	lbl->_parent.proc_recalc_size = (ui_method*) ui_lbl_recalc_size;
	lbl->text = malloc(sizeof(char) * textlenandzero);
	memcpy(lbl->text, text, textlenandzero);
	ui_lbl_recalc_size(lbl);
	return lbl;
}

void ui_lbl_dispose(struct UI_LABEL *lbl)
{
	free(lbl->text);
	free(lbl);
}

void ui_lbl_update(struct UI_LABEL *lbl)
{
}

void ui_lbl_draw(struct UI_LABEL *lbl)
{
	game_TextPrintString(
		lbl->_parent.x + fontpad,
		lbl->_parent.y + fontpad,
		lbl->text);
}

void ui_lbl_recalc_size(struct UI_LABEL *lbl)
{
	float textw;

	/*width calculations stop at whitespace, should whitespace be replaced
	with underscores for this size calculations?*/
	textw = game_TextGetSizeX(lbl->text, 0, 0);
	lbl->_parent.pref_width = textw + fontpad * 2.0f;
	lbl->_parent.pref_height = buttonheight;
}
