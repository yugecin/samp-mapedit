/* vim: set filetype=c ts=8 noexpandtab: */

typedef void (callback)();

struct UI_BUTTON {
	float x;
	float y;
	char *text;
	callback *cb;
};

struct UI_BUTTON *ui_btn_make(float x, float y, char *text, callback *cb);
void ui_btn_dispose(struct UI_BUTTON *btn);
void ui_btn_draw(struct UI_BUTTON *btn);
/**
@returns non-zero when the click was handled
*/
int ui_btn_handle_click(struct UI_BUTTON *btn);
/**
@param btnw width of the button or NULL if not calculated
*/
int ui_btn_is_hovered(struct UI_BUTTON *btn, float *btnw);