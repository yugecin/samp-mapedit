/* vim: set filetype=c ts=8 noexpandtab: */

typedef void (colpickcb)(struct UI_COLORPICKER *colpick);

#define COLPICK_ALPHABAR_WIDTH 20
#define COLPICK_ALPHABAR_PAD 10

#define CLICKMODE_COLOR 1
#define CLICKMODE_ALPHA 2

struct UI_COLORPICKER {
	struct UI_ELEMENT _parent;
	float size;
	float pickerx, pickery, barx;
	int last_selected_colorABGR;
	char last_selected_alpha_idx;
	char clickmode;
	float last_angle;
	float last_dist;
	colpickcb *cb;
};

void ui_colpick_init();
struct UI_COLORPICKER *ui_colpick_make(float size, colpickcb *cb);
void ui_colpick_dispose(struct UI_COLORPICKER *colpick);
void ui_colpick_update(struct UI_COLORPICKER *colpick);
void ui_colpick_draw(struct UI_COLORPICKER *colpick);
int ui_colpick_mousedown(struct UI_COLORPICKER *colpick);
int ui_colpick_mouseup(struct UI_COLORPICKER *colpick);
