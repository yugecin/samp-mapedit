/* vim: set filetype=c ts=8 noexpandtab: */

typedef void (inputcb)(struct UI_INPUT *in);

struct UI_INPUT {
	struct UI_ELEMENT _parent;
	char value[INPUT_TEXTLEN + 1];
	/**
	value but filtered (spaces into underscores)
	*/
	char displayvalue[INPUT_TEXTLEN + 1];
	/**
	ptr to start of displayvalue to display
	*/
	char *displayvaluestart;
	/**
	length of the value, without zero term
	*/
	unsigned char valuelen;
	/**
	position 0 is before the first character
	*/
	unsigned char cursorpos;
	/**
	offset from text x to caret x
	*/
	float caretoffsetx;
	int caretanimbasetime;
	inputcb *cb;
};

struct UI_INPUT *ui_in_make(inputcb *cb);
void ui_in_dispose(struct UI_INPUT *in);
void ui_in_update(struct UI_INPUT *in);
void ui_in_draw(struct UI_INPUT *in);
void ui_in_recalc_size(struct UI_INPUT *in);
int ui_in_mousedown(struct UI_INPUT *in);
int ui_in_mouseup(struct UI_INPUT *in);
void ui_in_set_text(struct UI_INPUT *in, char *text);
