/* vim: set filetype=c ts=8 noexpandtab: */

struct UI_COLORPICKER {
	struct UI_ELEMENT _parent;
	float size;
	int last_selected_colorABGR;
	cb *cb;
};

void ui_colpick_init();
struct UI_COLORPICKER *ui_colpick_make(
	float x, float y, float size, cb *cb);
void ui_colpick_dispose(struct UI_COLORPICKER *colpick);
void ui_colpick_update(struct UI_COLORPICKER *colpick);
void ui_colpick_draw(struct UI_COLORPICKER *colpick);
/**
Does not check if an other element is focused.

@returns non-zero when the event was handled
*/
int ui_colpick_handle_mousedown(struct UI_COLORPICKER *colpick);
/**
@returns non-zero when the event was handled
*/
int ui_colpick_handle_mouseup(struct UI_COLORPICKER *colpick);
