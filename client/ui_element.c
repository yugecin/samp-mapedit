/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ui.h"

struct UI_ELEMENT dummy_element;

int ui_elem_dummy_proc(struct UI_ELEMENT *elem)
{
	return 0;
}

void ui_elem_init(void *elem, enum eUIElementType type, float x, float y)
{
	struct UI_ELEMENT *e = (struct UI_ELEMENT*) elem;

	e->type = type;
	e->x = x;
	e->y = y;
	e->proc_dispose = (ui_method*) ui_elem_dummy_proc;
	e->proc_update = (ui_method*) ui_elem_dummy_proc;
	e->proc_draw = (ui_method*) ui_elem_dummy_proc;
	e->proc_mousedown = (ui_method*) ui_elem_dummy_proc;
	e->proc_mouseup = (ui_method*) ui_elem_dummy_proc;
	e->proc_recalc_size = (ui_method*) ui_elem_dummy_proc;
	e->alignment = 0;
	e->span = 1;
	e->parent = NULL;
	e->userdata = NULL;
}

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
