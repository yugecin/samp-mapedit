/* vim: set filetype=c ts=8 noexpandtab: */

typedef void (rbe_cb)(struct REMOVEDBUILDING *remove);

void rbe_init();
void rbe_dispose();
void rbe_show(short model, struct RwV3D *origin, float radius, rbe_cb *cb);
int rbe_handle_keydown(int vk);
