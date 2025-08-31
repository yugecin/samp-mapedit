/* vim: set filetype=c ts=8 noexpandtab: */

typedef int (ui_method)(void*);
typedef int (ui_method1)(void*, void*);

#define UIPROC(ELEM,PROC,...) \
	((struct UI_ELEMENT*) ELEM)->PROC(ELEM,__VA_ARGS__)

enum eUIElementType {
	UIE_DUMMY,
	UIE_BACKGROUND,
	UIE_BUTTON,
	UIE_RADIOBUTTON,
	UIE_COLORPICKER,
	UIE_WINDOW,
	UIE_LABEL,
	UIE_INPUT,
	UIE_LIST,
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
	@returns non-zero when the event was handled
	*/
	ui_method1 *proc_mousewheel;
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
	Called when the element is active and a WM_KEYDOWN msg was received.

	Use proc_accept_char to receive translated characters instead of VKs.
	Keyrepeat events also result in calls to this function.
	Return 0 to let the event bubble through.
	*/
	ui_method1 *proc_accept_keydown;
	/**
	Called when the element is active and a WM_KEYUP msg was received.

	Use proc_accept_char to receive translated characters instead of VKs.
	Keyrepeat events also result in calls to this function.
	Return 0 to let the event bubble through.
	*/
	ui_method1 *proc_accept_keyup;
	/**
	Called when the element is active and a WM_CHAR msg was received.

	Keyrepeat events also result in calls to this function.
	The translated character is passed as the first argument.
	Return 0 to let the event bubble through.
	*/
	ui_method1 *proc_accept_char;
	void *userdata;
};

extern struct UI_ELEMENT dummy_element;

int ui_elem_dummy_proc(struct UI_ELEMENT *elem);
void ui_elem_init(void *elem, enum eUIElementType type);
void ui_element_draw_background(struct UI_ELEMENT *elem, int argb);
int ui_element_is_hovered(struct UI_ELEMENT *element);
