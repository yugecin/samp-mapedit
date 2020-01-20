/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "objects.h"
#include "sockets.h"
#include "../shared/serverlink.h"

void objects_init()
{
	struct MSG msg;

	msg.id = MAPEDIT_MSG_RESETOBJECTS;
	sockets_send(&msg, sizeof(msg));
}

void objects_dispose()
{
}
