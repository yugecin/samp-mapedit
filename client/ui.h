/* vim: set filetype=c ts=8 noexpandtab: */

#include "ui_element.h"
#include "ui_container.h"

#include "ui_list.h"
#include "ui_input.h"
#include "ui_label.h"
#include "ui_button.h"
#include "ui_checkbox.h"
#include "ui_radiobutton.h"
#include "ui_colorpicker.h"
#include "ui_window.h"

#define UI_DEFAULT_FONT_SIZE -2
#define UI_DEFAULT_FONT_RATIO -4

extern float fresx, fresy;
extern float canvasx, canvasy;
/**
Screen-space coordinates of the cursor.
*/
extern float cursorx, cursory;
/**
Screen-space coordinates of last background click event
*/
extern float bgclickx, bgclicky;
extern char key_w, key_a, key_s, key_d;
extern char directional_movement;
extern float font_size_x, font_size_y;
extern int fontsize, fontratio;
extern float fontheight, buttonheight, fontpadx, fontpady;
extern void *ui_element_being_clicked;
extern void *ui_active_element;
extern void (*ui_exclusive_mode)();
extern struct CEntity *clicked_entity;
extern struct CColPoint clicked_colpoint;
extern struct CEntity *hovered_entity;
extern struct CColPoint hovered_colpoint;
extern struct RwV3D *player_position_for_camera;
/**
Last char that was down, if it's not printable this will be 0
*/
extern char ui_last_char_down;
extern int ui_last_key_down;
extern int ui_mouse_is_just_down;
extern int ui_mouse_is_just_up;
extern struct UI_CONTAINER *background_element;
extern struct UI_WINDOW *main_menu, *context_menu;
extern char *debugstring;
extern int hide_all_ui;

#define NEW_HUD_SCALE_X 0.0009f
#define NEW_HUD_SCALE_Y 0.0014f

extern float originalHudScaleX, originalHudScaleY;

/**
To be called after writing to debugstring
*/
void ui_push_debug_string();
void ui_default_font();
void ui_dispose();
void ui_init();
void ui_on_mousewheel(int value);
void ui_render();
void ui_do_cursor_movement();
void ui_grablastkey();
void ui_show_window(struct UI_WINDOW *wnd);
void ui_hide_window();
void ui_set_fontsize(int fontsize, int fontratio);
void ui_prj_save(/*FILE*/ void *f, char *buf);
int ui_prj_load_line(char *buf);
void ui_prj_postload();
int ui_is_cursor_hovering_any_window();
void ui_get_clicked_position(struct RwV3D *pos);
void ui_get_entity_pointed_at(void **entity, struct CColPoint *colpoint);
struct UI_WINDOW *ui_get_active_window();
void ui_update_camera();
void ui_update_camera_after_manual_position();
void ui_store_camera();
void ui_set_trapped_in_ui(int flag);
void ui_draw_debug_strings();
void ui_draw_cursor();
void ui_do_exclusive_mode_basics(struct UI_WINDOW *wnd, int allow_camera_move);
int ui_handle_keydown(int vk);
int ui_handle_keyup(int vk);
int ui_handle_char(char c);
void ui_open_persistent_state();
void ui_draw_default_help_text();
void ui_place_camera_behind_player();
