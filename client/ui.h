/* vim: set filetype=c ts=8 noexpandtab: */

extern float fresx, fresy;
extern float canvasx, canvasy;
/**
Screen-space coordinates of the cursor.
*/
extern float cursorx, cursory;

void ui_deactivate();
void ui_draw_rect();
void ui_init();
void ui_render();
