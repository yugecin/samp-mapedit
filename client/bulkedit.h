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
void bulkedit_render();
