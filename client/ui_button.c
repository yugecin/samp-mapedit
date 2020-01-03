/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ui.h"
#include <string.h>

struct UI_BUTTON *ui_btn_make(float x, float y, char *text, callback *cb)
{
	struct UI_BUTTON *btn;
	int textlen;

	textlen = strlen(text);
	btn = malloc(sizeof(struct UI_BUTTON));
	btn->x = x;
	btn->y = y;
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
		col = 0xEEFF0000;
	} else {
		col = 0xAAFF0000;
	}
	ui_draw_rect(btn->x, btn->y, btnw, buttonheight, col);
	game_TextPrintString(btn->x + fontpad, btn->y + fontpad, btn->text);
}

int ui_btn_is_hovered(struct UI_BUTTON *btn, float *btnw)
{
	float w;
	if (btnw != NULL) {
		w = *btnw;
	} else {
		w = game_TextGetSizeX(btn->text, 0, 0);
	}
	return btn->x <= cursorx && cursorx < btn->x + w &&
		btn->y <= cursory && cursory < btn->y + buttonheight;
}