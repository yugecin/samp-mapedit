/* vim: set filetype=c ts=8 noexpandtab: */

struct UI_COLORPICKER {
	struct UI_ELEMENT _parent;
	float size;
	int last_selected_colorABGR;
	float last_angle;
	float last_dist;
	cb *cb;
};

void ui_colpick_init();
struct UI_COLORPICKER *ui_colpick_make(
	float x, float y, float size, cb *cb);
void ui_colpick_dispose(struct UI_COLORPICKER *colpick);
void ui_colpick_update(struct UI_COLORPICKER *colpick);
void ui_colpick_draw(struct UI_COLORPICKER *colpick);
int ui_colpick_mousedown(struct UI_COLORPICKER *colpick);
int ui_colpick_mouseup(struct UI_COLORPICKER *colpick);
