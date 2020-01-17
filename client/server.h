/* vim: set filetype=c ts=8 noexpandtab: */

void server_init();
void server_recv();
void server_send(void *rpc, int len);
void server_stop();
