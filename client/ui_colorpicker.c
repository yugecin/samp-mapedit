/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ui.h"
#include <math.h>

#define COLORWHEEL_SEGMENTS 50
#define COLORWHEEL_VERTS (COLORWHEEL_SEGMENTS + 2)
#define ALPHABAR_SEGMENTS 20
#define ALPHABAR_VERTS ((ALPHABAR_SEGMENTS + 1) * 2)

static struct IM2DVERTEX normverts[COLORWHEEL_VERTS];
static struct IM2DVERTEX barverts[ALPHABAR_VERTS];

static
float hue(float t)
{
	if (t < 0.0f) t += 1.0f;
	if (t > 1.0f) t -= 1.0f;
	if (t < 1.0f / 6.0f) return 255.0f * 6.0f * t;
	if (t < 1.0f / 2.0f) return 255.0f;
	if( t < 2.0f / 3.0f) return 255.0f * (4.0f - 6.0f * t);
	return 0.0f;
};

void ui_colpick_init()
{
	float angle;
	int col, i, i2;

	normverts[0].x = 0.0f;
	normverts[0].y = 0.0f;
	normverts[0].near = 0;
	normverts[0].far = 0x40555556;
	normverts[0].col = 0xFFFFFFFF;
	normverts[0].u = 1.0f;
	normverts[0].v = 0.0f;
	for (i = 0; i < COLORWHEEL_VERTS;) {
		angle = 1.0f / (float) (COLORWHEEL_VERTS - 2) * (float) i;
		angle -= 1.0f / (float) COLORWHEEL_SEGMENTS / 2.0f;
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

	for (i = 0; i < ALPHABAR_VERTS; i++) {
		if (i % 2) {
			barverts[i].x = (float) COLPICK_ALPHABAR_WIDTH;
			i2 = (i - 1) / 2;
		} else {
			barverts[i].x = 0.0f;
			i2 = i / 2;
		}
		barverts[i].y = i2 / (ALPHABAR_VERTS / 2.0f - 1.0f);
		barverts[i].near = 0;
		barverts[i].far = 0x40555556;
		barverts[i].col =
			((int) (255.0f * i2 / (ALPHABAR_SEGMENTS - 1))) << 24;
		barverts[i].u = 1.0f;
		barverts[i].v = 0.0f;
	}
}

static
void ui_colpick_post_layout(struct UI_COLORPICKER *colpick)
{
	float dim, x;

	dim = colpick->size + COLPICK_ALPHABAR_PAD + COLPICK_ALPHABAR_WIDTH;
	x = dim / 2.0f;
	x += (colpick->size - x) - colpick->size / 2.0f;
	x += colpick->_parent.x;
	x += (colpick->_parent.width - dim) / 2.0f;
	colpick->pickerx = x;
	colpick->pickery = colpick->_parent.y + colpick->_parent.height / 2.0f;
	colpick->barx = x + colpick->size / 2.0f + COLPICK_ALPHABAR_PAD;
}

struct UI_COLORPICKER *ui_colpick_make(float size, colpickcb *cb)
{
	struct UI_COLORPICKER *colpick;

	colpick = malloc(sizeof(struct UI_COLORPICKER));
	ui_elem_init(colpick, COLORPICKER);
	colpick->_parent.type = COLORPICKER;
	colpick->_parent.pref_width = size + size / 10.0f;
	colpick->_parent.pref_height = size;
	colpick->_parent.proc_dispose = (ui_method*) ui_colpick_dispose;
	colpick->_parent.proc_update = (ui_method*) ui_colpick_update;
	colpick->_parent.proc_draw = (ui_method*) ui_colpick_draw;
	colpick->_parent.proc_mousedown = (ui_method*) ui_colpick_mousedown;
	colpick->_parent.proc_mouseup = (ui_method*) ui_colpick_mouseup;
	colpick->_parent.proc_post_layout = (ui_method*) ui_colpick_post_layout;
	colpick->size = size;
	colpick->cb = cb;
	colpick->last_selected_colorABGR = 0xFFFFFFFF;
	colpick->last_selected_alpha_idx = ALPHABAR_SEGMENTS - 1;
	colpick->last_angle = 0.0f;
	colpick->last_dist = 0.0f;
	return colpick;
}

void ui_colpick_dispose(struct UI_COLORPICKER *colpick)
{
	free(colpick);
}

void ui_colpick_update(struct UI_COLORPICKER *colpick)
{
	float dx, dy, dist, tmp;
	int col;

	if (ui_element_being_clicked == colpick) {
		if (colpick->clickmode == CLICKMODE_COLOR) {
			dx = cursorx - colpick->pickerx;
			dy = cursory - colpick->pickery;
			dist = (float) sqrt(dx * dx + dy * dy);
			dist *= 2.0f / colpick->size;
			if (dist > 1.0f) {
				dist = 1.0f;
			}
			colpick->last_dist = dist;
			tmp = (float) atan2(dy, dx) / M_PI / 2.0f;
			tmp += 1.0f / (float) COLORWHEEL_SEGMENTS / 2.0f;
			if (tmp < 0.0f) {
				tmp = 1.0f + tmp;
			}
			tmp = ((int) (tmp * (float) COLORWHEEL_SEGMENTS))
				/ (float) COLORWHEEL_SEGMENTS;
			colpick->last_angle = tmp * M_PI * 2.0f;
			col = ((unsigned char) hue(tmp + 1.0f / 3.0f));
			col |= ((unsigned char) hue(tmp)) << 8;
			col |= ((unsigned char) hue(tmp - 1.0f / 3.0f)) << 16;
		} else if (colpick->clickmode == CLICKMODE_ALPHA) {
			col = colpick->last_selected_colorABGR & 0xFFFFFF;
			tmp = cursory - colpick->_parent.y;
			tmp *= ALPHABAR_SEGMENTS;
			tmp /= colpick->_parent.height;
			if (tmp < 0.0f) {
				colpick->last_selected_alpha_idx = 0;
			} else if (tmp >= ALPHABAR_SEGMENTS - 1) {
				colpick->last_selected_alpha_idx =
					ALPHABAR_SEGMENTS - 1;
			} else {
				colpick->last_selected_alpha_idx = (char) tmp;
			}
		}
		tmp = 255.0f * colpick->last_selected_alpha_idx
			/ (float) (ALPHABAR_SEGMENTS - 1);
		col |= ((unsigned char) tmp) << 24;
		if (colpick->last_selected_colorABGR != col) {
			colpick->last_selected_colorABGR = col;
			colpick->cb(colpick);
		}
	}
}

void ui_colpick_draw(struct UI_COLORPICKER *colpick)
{
	struct IM2DVERTEX verts[COLORWHEEL_VERTS];
	float x, y, size;
	int i, col;

	game_RwIm2DPrepareRender();

	/*alphabar*/
	memcpy(verts, barverts, sizeof(barverts));
	x = colpick->barx;
	y = colpick->_parent.y;
	col = (colpick->last_selected_colorABGR & 0xFF) << 16;
	col |= (colpick->last_selected_colorABGR & 0xFF00);
	col |= (colpick->last_selected_colorABGR & 0xFF0000) >> 16;
	for (i = 0; i < ALPHABAR_VERTS; i++) {
		verts[i].x += x;
		verts[i].y = verts[i].y * colpick->_parent.height + y;
		verts[i].col = verts[i].col | col;
	}
	game_RwIm2DRenderPrimitive(4, verts, ALPHABAR_VERTS);

	/*alphabar indicator*/
	verts[0].col = verts[1].col = verts[2].col = 0xFF000000;
	verts[0].x = verts[1].x = colpick->barx + COLPICK_ALPHABAR_WIDTH;
	verts[2].x = verts[0].x - COLPICK_ALPHABAR_WIDTH / 2.0f;
	size = (0.5f + colpick->last_selected_alpha_idx);
	size /= (float) ALPHABAR_SEGMENTS;
	size *= colpick->_parent.height;
	size += colpick->_parent.y;
	verts[0].y = size - COLPICK_ALPHABAR_WIDTH / 4.0f;
	verts[1].y = size + COLPICK_ALPHABAR_WIDTH / 4.0f;
	verts[2].y = size;
	game_RwIm2DRenderPrimitive(4, verts, 3);

	/*colorwheel*/
	size = colpick->size / 2.0f;
	x = colpick->pickerx;
	y = colpick->pickery;
	memcpy(verts, normverts, sizeof(normverts));
	verts[0].x = x;
	verts[0].y = y;
	for (i = 1; i < COLORWHEEL_VERTS; i++) {
		verts[i].x = verts[i].x * size + x;
		verts[i].y = verts[i].y * size + y;
	}
	game_RwIm2DRenderPrimitive(5, verts, COLORWHEEL_VERTS);

	/*colorwheel indicator*/
	size *= colpick->last_dist;
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

	dx = cursorx - colpick->pickerx;
	dy = cursory - colpick->pickery;
	size = colpick->size;
	if (dx * dx + dy * dy < size * size / 4.0f) {
		colpick->clickmode = CLICKMODE_COLOR;
		return (int) (ui_element_being_clicked = colpick);
	}
	if (colpick->barx <= cursorx &&
		cursorx < colpick->barx + COLPICK_ALPHABAR_WIDTH &&
		colpick->_parent.y <= cursory &&
		cursory < colpick->_parent.y + colpick->_parent.height)
	{
		colpick->clickmode = CLICKMODE_ALPHA;
		return (int) (ui_element_being_clicked = colpick);
	}
	return 0;
}

int ui_colpick_mouseup(struct UI_COLORPICKER *colpick)
{
	return ui_element_being_clicked == colpick;
}
