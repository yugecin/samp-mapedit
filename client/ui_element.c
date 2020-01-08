/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "ui.h"

int ui_element_is_hovered(struct UI_ELEMENT *element)
{
	return element->x <= cursorx &&
		element->y <= cursory &&
		cursorx < element->x + element->width &&
		cursory < element->y + element->height;
}
