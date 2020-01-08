/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ui.h"
#include <string.h>

struct UI_BUTTON *ui_btn_make(float x, float y, char *text, cb *cb)
{
	struct UI_BUTTON *btn;
	int textlen;

	textlen = strlen(text);
	btn = malloc(sizeof(struct UI_BUTTON));
	btn->_parent.type = BUTTON;
	btn->_parent.x = x;
	btn->_parent.y = y;
	btn->text = malloc(sizeof(char) * textlen);
	btn->cb = cb;
	memcpy(btn->text, text, textlen + 1);
	return btn;
}

void ui_btn_dispose(struct UI_BUTTON *btn)
{
	free(btn->text);
	free(btn);
}

void ui_btn_draw(struct UI_BUTTON *btn)
{
	float btnw;
	int col;

	btnw = game_TextGetSizeX(btn->text, 0, 0) + fontpad * 2.0f;
	if (ui_btn_is_hovered(btn, &btnw)) {
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
	ui_draw_rect(btn->_parent.x, btn->_parent.y, btnw, buttonheight, col);
	game_TextPrintString(
		btn->_parent.x + fontpad,
		btn->_parent.y + fontpad,
		btn->text);
}

int ui_btn_is_hovered(struct UI_BUTTON *btn, float *btnw)
{
	float w;
	if (btnw != NULL) {
		w = *btnw;
	} else {
		w = game_TextGetSizeX(btn->text, 0, 0);
	}
	return btn->_parent.x <= cursorx &&
		btn->_parent.y <= cursory &&
		cursorx < btn->_parent.x + w &&
		cursory < btn->_parent.y + buttonheight;
}

int ui_btn_handle_mousedown(struct UI_BUTTON *btn)
{
	if (ui_btn_is_hovered(btn, NULL)) {
		return (int) (ui_element_being_clicked = btn);
	}
	return 0;
}

int ui_btn_handle_mouseup(struct UI_BUTTON *btn)
{
	if (ui_element_being_clicked == btn && ui_btn_is_hovered(btn, NULL)) {
		btn->cb();
	}
	return 1;
}