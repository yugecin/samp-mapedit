/* vim: set filetype=c ts=8 noexpandtab: */

enum eUIElementType {
	BUTTON,
	COLORPICKER,
};

struct UI_ELEMENT {
	enum eUIElementType type;
	float x, y;
	float width, height;
};

#include "ui_button.h"
#include "ui_colorpicker.h"

#define UI_ELEM_COLORWHEEL ((void*) 1)
#define UI_ELEM_WORLDSPACE ((void*) 2)

extern float fresx, fresy;
extern float canvasx, canvasy;
/**
Screen-space coordinates of the cursor.
*/
extern float cursorx, cursory;
extern float fontheight, buttonheight, fontpad;
extern void* ui_element_being_clicked;
extern int ui_mouse_is_just_down;
extern int ui_mouse_is_just_up;

void ui_default_font();
void ui_dispose();
void ui_draw_rect(float x, float y, float w, float h, int argb);
void ui_init();
void ui_render();
