/* vim: set filetype=c ts=8 noexpandtab: */

void rbe_init();
void rbe_dispose();
void rbe_show(short model, struct RwV3D *origin, float radius);
int rbe_handle_keydown(int vk);
void rbe_on_world_entity_added(struct CEntity *entity);
void rbe_on_world_entity_removed(struct CEntity *entity);
