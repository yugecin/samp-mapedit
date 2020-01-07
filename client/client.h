/* vim: set filetype=c ts=8 noexpandtab: */

#define STATIC_ASSERT(E) typedef char __static_assert_[(E)?1:-1]
#define EXPECT_SIZE(S,SIZE) STATIC_ASSERT(sizeof(S)==(SIZE))

EXPECT_SIZE(char, 1);
EXPECT_SIZE(short, 2);
EXPECT_SIZE(int, 4);

typedef void (cb)();

struct CLIENTLINK {
	cb *proc_loader_reload_client;
	cb *proc_loader_unload_client;
	cb *proc_client_clientmain;
	cb *proc_client_finalize;
};

typedef void (MapEditMain_t)(struct CLIENTLINK *data);
