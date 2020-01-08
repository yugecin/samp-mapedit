/* vim: set filetype=c ts=8 noexpandtab: */

typedef (ui_method)(void*);

enum eUIElementType {
	BACKGROUND,
	BUTTON,
	COLORPICKER,
	WINDOW,
};

struct UI_ELEMENT {
	enum eUIElementType type;
	float x, y;
	float width, height;
	ui_method *proc_draw;
	ui_method *proc_dispose;
	/**
	Does not check if an other element is focused.

	@returns non-zero when the event was handled
	*/
	ui_method *proc_mousedown;
	/**
	@returns non-zero when the event was handled
	*/
	ui_method *proc_mouseup;
};

void ui_element_draw_background(struct UI_ELEMENT *elem, int argb);
int ui_element_is_hovered(struct UI_ELEMENT *element);
