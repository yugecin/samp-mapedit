/* vim: set filetype=c ts=8 noexpandtab: */

typedef void (chkcb)(struct UI_CHECKBOX *chk);

struct UI_CHECKBOX {
	struct UI_BUTTON _parent;
	chkcb *cb;
	char checked;
};

struct UI_CHECKBOX *ui_chk_make(char *text, int check, chkcb *cb);
void ui_chk_dispose(struct UI_CHECKBOX *chk);
void ui_chk_update(struct UI_CHECKBOX *chk);
int ui_chk_mouseup(struct UI_CHECKBOX *chk);
