/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "objbase.h"
#include "objects.h"
#include "objbrowser.h"
#include "player.h"
#include "sockets.h"
#include "ui.h"
#include "vehicles.h"
#include "../shared/serverlink.h"
#include <math.h>
#include <string.h>
#include <windows.h>

struct CEntity *exclusiveEntity = NULL;

struct ENTITYCOLORINFO {
	void *entity;
	int lit_flag_mask[2]; /*one for entity + one for LOD*/
};

static struct ENTITYCOLORINFO selected_entity, hovered_entity;

struct OBJECT manipulateObject;
struct CEntity *manipulateEntity = NULL;
struct CEntity *lastCarSpawned;

/*RpGeometry *RpGeometryForAllMaterials
	(RpGeometry* geometry, RpMaterialCallBack fpCallBack, void* pData);*/
static int *RpGeometryForAllMaterials;

void objbase_set_entity_to_render_exclusively(void *entity)
{
	struct RwV3D pos;

	TRACE("objbase_set_entity_to_render_exclusively");
	exclusiveEntity = entity;
	if (entity != NULL && manipulateEntity != NULL) {
		game_ObjectGetPos(entity, &pos);
		game_ObjectSetPos(manipulateEntity, &pos);
	}
}

/**
Since x-coord of creation is a pointer to the object handle, the position needs
to be reset.
*/
static
void objbase_set_position_rotation_after_creation(struct OBJECT *object)
{
	struct RwV3D pos;
	struct MSG_NC nc;

	TRACE("objbase_set_position_rotation_after_creation");

	game_ObjectGetPos(object->sa_object, &pos);
	pos.x = object->temp_x;
	nc._parent.id = MAPEDIT_MSG_NATIVECALL;
	nc._parent.data = 0;
	nc.nc = NC_SetObjectPos;
	nc.params.asint[1] = object->samp_objectid;
	nc.params.asflt[2] = pos.x;
	nc.params.asflt[3] = pos.y;
	nc.params.asflt[4] = pos.z;
	sockets_send(&nc, sizeof(nc));

	if (object->rot != NULL) {
		nc._parent.id = MAPEDIT_MSG_NATIVECALL;
		nc._parent.data = 0;
		nc.nc = NC_SetObjectRot;
		nc.params.asint[1] = object->samp_objectid;
		nc.params.asflt[2] = object->rot->x;
		nc.params.asflt[3] = object->rot->y;
		nc.params.asflt[4] = object->rot->z;
		sockets_send(&nc, sizeof(nc));
		free(object->rot);
		object->rot = NULL;
	}
}

static
void objbase_object_creation_confirmed(struct OBJECT *object)
{
	TRACE("objbase_object_creation_confirmed");
	objbase_set_position_rotation_after_creation(object);

	if (object == &manipulateObject) {
		manipulateEntity = object->sa_object;
		manipulateEntity->flags &= ~1; /*collision flag*/
		*(float*)((char*) object->sa_object + 0x15C) = 0.01f; /*scale*/
	} else {
		objbrowser_object_created(object) ||
			objects_object_created(object);
	}
}

void objbase_mkobject(struct OBJECT *object, struct RwV3D *pos)
{
	struct MSG_NC nc;

	TRACE("objbase_mkobject");
	object->temp_x = pos->x;
	object->rot = NULL;
	object->justcreated = 1;
	object->samp_objectid = -1;
	object->sa_object = NULL;

	nc._parent.id = MAPEDIT_MSG_NATIVECALL;
	nc._parent.data = 0; /*TODO*/
	nc.nc = NC_CreateObject;
	nc.params.asint[1] = object->model;
	nc.params.asint[2] = (int) object;
	nc.params.asflt[3] = pos->y;
	nc.params.asflt[4] = pos->z;
	nc.params.asflt[5] = 0.0f;
	nc.params.asflt[6] = 0.0f;
	nc.params.asflt[7] = 0.0f;
	nc.params.asflt[8] = 500.0f;
	sockets_send(&nc, sizeof(nc));
}

void objbase_server_object_created(struct MSG_OBJECT_CREATED *msg)
{
	struct OBJECT *object;

	TRACE("objbase_server_object_created");
	object = msg->object;
	object->samp_objectid = msg->samp_objectid;
	if (!object->justcreated) {
		/*race with objects_object_rotation_changed*/
		objbase_object_creation_confirmed(object);
	}
}

/*RpClump *RpClumpForAllAtomics
	(RpClump* clump, RpAtomicCallBack callback, void* pData)*/
#define RpClumpForAllAtomics 0x749B70
#define rpGEOMETRYMODULATEMATERIALCOLOR 0x00000040

static
__declspec(naked) void *objbase_obj_material_cb(void *material, void *data)
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
__declspec(naked) void *objbase_obj_atomic_cb(void *atomic, void *data)
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
		push objbase_obj_material_cb /*cb*/
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
		push objbase_obj_atomic_cb /*cb*/
		push eax /*clump*/
		mov eax, RpClumpForAllAtomics
		call eax
		add esp, 0xC
		jmp nogeometry
noclump:
		push eax /*atomic*/
		call objbase_obj_atomic_cb
		add esp, 0x8
nogeometry:
		ret
	}
}

/*https://github.com/DK22Pac/plugin-sdk/blob/
8d4d2ff5502ffcb3a741cbcac238d49664689808/plugin_sa/game_sa/CEntity.h#L58*/
#define CENTITY_FLAGS_LIT 0x10000000

static
void objbase_color_entity(struct CEntity *entity, int color, int *lit_flag_mask)
{
	TRACE("objbase_color_entity");
	set_entity_color_agbr(entity, color);
	if (color) {
		*lit_flag_mask = entity->flags & CENTITY_FLAGS_LIT;
		entity->flags |= CENTITY_FLAGS_LIT;
	} else {
		entity->flags &= *lit_flag_mask | ~CENTITY_FLAGS_LIT;
	}

	if (entity->lod != NULL) {
		objbase_color_entity(entity->lod, color, lit_flag_mask + 1);
	}
}

/**
@return 1 if the entity is now colored, and was not already colored
*/
static
int objbase_color_new_entity(info, entity, color)
	struct ENTITYCOLORINFO *info;
	void *entity;
	int color;
{
	TRACE("objbase_color_new_entity");
	if (entity == info->entity) {
		return 0;
	}

	if (info->entity != NULL) {
		objbase_color_entity(info->entity, 0, info->lit_flag_mask);
	}

	if ((info->entity = entity) != NULL) {
		objbase_color_entity(info->entity, color, info->lit_flag_mask);
		return 1;
	}
	return 0;
}

void objbase_select_entity(void *entity)
{
	TRACE("objbase_select_entity");
	/*remove hovered entity to revert its changes first*/
	objbase_color_new_entity(&hovered_entity, NULL, 0);
	objbase_color_new_entity(&selected_entity, entity, 0xFF0000FF);
}

static
void objbase_do_hover()
{
	struct CColPoint cp;
	void *entity;

	TRACE("objbase_do_hover");
	if (objects_is_currently_selecting_object()) {
		if (ui_is_cursor_hovering_any_window()) {
			entity = NULL;
		} else {
			ui_get_entity_pointed_at(&entity, &cp);
			if (entity != NULL) {
				player_position = cp.pos;
			} else {
				game_ScreenToWorld(&player_position,
					cursorx, cursory, 200.0f);
			}
			if (entity == selected_entity.entity) {
				entity = NULL;
			}
		}
		objbase_color_new_entity(&hovered_entity, entity, 0xFFFF00FF);
	}
}

void objbase_on_world_entity_removed(void *entity)
{
	TRACE("objbase_on_world_entity_removed");
	if (entity == selected_entity.entity) {
		objects_select_entity(NULL);
	}
	if (entity == hovered_entity.entity) {
		objbase_color_new_entity(&hovered_entity, NULL, 0);
	}
}

void objbase_draw_entity_bound_rect(struct CEntity *entity, int col)
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

	TRACE("objbase_draw_entity_bound_rect");
	if (entity == NULL) {
		return;
	}

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

void objbase_frame_update()
{
	union {
		int full;
		struct {
			char b, g, r, a;
		} comps;
	} color;

	if (!objects_is_currently_selecting_object()) {
		return;
	}

	color.full = 0x00FF0000;
	color.comps.a = BBOX_ALPHA_ANIM_VALUE;
	objbase_do_hover();
	objbase_draw_entity_bound_rect(selected_entity.entity, color.full);
	color.comps.b = 0xFF;
	objbase_draw_entity_bound_rect(hovered_entity.entity, color.full);
}

void objbase_object_created(object, sa_object, sa_handle)
	struct OBJECT *object;
	struct CEntity *sa_object;
	int sa_handle;
{
	object->sa_object = sa_object;
	object->sa_handle = sa_handle;
}

void objbase_object_rotation_changed(int sa_handle)
{
	struct OBJECT *object;

	object = objects_find_by_sa_handle(sa_handle);
	if (object == NULL) {
		if (manipulateObject.sa_handle == sa_handle) {
			object = &manipulateObject;
		} else {
			return;
		}
	}
	if (object->justcreated) {
		object->justcreated = 0;
		/*race with objects_server_object_created*/
		if (object->samp_objectid != -1) {
			objbase_object_creation_confirmed(object);
		}
	}
}

void objbase_create_dummy_entity()
{
	struct RwV3D pos;

	pos.x = 0.0f;
	pos.y = 0.0f;
	pos.z = -10000.0f;
	manipulateObject.model = 1214;
	objbase_mkobject(&manipulateObject, &pos);
}

void objbase_init()
{
	RpGeometryForAllMaterials = (int*) 0x74C790; /*laptop exe*/
	if (*RpGeometryForAllMaterials == 0x24448B10) {
		RpGeometryForAllMaterials = (int*) 0x74C7E0; /*desktop exe*/
	}
}

void objbase_dispose()
{
	objbase_color_new_entity(&selected_entity, NULL, 0);
	objbase_color_new_entity(&hovered_entity, NULL, 0);
}
