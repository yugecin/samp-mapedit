/* vim: set filetype=c ts=8 noexpandtab: */

typedef void (inputcb)(struct UI_INPUT *btn);

struct UI_INPUT {
	struct UI_ELEMENT _parent;
	char value[144];
	inputcb *cb;
};

struct UI_INPUT *ui_in_make(inputcb *cb);
void ui_in_dispose(struct UI_INPUT *in);
void ui_in_update(struct UI_INPUT *in);
void ui_in_draw(struct UI_INPUT *in);
void ui_in_recalc_size(struct UI_INPUT *in);
int ui_in_mousedown(struct UI_INPUT *in);
int ui_in_mouseup(struct UI_INPUT *in);