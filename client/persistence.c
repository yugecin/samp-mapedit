/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"

#define MAGIC_ISSET 0x80
#define MAGIC_PROJECT (MAGIC_ISSET | 0x40)
#define MAGIC_OBJECT_LAYER (MAGIC_ISSET | 0x20)

#define PROJECT_NAME_OFFSET 20
#define OBJECT_LAYER_OFFSET 1

void persistence_init()
{
	if (!(logBuffer[0] & MAGIC_ISSET)) {
		logBuffer[0] = MAGIC_ISSET;
		logBuffer[1] = 0;
	}
}

char *persistence_get_project_to_load()
{
	if (logBuffer[0] & MAGIC_PROJECT) {
		return logBuffer + PROJECT_NAME_OFFSET;
	}
	return NULL;
}

void persistence_set_project_to_load(char *project_name)
{
	logBuffer[0] |= MAGIC_PROJECT;
	strcpy(logBuffer + PROJECT_NAME_OFFSET, project_name);
}

int persistence_get_object_layerid()
{
	if (logBuffer[0] & MAGIC_OBJECT_LAYER) {
		return logBuffer[OBJECT_LAYER_OFFSET];
	}
	return -1;
}

void persistence_set_object_layerid(char layerid)
{
	logBuffer[0] |= MAGIC_OBJECT_LAYER;
	logBuffer[OBJECT_LAYER_OFFSET] = layerid;
}
