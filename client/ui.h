/* vim: set filetype=c ts=8 noexpandtab: */

#include "ui_element.h"
#include "ui_container.h"

#include "ui_label.h"
#include "ui_button.h"
#include "ui_radiobutton.h"
#include "ui_colorpicker.h"
#include "ui_window.h"

extern float fresx, fresy;
extern float canvasx, canvasy;
/**
Screen-space coordinates of the cursor.
*/
extern float cursorx, cursory;
extern char key_w, key_a, key_s, key_d;
extern float font_size_x, font_size_y;
extern int fontsize, fontratio;
extern float fontheight, buttonheight, fontpadx, fontpady;
extern void* ui_element_being_clicked;
extern int ui_mouse_is_just_down;
extern int ui_mouse_is_just_up;
extern struct UI_CONTAINER *background_element;

void ui_default_font();
void ui_dispose();
void ui_init();
void ui_render();
void ui_show_window(struct UI_WINDOW *wnd);
void ui_hide_window(struct UI_WINDOW *wnd);
void ui_set_fontsize(int fontsize, int fontratio);
