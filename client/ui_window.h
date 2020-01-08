/* vim: set filetype=c ts=8 noexpandtab: */

#define WINDOW_MAX_CHILD_COUNT 20

struct UI_WINDOW {
	struct UI_ELEMENT _parent;
	struct UI_ELEMENT *children[WINDOW_MAX_CHILD_COUNT];
	int childcount;
	char *title;
};

struct UI_WINDOW *ui_wnd_make(float x, float y, char *title);
/**
Also disposes its children.
*/
void ui_wnd_dispose(struct UI_WINDOW *wnd);
void ui_wnd_draw(struct UI_WINDOW *wnd);
/**
Recalculates the width and height of the window.

The correct font properties must be active for this to work!
*/
void ui_wnd_recalc_size(struct UI_WINDOW *wnd);
int ui_wnd_mousedown(struct UI_WINDOW *wnd);
int ui_wnd_mouseup(struct UI_WINDOW *wnd);
/**
Does not perform layout.
*/
void ui_wnd_add_child(struct UI_WINDOW *wnd, struct UI_ELEMENT *child);
