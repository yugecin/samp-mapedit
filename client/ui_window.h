/* vim: set filetype=c ts=8 noexpandtab: */

struct UI_WINDOW {
	struct UI_CONTAINER _parent;
	short columns;
	char *title;
};

struct UI_WINDOW *ui_wnd_make(float x, float y, char *title);
/**
Also disposes its children.
*/
void ui_wnd_dispose(struct UI_WINDOW *wnd);
void ui_wnd_update(struct UI_WINDOW *wnd);
void ui_wnd_draw(struct UI_WINDOW *wnd);
int ui_wnd_mousedown(struct UI_WINDOW *wnd);
int ui_wnd_mouseup(struct UI_WINDOW *wnd);
void ui_wnd_add_child(struct UI_WINDOW *wnd, struct UI_ELEMENT *child);
