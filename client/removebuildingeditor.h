/* vim: set filetype=c ts=8 noexpandtab: */

typedef void (rbe_cb)(struct REMOVEDBUILDING *remove);

void rbe_init();
void rbe_dispose();
/*cb will be called with NULL when canceled*/
void rbe_show(struct REMOVEDBUILDING *remove, rbe_cb *cb);
int rbe_handle_keydown(int vk);
