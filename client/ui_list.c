/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ui.h"
#include <string.h>

#define MAX_ITEM_LENGTH 50
#define SCROLLBW 20.0f

static
int ui_lst_is_index_visible(struct UI_LIST *lst, int idx)
{
	return
		lst->topoffset <= idx &&
		idx < lst->topoffset + lst->realpagesize;
}

static
void ui_lst_ensure_topoffset_in_range(struct UI_LIST *lst)
{
	if (lst->topoffset > lst->numitems - lst->realpagesize) {
		lst->topoffset = lst->numitems - lst->realpagesize;
	}
	if (lst->topoffset < 0) {
		lst->topoffset = 0;
	}
}

static
void ui_lst_dispose(struct UI_LIST *lst)
{
	free(lst->allItems);
	free(lst->filteredItems);
	free(lst->filteredIndexMapping);
	free(lst);
}

static
void calc_scrollbar_size(struct UI_LIST *lst, float *y, float *height)
{
	float innerheight;

	innerheight = lst->_parent.height - 4.0f;
	if (lst->numitems >= lst->realpagesize * 2) {
		*height = fontheight;
		if (lst->topoffset == lst->numitems - lst->realpagesize) {
			*y = innerheight - fontheight;
		} else if (lst->topoffset == 0) {
			*y = 0.0f;
		} else {
			*y = innerheight - *height;
			*y *= (float) lst->topoffset /
				(float) (lst->numitems - lst->realpagesize);
		}
	} else if (lst->numitems <= lst->realpagesize) {
		*height = innerheight;
		*y = 0.0f;
	} else {
		*height = fontheight * (lst->realpagesize -
			(lst->numitems - lst->realpagesize));
		*y = fontheight * lst->topoffset;
	}
	*y += 2.0f + lst->_parent.y;
}

/**
@return hovered offset from topoffset or -1
*/
static
int calc_list_hovered_offset(struct UI_LIST *lst)
{
	int offset;

	offset = (int) ((cursory - lst->_parent.y - 2.0f) / fontheight);
	if (0 <= offset && offset < lst->realpagesize &&
		offset + lst->topoffset < lst->numitems)
	{
		return offset;
	} else {
		return -1;
	}
}

static
void ui_lst_draw(struct UI_LIST *lst)
{
	struct UI_ELEMENT *elem;
	int col;
	int i, itemend, j, ishovered, lasthoveridx;
	float scrollby, scrollbh, scrollbx;

	elem = (struct UI_ELEMENT*) lst;
	ishovered = ui_element_is_hovered(elem);
	col = 0xAAFF0000;
	if (ui_active_element == lst) {
		col = 0xEE00FF00;
	} else if (ishovered) {
		if (ui_element_being_clicked == NULL) {
			col = 0xEEFF0000;
		} else if (ui_element_being_clicked == lst) {
			col = 0xEE0000FF;
		}
	}
	ui_element_draw_background(elem, col);
	game_DrawRect(elem->x + 2.0f, elem->y + 2.0f,
		elem->width - 4.0f, elem->height - 4.0f, 0xFF000000);

	/*scrollbar*/
	game_DrawRect(
		elem->x + elem->width - 2.0f - SCROLLBW,
		elem->y + 2.0f,
		SCROLLBW,
		elem->height - 4.0f,
		0xFF333333);
	calc_scrollbar_size(lst, &scrollby, &scrollbh);
	scrollbx = elem->x + elem->width - 2.0f - SCROLLBAR_WIDTH;
	game_DrawRect(
		scrollbx,
		scrollby,
		SCROLLBAR_WIDTH,
		scrollbh,
		0xFFFF0000);

	lasthoveridx = lst->hoveredindex;
	lst->hoveredindex = -1;
	if (ishovered && cursorx < scrollbx) {
		i = calc_list_hovered_offset(lst);
		if (i != -1) {
			lst->hoveredindex = i + lst->topoffset;
			game_DrawRect(
				elem->x + 2.0f,
				elem->y + 2.0f + fontheight * i,
				elem->width - 4.0f - SCROLLBAR_WIDTH,
				fontheight,
				0xFF000077);
		}
	}
	if (lst->hoveredindex != lasthoveridx && lst->hovercb != NULL) {
		lst->hovercb(lst);
	}

	if (lst->topoffset <= lst->selectedindex &&
		lst->selectedindex < lst->topoffset + lst->realpagesize)
	{
		i = lst->selectedindex - lst->topoffset;
		game_DrawRect(
			elem->x + 2.0f,
			elem->y + 2.0f + fontheight * i,
			elem->width - 4.0f - SCROLLBAR_WIDTH,
			fontheight,
			0xFF0000CC);
	}

	itemend = lst->topoffset + lst->realpagesize;
	if (itemend > lst->numitems) {
		itemend = lst->numitems;
	}
	/*TODO: scrollbar and don't display out of right bounds*/
	game_TextSetWrapX(fresx);
	for (i = lst->topoffset, j = 0; i < itemend; i++, j++) {
		game_TextPrintString(
			elem->x + 2.0f,
			elem->y + 2.0f + j * fontheight,
			lst->items[i]);
	}
}

static
void ui_lst_recalc_size(struct UI_LIST *lst)
{
	struct UI_ELEMENT *elem;

	elem = (struct UI_ELEMENT*) lst;
	elem->pref_height = fontheight * lst->prefpagesize + 4.0f;
	/*tell container it needs to redo its layout*/
	if (elem->parent != NULL) {
		elem->parent->need_layout = 1;
	}
}

static
int ui_lst_update(struct UI_LIST *lst)
{
	float scrollby, scrollbh, tmp;
	int i;

	if (lst->scrolling) {
		calc_scrollbar_size(lst, &scrollby, &scrollbh);
		tmp = (cursory - lst->_parent.y);
		tmp /= (lst->_parent.height - 4.0f);
		i = (int) ((lst->numitems - lst->realpagesize + 1) * tmp);
		if (i >= 0 && lst->numitems > lst->realpagesize) {
			if (i > lst->numitems - lst->realpagesize) {
				lst->topoffset =
					lst->numitems - lst->realpagesize;
			} else {
				lst->topoffset = i;
			}
		}
	}
	return 0;
}

static
int ui_lst_mousedown(struct UI_LIST *lst)
{
	int offs;
	float x;

	if (ui_element_is_hovered(&lst->_parent)) {
		x = lst->_parent.x + lst->_parent.width - 2.0f;
		if (x - SCROLLBAR_WIDTH <= cursorx && cursorx < x) {
			lst->scrolling = 1;
		} else {
			offs = calc_list_hovered_offset(lst);
			if (offs >= 0) {
				lst->selectedindex = lst->topoffset + offs;
			} else {
				lst->selectedindex = -1;
			}
			if (lst->cb != NULL) {
				lst->cb(lst);
			}
		}
		return (int) (ui_element_being_clicked = lst);
	}
	return 0;
}

static
int ui_lst_mouseup(struct UI_LIST *lst)
{
	if (ui_element_being_clicked == lst) {
		lst->scrolling = 0;
		if (ui_element_is_hovered(&lst->_parent)) {
			ui_active_element = lst;
		}
		return 1;
	}
	return 0;
}

static int ui_lst_mousewheel(struct UI_LIST *lst, int value)
{
	if (ui_active_element == lst || ui_element_is_hovered((void*) lst)) {
		while (value > 0 && lst->topoffset > 0) {
			lst->topoffset--;
			value--;
		}
		while (value < 0 &&
			lst->topoffset + lst->realpagesize < lst->numitems)
		{
			lst->topoffset++;
			value++;
		}
		return 1;
	}
	return 0;
}

static
int ui_lst_post_layout(struct UI_LIST *lst)
{
	/*does topoffset need to be recalc'd here?*/
	lst->realpagesize = (int) ((lst->_parent.height - 4.0f) / fontheight);
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
	lst->_parent.proc_update = (ui_method*) ui_lst_update;
	lst->_parent.proc_mousedown = (ui_method*) ui_lst_mousedown;
	lst->_parent.proc_mouseup = (ui_method*) ui_lst_mouseup;
	lst->_parent.proc_mousewheel = (ui_method1*) ui_lst_mousewheel;
	lst->_parent.proc_recalc_size = (ui_method*) ui_lst_recalc_size;
	lst->_parent.proc_post_layout = (ui_method*) ui_lst_post_layout;
	lst->cb = cb;
	lst->hovercb = NULL;
	lst->prefpagesize = pagesize;
	lst->numitems = 0;
	lst->numAllitems = 0;
	lst->topoffset = 0;
	lst->items = NULL;
	lst->allItems = NULL;
	lst->filteredIndexMapping = NULL;
	lst->numitems = 0;
	lst->selectedindex = -1;
	lst->hoveredindex = -1;
	ui_lst_recalc_size(lst);
	return lst;
}

void ui_lst_set_data(struct UI_LIST *lst, char** items, int numitems)
{
	char **table, *names, *currentname, c;
	int i, j;

	free(lst->allItems);
	free(lst->filteredItems);
	free(lst->filteredIndexMapping);
	if (numitems <= 0) {
		lst->items = NULL;
		lst->allItems = NULL;
		lst->filteredItems = NULL;
		lst->filteredIndexMapping = NULL;
		lst->numitems = 0;
		lst->numAllitems = 0;
		lst->selectedindex = -1;
		return;
	}
	lst->selectedindex = -1;
	lst->numAllitems = numitems;
	lst->allItems = malloc((sizeof(char*) + MAX_ITEM_LENGTH) * numitems);
	lst->filteredItems = malloc(sizeof(char*) * numitems);
	lst->filteredIndexMapping = malloc(sizeof(short) * numitems);
	table = lst->allItems;
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
	ui_lst_recalculate_filter(lst);
}

/**
@return index of the item in the filtered list (if filtered), -1 when invalid
*/
static
int ui_lst_filtered_index_for_absolute_index(struct UI_LIST *lst, int idx)
{
	int i;
	if (idx < 0 || lst->numAllitems <= idx) {
		return -1;
	}
	if (lst->items == lst->allItems) {
		/*no filter*/
		return idx;
	}
	for (i = 0; i < lst->numitems; i++) {
		if (lst->filteredIndexMapping[i] == idx) {
			return i;
		}
	}
	return -1;
}

void ui_lst_set_selected_index(struct UI_LIST *lst, int idx)
{
	idx = ui_lst_filtered_index_for_absolute_index(lst, idx);

	lst->selectedindex = idx;
	if (idx != -1) {
		lst->topoffset = idx - lst->realpagesize / 2;
	}
	ui_lst_ensure_topoffset_in_range(lst);
}

int ui_lst_get_selected_index(struct UI_LIST *lst)
{
	int idx;

	idx = lst->selectedindex;
	if (idx != -1 && lst->items == lst->filteredItems) {
		idx = lst->filteredIndexMapping[idx];
	}
	return idx;
}

int ui_lst_is_index_valid(struct UI_LIST *lst, int idx)
{
	return ui_lst_filtered_index_for_absolute_index(lst, idx) != -1;
}

void ui_lst_recalculate_filter(struct UI_LIST *lst)
{
	int i;

	/*temp change selected index from filtered to absolute, see below*/
	lst->selectedindex = ui_lst_get_selected_index(lst);

	if (lst->filter == NULL || lst->filter[0] == 0) {
		lst->items = lst->allItems;
		lst->numitems = lst->numAllitems;
		ui_lst_ensure_topoffset_in_range(lst);
		goto ret;
	}


	lst->items = lst->filteredItems;
	lst->numitems = 0;
	for (i = 0; i < lst->numAllitems; i++) {
		if (stristr(lst->allItems[i], lst->filter) != NULL) {
			lst->filteredItems[lst->numitems] = lst->allItems[i];
			lst->filteredIndexMapping[lst->numitems] = i;
			lst->numitems++;
		}
	}

ret:
	ui_lst_set_selected_index(lst, lst->selectedindex);
	ui_lst_ensure_topoffset_in_range(lst);
}
