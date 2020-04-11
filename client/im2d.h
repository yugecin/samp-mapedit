/* vim: set filetype=c ts=8 noexpandtab: */

/*this has to be (n + 3)*/
#define SPHERE_SEGMENTS 5
/*tri fan at top and bottom, tri strip in the middle*/
#define SPHERE_NV_TOP_BOT (SPHERE_SEGMENTS + 1 + 1)
#define SPHERE_NV_ROW (SPHERE_SEGMENTS * 2 + 2)
#define SPHERE_NUM_ROWS (SPHERE_SEGMENTS - 3)
#define SPHERE_VERTS (SPHERE_NV_TOP_BOT * 2 + SPHERE_NUM_ROWS * SPHERE_NV_ROW)

struct IM2DSPHERE {
	struct IM2DVERTEX verts[SPHERE_VERTS];
	struct RwV3D pos[SPHERE_VERTS];
};

/**
Free this when done with it.
*/
struct IM2DSPHERE *im2d_sphere_make(int colorARGB);
void im2d_sphere_pos(struct IM2DSPHERE *sphere, struct RwV3D *pos, float rad);
void im2d_sphere_project(struct IM2DSPHERE *sphere);
void im2d_sphere_draw(struct IM2DSPHERE *sphere);
