/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "entity.h"
#include "objects.h"
#include <math.h>

struct CEntity *exclusiveEntity = NULL;
struct CEntity *lastCarSpawned;

/*RpClump *RpClumpForAllAtomics
	(RpClump* clump, RpAtomicCallBack callback, void* pData)*/
#define RpClumpForAllAtomics 0x749B70
#define rpGEOMETRYMODULATEMATERIALCOLOR 0x00000040

/*RpGeometry *RpGeometryForAllMaterials
	(RpGeometry* geometry, RpMaterialCallBack fpCallBack, void* pData);*/
static int *RpGeometryForAllMaterials;

static
__declspec(naked) void *entity_color_material_cb(void *material, void *data)
{
	_asm {
		push esi
		mov eax, [esp+0x8]
		/*remove all textures
		mov dword ptr [eax], 0*/
		mov esi, [esp+0xC]
		mov dword ptr [eax+0x4], esi
		pop esi
		ret
	}
}

/*
See

https://github.com/DK22Pac/plugin-sdk/blob/\
master/examples/ColouredObjects/Main.cpp

https://github.com/DK22Pac/plugin-sdk/blob/\
8d4d2ff5502ffcb3a741cbcac238d49664689808/plugin_sa/game_sa/rw/rpworld.h#L1424
*/
static
__declspec(naked) void *entity_color_atomic_cb(void *atomic, void *data)
{
	_asm {
		push edx
		push esi
		mov esi, [esp+0xC] /*atomic*/
		mov esi, [esi+0x18] /*geometry*/
		test esi, esi
		jz nogeometry
		mov eax, esi
		add eax, 0x8 /*flags*/
		mov edx, [esp+0x10] /*data*/
		test edx, edx
		jnz setcolorflag
		and dword ptr [eax], ~rpGEOMETRYMODULATEMATERIALCOLOR
		jmp setcolor
setcolorflag:
		or dword ptr [eax], rpGEOMETRYMODULATEMATERIALCOLOR
setcolor:
		push [esp+0x10] /*data*/
		push entity_color_material_cb /*cb*/
		push esi /*geometry*/
		mov eax, RpGeometryForAllMaterials
		call eax
		add esp, 0xC
nogeometry:
		pop esi
		pop edx
		mov eax, [esp+0x4]
		ret
	}
}

static
__declspec(naked) void set_entity_color_agbr(void *centity, int color)
{
	_asm {
		mov eax, [esp+0x4]
		mov eax, [eax+0x18]
		test eax, eax
		jz nogeometry
		push [esp+0x8] /*data*/
		cmp byte ptr [eax], 0x2
		jne noclump
		push entity_color_atomic_cb /*cb*/
		push eax /*clump*/
		mov eax, RpClumpForAllAtomics
		call eax
		add esp, 0xC
		jmp nogeometry
noclump:
		push eax /*atomic*/
		call entity_color_atomic_cb
		add esp, 0x8
nogeometry:
		ret
	}
}

/*https://github.com/DK22Pac/plugin-sdk/blob/
8d4d2ff5502ffcb3a741cbcac238d49664689808/plugin_sa/game_sa/CEntity.h#L58*/
#define CENTITY_FLAGS_LIT 0x10000000

static
void entity_apply_color(struct CEntity *entity, int color, int *lit_flag_mask)
{
	TRACE("entity_apply_color");
	set_entity_color_agbr(entity, color);
	if (color) {
		*lit_flag_mask = entity->flags & CENTITY_FLAGS_LIT;
		entity->flags |= CENTITY_FLAGS_LIT;
	} else {
		entity->flags &= *lit_flag_mask | ~CENTITY_FLAGS_LIT;
	}

	if (entity->lod != NULL) {
		entity_apply_color(entity->lod, color, lit_flag_mask + 1);
	}
}

void entity_color(info, entity, color)
	struct ENTITYCOLORINFO *info;
	struct CEntity *entity;
	int color;
{
	TRACE("entity_color");
	if (entity == info->entity) {
		return;
	}

	if (info->entity != NULL) {
		entity_apply_color(info->entity, 0, info->lit_flag_mask);
	}

	if ((info->entity = entity) != NULL) {
		entity_apply_color(info->entity, color, info->lit_flag_mask);
	}
}

void entity_draw_bound_rect(struct CEntity *entity, int col)
{
#define P(A,B,C,D) \
	if(A.z>0.0f&&B.z>0.0f&&C.z>0.0f&&D.z>0.0f){\
	verts[0].x = A.x; verts[0].y = A.y;\
	verts[1].x = B.x; verts[1].y = B.y;\
	verts[2].x = C.x; verts[2].y = C.y;\
	verts[3].x = D.x; verts[3].y = D.y;\
	game_RwIm2DRenderPrimitive(rwPRIMTYPETRIFAN, verts, 4);}

	struct CColModel fakeColModel;
	struct CColModel *colmodel;
	struct CMatrix mat;
	struct RwV3D _a, _b, _c, _d, _e, _f, _g, _h, a, b, c, d, e, f, g, h;
	struct RwV3D colsize;
	struct IM2DVERTEX verts[] = {
		{0, 0, 0, 0x40555556, 0, 1.0f, 0.0f},
		{0, 0, 0, 0x40555556, 0, 1.0f, 0.0f},
		{0, 0, 0, 0x40555556, 0, 1.0f, 0.0f},
		{0, 0, 0, 0x40555556, 0, 1.0f, 0.0f},
	};
	char alpha;

	TRACE("entity_draw_bound_rect");
	if (entity == NULL) {
		return;
	}

	alpha = (char) (55 * (1.0f - fabs(sinf(*timeInGame * 0.004f))));
	col = (alpha << 24) | (col & 0xFFFFFF);

	colmodel = game_EntityGetColModel(entity);
	if (colmodel == NULL) {
		colmodel = &fakeColModel;
		fakeColModel.min.x = -10.0f;
		fakeColModel.min.y = -10.0f;
		fakeColModel.min.z = -10.0f;
		fakeColModel.max.x = 10.0f;
		fakeColModel.max.y = 10.0f;
		fakeColModel.max.z = 10.0f;
		col = (col & 0xFF) | (~col & 0xFFFFFF00);
	}

	if (entity->_parent.matrix != NULL) {
		mat = *entity->_parent.matrix;
	} else {
		game_ObjectGetPos(entity, &mat.pos);
		mat.up.x = 0.0f;
		mat.up.y = 1.0f;
		mat.up.z = 0.0f;
		mat.right.x = 1.0f;
		mat.right.y = 0.0f;
		mat.right.z = 0.0f;
		mat.at.x = 0.0f;
		mat.at.y = 0.0f;
		mat.at.z = 1.0f;
	}

	colsize.x = colmodel->max.x - colmodel->min.x;
	colsize.y = colmodel->max.y - colmodel->min.y;
	colsize.z = colmodel->max.z - colmodel->min.z;

	/*up and at seem to be switched in my head*/

	_a.x = mat.pos.x;
	_a.x += mat.at.x * colmodel->min.z;
	_a.x += mat.up.x * colmodel->min.y;
	_a.x += mat.right.x * colmodel->min.x;
	_a.y = mat.pos.y;
	_a.y += mat.at.y * colmodel->min.z;
	_a.y += mat.up.y * colmodel->min.y;
	_a.y += mat.right.y * colmodel->min.x;
	_a.z = mat.pos.z;
	_a.z += mat.at.z * colmodel->min.z;
	_a.z += mat.up.z * colmodel->min.y;
	_a.z += mat.right.z * colmodel->min.x;

	_b.x = _a.x + mat.right.x * colsize.x;
	_b.y = _a.y + mat.right.y * colsize.x;
	_b.z = _a.z + mat.right.z * colsize.x;

	_c.x = _b.x + mat.up.x * colsize.y;
	_c.y = _b.y + mat.up.y * colsize.y;
	_c.z = _b.z + mat.up.z * colsize.y;

	_d.x = _a.x + mat.up.x * colsize.y;
	_d.y = _a.y + mat.up.y * colsize.y;
	_d.z = _a.z + mat.up.z * colsize.y;

	_e.x = _a.x + mat.at.x * colsize.z;
	_e.y = _a.y + mat.at.y * colsize.z;
	_e.z = _a.z + mat.at.z * colsize.z;

	_f.x = _b.x + mat.at.x * colsize.z;
	_f.y = _b.y + mat.at.y * colsize.z;
	_f.z = _b.z + mat.at.z * colsize.z;

	_g.x = _c.x + mat.at.x * colsize.z;
	_g.y = _c.y + mat.at.y * colsize.z;
	_g.z = _c.z + mat.at.z * colsize.z;

	_h.x = _d.x + mat.at.x * colsize.z;
	_h.y = _d.y + mat.at.y * colsize.z;
	_h.z = _d.z + mat.at.z * colsize.z;

	game_WorldToScreen(&a, &_a);
	game_WorldToScreen(&b, &_b);
	game_WorldToScreen(&c, &_c);
	game_WorldToScreen(&d, &_d);
	game_WorldToScreen(&e, &_e);
	game_WorldToScreen(&f, &_f);
	game_WorldToScreen(&g, &_g);
	game_WorldToScreen(&h, &_h);

	verts[0].col = verts[1].col = verts[2].col = verts[3].col = col;
	game_RwIm2DPrepareRender();
	P(a, b, c, d);
	P(e, f, g, h);
	P(a, b, f, e);
	P(b, c, g, f);
	P(c, d, h, g);
	P(d, a, e, h);
#undef P
}

void entity_render_exclusively(void *entity)
{
	struct RwV3D pos;

	TRACE("entity_render_exclusively");
	exclusiveEntity = entity;
	if (entity != NULL && manipulateEntity != NULL) {
		game_ObjectGetPos(entity, &pos);
		game_ObjectSetPos(manipulateEntity, &pos);
	}
}

void entity_init()
{
	RpGeometryForAllMaterials = (int*) 0x74C790; /*laptop exe*/
	if (*RpGeometryForAllMaterials == 0x24448B10) {
		RpGeometryForAllMaterials = (int*) 0x74C7E0; /*desktop exe*/
	}
}
