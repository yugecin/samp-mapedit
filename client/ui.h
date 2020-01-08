/* vim: set filetype=c ts=8 noexpandtab: */

#include "ui_element.h"

#include "ui_button.h"
#include "ui_colorpicker.h"
#include "ui_window.h"

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
extern struct UI_ELEMENT *background_element;

void ui_default_font();
void ui_dispose();
void ui_draw_rect(float x, float y, float w, float h, int argb);
void ui_draw_element_rect(struct UI_ELEMENT *element, int argb);
void ui_init();
void ui_render();
