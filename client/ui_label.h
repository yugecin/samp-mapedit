/* vim: set filetype=c ts=8 noexpandtab: */

struct UI_LABEL {
	struct UI_ELEMENT _parent;
	char *text;
};

struct UI_LABEL *ui_lbl_make(float x, float y, char *text);
void ui_lbl_dispose(struct UI_LABEL *lbl);
void ui_lbl_update(struct UI_LABEL *lbl);
void ui_lbl_draw(struct UI_LABEL *lbl);
void ui_lbl_recalc_size(struct UI_LABEL *lbl);
