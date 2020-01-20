/* vim: set filetype=c ts=8 noexpandtab: */

typedef (ui_method)(void*);

enum eUIElementType {
	UIE_DUMMY,
	UIE_BACKGROUND,
	UIE_BUTTON,
	UIE_RADIOBUTTON,
	UIE_COLORPICKER,
	UIE_WINDOW,
	UIE_LABEL,
	UIE_INPUT,
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
	/**
	Called when ui font or screen resolution changes.
	*/
	ui_method *proc_recalc_size;
	/**
	Called after container layout happenend.

	Also called when the container's position changed.
	*/
	ui_method *proc_post_layout;
	/**
	Called when the element is active and a key was pressed.

	Key character can be found in ui_last_key_down
	Return 0 to let the event bubble through.
	*/
	ui_method *proc_accept_key;
	void *userdata;
};

extern struct UI_ELEMENT dummy_element;

int ui_elem_dummy_proc(struct UI_ELEMENT *elem);
void ui_elem_init(void *elem, enum eUIElementType type);
void ui_element_draw_background(struct UI_ELEMENT *elem, int argb);
int ui_element_is_hovered(struct UI_ELEMENT *element);
