/* vim: set filetype=c ts=8 noexpandtab: */

void objui_cb_rb_save_new(struct REMOVEDBUILDING *remove);
/**
@return 0 when context menu should be suppressed
*/
int objui_on_background_element_just_clicked();
void objui_show_delete_confirm_msg(msgboxcb *cb);
void objui_show_select_layer_first_msg();
int objui_handle_esc();
void objui_ui_activated();
void objui_ui_deactivated();
void objects_on_active_window_changed(struct UI_WINDOW *wnd);
void objui_on_world_entity_removed(void *entity);
void objui_select_entity(void *entity);
void objui_layer_changed();
void objui_frame_update();
void objui_prj_preload();
void objui_prj_postload();
void objui_init();
void objui_dispose();
