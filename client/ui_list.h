/* vim: set filetype=c ts=8 noexpandtab: */

typedef void (listcb)(struct UI_LIST *lst);

#define SCROLLBAR_WIDTH 20

struct UI_LIST {
	struct UI_ELEMENT _parent;
	char **items;
	int numitems;
	int prefpagesize, realpagesize;
	int topoffset;
	int scrolling;
	int selectedindex;
	listcb *cb;
};

struct UI_LIST *ui_lst_make(int pagesize, listcb *cb);
void ui_lst_set_data(struct UI_LIST *lst, char** items, int numitems);
