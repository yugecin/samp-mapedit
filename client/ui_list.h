/* vim: set filetype=c ts=8 noexpandtab: */

typedef void (listcb)(struct UI_LIST *lst);

#define SCROLLBAR_WIDTH 20

struct UI_LIST {
	struct UI_ELEMENT _parent;
	char **items;
	char **allItems;
	char **filteredItems;
	/*filtered to non-filtered index*/
	short *filteredIndexMapping;
	int numitems;
	int numAllitems;
	int prefpagesize, realpagesize;
	int topoffset;
	int scrolling;
	int selectedindex;
	int hoveredindex;
	listcb *cb;
	char *filter;
};

struct UI_LIST *ui_lst_make(int pagesize, listcb *cb);
void ui_lst_set_data(struct UI_LIST *lst, char** items, int numitems);
/**
@param idx absolute index (index of the item, ignoring the filter)
*/
void ui_lst_set_selected_index(struct UI_LIST *lst, int idx);
void ui_lst_recalculate_filter(struct UI_LIST *lst);
/**
@returns absolute index (index of the item, ignoring the filter)
*/
int ui_lst_get_selected_index(struct UI_LIST *lst);
/**
@param idx absolute index (index of the item, ignoring the filter)
*/
int ui_lst_is_index_valid(struct UI_LIST *lst, int idx);
