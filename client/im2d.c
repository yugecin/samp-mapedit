/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "im2d.h"
#include <math.h>

struct IM2DSPHERE *im2d_sphere_make(int colorARGB)
{
	struct IM2DSPHERE *sphere;
	int size;

	sphere = malloc(sizeof(struct IM2DSPHERE));

	size = SPHERE_VERTS;
	while (size--) {
		sphere->verts[size].near = 0;
		sphere->verts[size].far = 0x40555556;
		sphere->verts[size].col = colorARGB;
		sphere->verts[size].u = 1.0f;
		sphere->verts[size].v = 0.0f;
	}

	return sphere;
}

void im2d_sphere_pos(struct IM2DSPHERE *sphere, struct RwV3D *pos, float rad)
{
	float angle_delta_v, angle_delta_h;
	float x, y, z, currentRowZ, nextRowZ, twoDeeAngle;
	float twoDeeRadUpper, twoDeeRadLower;
	int i, j;
	struct RwV3D *p;

	angle_delta_v = M_PI / (SPHERE_SEGMENTS - 1);
	angle_delta_h = 2 * M_PI / SPHERE_SEGMENTS;

	twoDeeRadUpper = twoDeeRadLower = cosf(M_PI2 - angle_delta_v) * rad;
	/*top*/
	sphere->pos[0].x = pos->x;
	sphere->pos[0].y = pos->y;
	sphere->pos[0].z = pos->z + rad;
	z = pos->z + rad * sinf(M_PI2 - angle_delta_v);
	p = sphere->pos;
	for (i = 0; i < SPHERE_SEGMENTS + 1; i++) {
		p++;
		p->x = pos->x + cosf(i * angle_delta_h) * twoDeeRadUpper;
		p->y = pos->y + sinf(i * angle_delta_h) * twoDeeRadUpper;
		p->z = z;
	}
	/*bottom*/
	p++;
	p->x = pos->x;
	p->y = pos->y;
	p->z = pos->z - rad;
	z = pos->z - rad * sinf(M_PI2 - angle_delta_v);
	for (i = 0; i < SPHERE_SEGMENTS + 1; i++) {
		p++;
		p->x = pos->x + cosf(i * angle_delta_h) * twoDeeRadLower;
		p->y = pos->y + sinf(i * angle_delta_h) * twoDeeRadLower;
		p->z = z;
	}
	/*rest*/
	for (i = 0; i < SPHERE_NUM_ROWS; i++) {
		currentRowZ = M_PI2 - angle_delta_v * (i + 1);
		nextRowZ = currentRowZ - angle_delta_v;
		twoDeeRadUpper = cosf(currentRowZ) * rad;
		twoDeeRadLower = cosf(nextRowZ) * rad;
		currentRowZ = pos->z + sinf(currentRowZ) * rad;
		nextRowZ = pos->z + sinf(nextRowZ) * rad;
		twoDeeAngle = 0;
		for (j = 0; j < SPHERE_SEGMENTS + 1; j++) {
			x = cosf(twoDeeAngle);
			y = sinf(twoDeeAngle);
			p++;
			p->x = pos->x + x * twoDeeRadUpper;
			p->y = pos->y + y * twoDeeRadUpper;
			p->z = currentRowZ;
			p++;
			p->x = pos->x + x * twoDeeRadLower;
			p->y = pos->y + y * twoDeeRadLower;
			p->z = nextRowZ;
			twoDeeAngle += angle_delta_h;
		}
	}
}

void im2d_sphere_project(struct IM2DSPHERE *sphere)
{
	struct RwV3D *p;
	struct IM2DVERTEX *v;
	int i;
	int segmentidx;

	p = sphere->pos;
	v = sphere->verts;
	for (i = 0; i < SPHERE_VERTS; i++) {
		game_WorldToScreen((struct RwV3D*) v, p);
		if (v->near < 0) {
			if (i < SPHERE_NV_TOP_BOT) {
				/*top segment*/
				sphere->verts[0].near = -1;
			} else if (i < SPHERE_NV_TOP_BOT * 2) {
				/*bottom segment*/
				sphere->verts[SPHERE_NV_TOP_BOT].near = -1;
			} else {
				/*a row*/
				segmentidx = i - SPHERE_NV_TOP_BOT * 2;
				segmentidx /= SPHERE_NV_ROW;
				segmentidx *= SPHERE_NV_ROW;
				segmentidx += SPHERE_NV_TOP_BOT * 2;
				sphere->verts[segmentidx].near = -1;
			}
		}
		v->near = 0;
		p++;
		v++;
	}
}

void im2d_sphere_draw(struct IM2DSPHERE *sphere)
{
	struct IM2DVERTEX *v;
	int i = 0;

	v = sphere->verts;
	game_RwIm2DPrepareRender();
	/*top*/
	if (v->near != -1) {
		game_RwIm2DRenderPrimitive(5, v, SPHERE_NV_TOP_BOT);
	}
	/*bottom*/
	v += SPHERE_NV_TOP_BOT;
	if (v->near != -1) {
		game_RwIm2DRenderPrimitive(5, v, SPHERE_NV_TOP_BOT);
	}
	/*middle parts*/
	v += SPHERE_NV_TOP_BOT;
	for (i = 0; i < SPHERE_SEGMENTS - 3; i++) {
		if (v->near != -1) {
			game_RwIm2DRenderPrimitive(4, v, SPHERE_NV_ROW);
		}
		v += SPHERE_NV_ROW;
	}
}
