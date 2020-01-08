/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ui.h"

void ui_element_draw_background(struct UI_ELEMENT *elem, int argb)
{
	game_DrawRect(elem->x, elem->y, elem->width, elem->height, argb);
}

int ui_element_is_hovered(struct UI_ELEMENT *element)
{
	return element->x <= cursorx &&
		element->y <= cursory &&
		cursorx < element->x + element->width &&
		cursory < element->y + element->height;
}
