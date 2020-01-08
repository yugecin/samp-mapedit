/* vim: set filetype=c ts=8 noexpandtab: */

struct UI_BUTTON {
	struct UI_ELEMENT _parent;
	char *text;
	cb *cb;
};

struct UI_BUTTON *ui_btn_make(float x, float y, char *text, cb *cb);
void ui_btn_dispose(struct UI_BUTTON *btn);
void ui_btn_draw(struct UI_BUTTON *btn);
/**
Recalculates the width and height of the button.

The correct font properties must be active for this to work!
*/
void ui_btn_recalc_size(struct UI_BUTTON *btn);
int ui_btn_mousedown(struct UI_BUTTON *btn);
int ui_btn_mouseup(struct UI_BUTTON *btn);
