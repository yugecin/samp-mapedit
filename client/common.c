/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ui.h"

#define TRACEDATA_SIZE 8000

static char *tracedata, *traceptr;

void common_init()
{
	tracedata = malloc(TRACEDATA_SIZE);
	traceptr = tracedata;
}

void common_dispose()
{
	free(tracedata);
}

unsigned char hue(float t, int component)
{
	t += 1.0f / 3.0f * component;
	if (t < 0.0f) t += 1.0f;
	if (t > 1.0f) t -= 1.0f;
	if (t < 1.0f / 6.0f) {
		return (unsigned char) (255.0f * 6.0f * t);
	}
	if (t < 1.0f / 2.0f) {
		return 255;
	}
	if( t < 2.0f / 3.0f) {
		return (unsigned char) (255.0f * (4.0f - 6.0f * t));
	}
	return 0;
}

char *stristr(char *haystack, char *needle)
{
	char *h, *n;

	if (!*needle) {
		return NULL;
	}

	for (;;) {
		h = haystack;
		n = needle;
		while ((*h | 0x20) == (*n | 0x20)) {
			n++;
			if (*n == 0) {
				return haystack;
			}
			h++;
		}
		if (*h == 0) {
			return NULL;
		}
		haystack++;
	}
}

void center_camera_on(struct RwV3D *pos)
{
	camera->lookVector.x = 45.0f;
	camera->lookVector.y = 45.0f;
	camera->lookVector.z = -25.0f;
	camera->position.x = pos->x - camera->lookVector.x;
	camera->position.y = pos->y - camera->lookVector.y;
	camera->position.z = pos->z - camera->lookVector.z;
	ui_update_camera_after_manual_position();
	ui_store_camera();
}

void common_trace(char *data)
{
	int len;

	len = strlen(data);
	if (traceptr + len >= tracedata + TRACEDATA_SIZE) {
		traceptr = tracedata;
	}
	strcpy(traceptr, data);
	*((int*) 0xBAB2C8) = (int) traceptr;
	traceptr += len + 1;
}
