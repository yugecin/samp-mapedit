/* vim: set filetype=c ts=8 noexpandtab: */

void bulkedit_reset();
int bulkedit_add(struct OBJECT *object);
int bulkedit_remove(struct OBJECT *object);
int bulkedit_is_in_list(struct OBJECT *object);
void bulkedit_begin(struct OBJECT *object);
void bulkedit_update();
void bulkedit_commit();
void bulkedit_revert();
void bulkedit_end();
void bulkedit_draw_object_boxes();
void bulkedit_update_pos_sync();
void bulkedit_update_pos_spread();
void bulkedit_update_pos_copyx();
void bulkedit_update_pos_copyy();
void bulkedit_update_pos_copyz();

extern void (*bulkedit_pos_update_method)();
