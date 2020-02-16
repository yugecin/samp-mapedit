/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ui.h"
#include <string.h>

#define MAX_ITEM_LENGTH 50

static
void ui_lst_dispose(struct UI_LIST *lst)
{
	free(lst->items);
	free(lst);
}

static
void ui_lst_draw(struct UI_LIST *lst)
{
	struct UI_ELEMENT *elem;
	int col;
	int i, itemend;

	elem = (struct UI_ELEMENT*) lst;
	col = 0xAAFF0000;
	if (ui_active_element == lst) {
		col = 0xEE00FF00;
	} else if (ui_element_is_hovered(elem)) {
		if (ui_element_being_clicked == NULL) {
			col = 0xEEFF0000;
		} else if (ui_element_being_clicked == lst) {
			col = 0xEE0000FF;
		}
	}
	ui_element_draw_background(elem, col);
	game_DrawRect(elem->x + 2.0f, elem->y + 2.0f,
		elem->width - 4.0f, elem->height - 4.0f, 0xFF000000);

	itemend = lst->topoffset + lst->realpagesize;
	if (itemend > lst->numitems) {
		itemend = lst->numitems;
	}
	/*TODO: scrollbar and don't display out of right bounds*/
	game_TextSetWrapX(fresx);
	for (i = lst->topoffset; i < itemend; i++) {
		game_TextPrintString(
			elem->x + 2.0f,
			elem->y + 2.0f + i * fontheight,
			lst->items[i]);
	}
}

static
void ui_lst_recalc_size(struct UI_LIST *lst)
{
	struct UI_ELEMENT *elem;

	elem = (struct UI_ELEMENT*) lst;
	elem->pref_height = fontheight * lst->prefpagesize + fontpady * 2.0f;
	/*tell container it needs to redo its layout*/
	if (elem->parent != NULL) {
		elem->parent->need_layout = 1;
	}
}

static
int ui_lst_mousedown(struct UI_LIST *lst)
{
	if (ui_element_is_hovered(&lst->_parent)) {
		return (int) (ui_element_being_clicked = lst);
	}
	return 0;
}

static
int ui_lst_mouseup(struct UI_LIST *lst)
{
	if (ui_element_being_clicked == lst) {
		if (ui_element_is_hovered(&lst->_parent)) {
			ui_active_element = lst;
		}
		return 1;
	}
	return 0;
}

static
int ui_lst_post_layout(struct UI_LIST *lst)
{
	/*does topoffset need to be recalc'd here?*/
	lst->realpagesize =
		(int) ((lst->_parent.height - fontpady * 2.0f) / fontheight);
	return 1;
}

struct UI_LIST *ui_lst_make(int pagesize, listcb *cb)
{
	struct UI_LIST *lst;

	lst = malloc(sizeof(struct UI_LIST));
	ui_elem_init(lst, UIE_LIST);
	lst->_parent.pref_width = 300.0f;
	lst->_parent.proc_dispose = (ui_method*) ui_lst_dispose;
	lst->_parent.proc_draw = (ui_method*) ui_lst_draw;
	lst->_parent.proc_mousedown = (ui_method*) ui_lst_mousedown;
	lst->_parent.proc_mouseup = (ui_method*) ui_lst_mouseup;
	lst->_parent.proc_recalc_size = (ui_method*) ui_lst_recalc_size;
	lst->_parent.proc_post_layout = (ui_method*) ui_lst_post_layout;
	lst->cb = cb;
	lst->prefpagesize = pagesize;
	lst->numitems = 0;
	lst->topoffset = 0;
	lst->items = NULL;
	lst->numitems = 0;
	ui_lst_recalc_size(lst);
	return lst;
}

void ui_lst_set_data(struct UI_LIST *lst, char** items, int numitems)
{
	char **table, *names, *currentname, c;
	int i, j;

	if (lst->items != NULL) {
		free(lst->items);
	}
	if (numitems <= 0) {
		lst->items = NULL;
		return;
	}
	lst->numitems = numitems;
	lst->items = malloc((sizeof(char*) + MAX_ITEM_LENGTH) * numitems);
	table = lst->items;
	names = ((char*) table) + numitems * sizeof(char*);
	for (i = 0; i < numitems; i++) {
		currentname = items[i];
		for (j = 0;; j++) {
			c = currentname[j];
			if (c == 0 || j == MAX_ITEM_LENGTH - 1) {
				names[j] = 0;
				break;
			}
			names[j] = c;
		}
		*table = names;
		table++;
		names += MAX_ITEM_LENGTH;
	}
}
