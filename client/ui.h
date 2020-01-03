/* vim: set filetype=c ts=8 noexpandtab: */

#include "ui_button.h"

extern float fresx, fresy;
extern float canvasx, canvasy;
/**
Screen-space coordinates of the cursor.
*/
extern float cursorx, cursory;
extern float fontheight, buttonheight, fontpad;

void ui_default_font();
void ui_dispose();
void ui_draw_rect(float x, float y, float w, float h, int argb);
void ui_init();
void ui_render();
