/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ui.h"
#include <math.h>

#define COLORWHEEL_SEGMENTS 50
#define VERTCOUNT (COLORWHEEL_SEGMENTS + 1)

static struct IM2DVERTEX normverts[VERTCOUNT];

static
float hue(float t)
{
	if(t < 0.0f) t += 1.0f;
	if(t > 1.0f) t -= 1.0f;
	if(t < 1.0f / 6.0f) return 255.0f * 6.0f * t;
	if(t < 1.0f / 2.0f) return 255.0f;
	if(t < 2.0f / 3.0f) return 255.0f * (4.0f - 6.0f * t);
	return 0.0f;
};

void ui_colpick_init()
{
	float angle;
	int col, i;

	normverts[0].x = 0.0f;
	normverts[0].y = 0.0f;
	normverts[0].near = 0;
	normverts[0].far = 0x40555556;
	normverts[0].col = 0xFFFFFFFF;
	normverts[0].u = 1.0f;
	normverts[0].v = 0.0f;
	for (i = 0; i < VERTCOUNT;) {
		angle = 1.0f / (float) (VERTCOUNT - 2) * (float) i;
		col = 0xFF000000;
		col |= ((unsigned char) hue(angle + 1.0f / 3.0f)) << 16;
		col |= ((unsigned char) hue(angle)) << 8;
		col |= ((unsigned char) hue(angle - 1.0f / 3.0f));
		angle *= 2.0f * M_PI;
		i++;
		normverts[i].x = cosf(angle);
		normverts[i].y = sinf(angle);
		normverts[i].near = 0;
		normverts[i].far = 0x40555556;
		normverts[i].col = col;
		normverts[i].u = 1.0f;
		normverts[i].v = 0.0f;
	}
}

struct UI_COLORPICKER *ui_colpick_make(float x, float y, float size, cb *cb)
{
	struct UI_COLORPICKER *colpick;

	colpick = malloc(sizeof(struct UI_COLORPICKER));
	colpick->_parent.type = COLORPICKER;
	colpick->_parent.x = x;
	colpick->_parent.y = y;
	colpick->_parent.width = size;
	colpick->_parent.height = size;
	colpick->_parent.proc_draw = (ui_method*) ui_colpick_draw;
	colpick->_parent.proc_dispose = (ui_method*) ui_colpick_dispose;
	colpick->_parent.proc_mousedown = (ui_method*) ui_colpick_mousedown;
	colpick->_parent.proc_mouseup = (ui_method*) ui_colpick_mouseup;
	colpick->size = size;
	colpick->cb = cb;
	return colpick;
}

void ui_colpick_dispose(struct UI_COLORPICKER *colpick)
{
	free(colpick);
}

void ui_colpick_update(struct UI_COLORPICKER *colpick)
{
	float dx, dy, dist, angle;
	int col;

	if (ui_element_being_clicked == colpick) {
		dx = cursorx - colpick->_parent.x;
		dy = cursory - colpick->_parent.y;
		dist = (float) sqrt(dx * dx + dy * dy) / colpick->size;
		if (dist > 1.0f) {
			dist = 1.0f;
		}
		colpick->last_dist = dist;
		angle = (float) atan2(dy, dx);
		colpick->last_angle = angle;
		angle = angle / M_PI / 2.0f;
		col = 0xFF000000;
		col |= ((unsigned char) hue(angle + 1.0f / 3.0f));
		col |= ((unsigned char) hue(angle)) << 8;
		col |= ((unsigned char) hue(angle - 1.0f / 3.0f)) << 16;
		if (colpick->last_selected_colorABGR != col) {
			colpick->last_selected_colorABGR = col;
			colpick->cb();
		}
	}
}

void ui_colpick_draw(struct UI_COLORPICKER *colpick)
{
	struct IM2DVERTEX verts[VERTCOUNT];
	float x, y, size;
	int i;

	x = colpick->_parent.x;
	y = colpick->_parent.y;
	size = colpick->size;
	memcpy(verts, normverts, sizeof(normverts));
	verts[0].x = x;
	verts[0].y = y;
	for (i = 1; i < VERTCOUNT; i++) {
		verts[i].x = verts[i].x * size + x;
		verts[i].y = verts[i].y * size + y;
	}
	game_RwIm2DPrepareRender();
	game_RwIm2DRenderPrimitive(5, verts, VERTCOUNT);
	size = colpick->last_dist * colpick->size;
	game_DrawRect(
		x + cosf(colpick->last_angle) * size - 3.0f,
		y + sinf(colpick->last_angle) * size - 3.0f,
		7.0f,
		7.0f,
		0xFF000000);
}

int ui_colpick_mousedown(struct UI_COLORPICKER *colpick)
{
	float dx, dy, size;

	dx = cursorx - colpick->_parent.x;
	dy = cursory - colpick->_parent.y;
	size = colpick->size;
	if (dx * dx + dy * dy < size * size) {
		return (int) (ui_element_being_clicked = colpick);
	}
	return 0;
}

int ui_colpick_mouseup(struct UI_COLORPICKER *colpick)
{
	return ui_element_being_clicked == colpick;
}
