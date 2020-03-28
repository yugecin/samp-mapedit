/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ui.h"
#include <string.h>

struct RwV3D player_position;

void player_prj_save(FILE *f, char *buf)
{
	union {
		int i;
		float f;
		int *p;
	} value;
	struct {
		int x, y, z;
	} vec3i;

	game_PedGetPos(player, (struct RwV3D*) &vec3i, &value.f);
	fwrite(buf, sprintf(buf, "playa.pos.x %d\n", vec3i.x), 1, f);
	fwrite(buf, sprintf(buf, "playa.pos.y %d\n", vec3i.y), 1, f);
	fwrite(buf, sprintf(buf, "playa.pos.z %d\n", vec3i.z), 1, f);
	fwrite(buf, sprintf(buf, "playa.rot %d\n", value.i), 1, f);
}

void player_prj_preload()
{
	player_position.x = 0.0f;
	player_position.y = 0.0f;
	player_position.z = 2.5f;
}

int player_prj_load_line(char *buf)
{
	union {
		int i;
		float f;
		int *p;
	} value;

	if (strncmp("playa.", buf, 6) == 0) {
		if (strncmp("pos.", buf + 6, 4) == 0) {
			value.p = (int*) &player_position + buf[10] - 'x';
			*value.p = atoi(buf + 12);
		} else if (strncmp("rot ", buf + 6, 4) == 0) {
			value.i = atoi(buf + 10);
			game_PedSetRot(player, value.f);
		}
		return 1;
	}
	return 0;
}
