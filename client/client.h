/* vim: set filetype=c ts=8 noexpandtab: */

typedef void (*unload_client_t)();
typedef void (*client_finalize_t)();
typedef client_finalize_t (*MapEditMain_t)(unload_client_t);
