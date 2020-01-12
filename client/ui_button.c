/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ui.h"
#include <string.h>

struct UI_BUTTON *ui_btn_make(char *text, btncb *cb)
{
	struct UI_BUTTON *btn;
	int textlenandzero;

	textlenandzero = strlen(text) + 1;
	btn = malloc(sizeof(struct UI_BUTTON));
	ui_elem_init(btn, BUTTON);
	btn->_parent.proc_dispose = (ui_method*) ui_btn_dispose;
	btn->_parent.proc_update = (ui_method*) ui_btn_update;
	btn->_parent.proc_draw = (ui_method*) ui_btn_draw;
	btn->_parent.proc_mousedown = (ui_method*) ui_btn_mousedown;
	btn->_parent.proc_mouseup = (ui_method*) ui_btn_mouseup;
	btn->_parent.proc_recalc_size = (ui_method*) ui_btn_recalc_size;
	btn->text = malloc(sizeof(char) * textlenandzero);
	btn->cb = cb;
	memcpy(btn->text, text, textlenandzero);
	ui_btn_recalc_size(btn);
	return btn;
}

void ui_btn_dispose(struct UI_BUTTON *btn)
{
	free(btn->text);
	free(btn);
}

void ui_btn_update(struct UI_BUTTON *btn)
{
}

void ui_btn_draw(struct UI_BUTTON *btn)
{
	int col;

	if (ui_element_is_hovered(&btn->_parent)) {
		if (ui_element_being_clicked == NULL) {
			col = 0xEEFF0000;
		} else if (ui_element_being_clicked == btn) {
			col = 0xEE0000FF;
		} else {
			col = 0xAAFF0000;
		}
	} else if (ui_element_being_clicked == btn) {
		/*being clicked but not hovered = hovered color*/
		col = 0xEEFF0000;
	} else {
		col = 0xAAFF0000;
	}
	ui_element_draw_background(&btn->_parent, col);
	game_TextSetAlign(CENTER);
	game_TextPrintString(
		btn->_parent.x + btn->_parent.width / 2.0f,
		btn->_parent.y + fontpady,
		btn->text);
	game_TextSetAlign(LEFT);
}

void ui_btn_recalc_size(struct UI_BUTTON *btn)
{
	float textw;

	/*width calculations stop at whitespace, should whitespace be replaced
	with underscores for this size calculations?*/
	textw = game_TextGetSizeX(btn->text, 0, 0);
	btn->_parent.pref_width = textw + fontpadx * 2.0f;
	btn->_parent.pref_height = buttonheight;
	/*tell container it needs to redo its layout*/
	if (btn->_parent.parent != NULL) {
		btn->_parent.parent->need_layout = 1;
	}
}

int ui_btn_mousedown(struct UI_BUTTON *btn)
{
	if (ui_element_is_hovered(&btn->_parent)) {
		return (int) (ui_element_being_clicked = btn);
	}
	return 0;
}

int ui_btn_mouseup(struct UI_BUTTON *btn)
{
	if (ui_element_being_clicked == btn) {
		if (ui_element_is_hovered(&btn->_parent)) {
			btn->cb(btn);
		}
		return 1;
	}
	return 0;
}