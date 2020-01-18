/* vim: set filetype=c ts=8 noexpandtab: */

typedef void (cb)();

struct CLIENTLINK {
	cb *proc_loader_reload_client;
	cb *proc_loader_unload_client;
	cb *proc_client_clientmain;
	cb *proc_client_finalize;
};
EXPECT_SIZE(struct CLIENTLINK, 4 * 4);

typedef void (MapEditMain_t)(struct CLIENTLINK *data);

extern struct CLIENTLINK *linkdata;
extern int reload_requested;
