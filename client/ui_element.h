/* vim: set filetype=c ts=8 noexpandtab: */

typedef (ui_method)(void*);

enum eUIElementType {
	DUMMY,
	BACKGROUND,
	BUTTON,
	COLORPICKER,
	WINDOW,
	LABEL,
};

struct UI_ELEMENT {
	enum eUIElementType type;
	float x, y;
	float pref_width, pref_height;
	float width, height;
	short alignment, span;
	struct UI_CONTAINER *parent;
	ui_method *proc_dispose;
	/**
	Update proc, called before parent container performed layout.
	*/
	ui_method *proc_update;
	ui_method *proc_draw;
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

extern struct UI_ELEMENT dummy_element;

void ui_elem_init(void *elem, enum eUIElementType type, float x, float y);
void ui_element_draw_background(struct UI_ELEMENT *elem, int argb);
int ui_element_is_hovered(struct UI_ELEMENT *element);
