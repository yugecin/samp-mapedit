/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "objbase.h"
#include "objects.h"
#include "objbrowser.h"
#include "player.h"
#include "removedbuildings.h"
#include "removebuildingeditor.h"
#include "sockets.h"
#include "ui.h"
#include "../shared/serverlink.h"
#include <math.h>
#include <string.h>
#include <windows.h>

struct ENTITYCOLORINFO {
	void *entity;
	int lit_flag_mask[2]; /*one for entity + one for LOD*/
};

static struct ENTITYCOLORINFO selected_entity, hovered_entity;

static void *exclusiveEntity = NULL;

struct OBJECT manipulateObject;
struct CEntity *manipulateEntity = NULL;

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
void objbase_set_position_after_creation(struct OBJECT *object)
{
	struct RwV3D pos;

	TRACE("objbase_set_position_after_creation");
	game_ObjectGetPos(object->sa_object, &pos);
	pos.x = object->temp_x;
	game_ObjectSetPos(object->sa_object, &pos);
}

static
void objbase_object_creation_confirmed(struct OBJECT *object)
{
	TRACE("objbase_object_creation_confirmed");
	objbase_set_position_after_creation(object);

	if (object == &manipulateObject) {
		manipulateEntity = object->sa_object;
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

static
__declspec(naked) void render_centity_detour()
{
	_asm {
		mov eax, exclusiveEntity
		test eax, eax
		jz continuerender
		cmp ecx, exclusiveEntity
		je continuerender
		cmp ecx, manipulateEntity
		je continuerender
		add esp, 0x4
		pop esi
		pop ecx
		ret
continuerender:
		mov esi, ecx
		mov eax, [esi+0x18]
		ret
	}
}

static
__declspec(naked) void render_water_detour()
{
	_asm {
		mov eax, exclusiveEntity
		test eax, eax
		jz continuerender
		add esp, 0x4
		pop edi
		pop esi
		pop ebp
		pop ebx
		add esp, 0x5C
		ret
continuerender:
		mov eax, 0x53C4B0
		jmp eax
	}
}

/*https://github.com/DK22Pac/plugin-sdk/blob/
8d4d2ff5502ffcb3a741cbcac238d49664689808/plugin_sa/game_sa/CEntity.h#L58*/
#define CENTITY_FLAGS_LIT 0x10000000

#define __MaybeCBuilding_GetBoundingBox 0x534120
#define __MaybeCObject_GetBoundingBox 0x5449B0

static
void objbase_color_entity(void *entity, int color, int *lit_flag_mask)
{
	void *lod;
	int *flags;

	TRACE("objbase_color_entity");
	set_entity_color_agbr(entity, color);
	flags = (int*) ((char*) entity + 0x1C);
	if (color) {
		*lit_flag_mask = *flags & CENTITY_FLAGS_LIT;
		*flags |= CENTITY_FLAGS_LIT;
	} else {
		*flags &= *lit_flag_mask | ~CENTITY_FLAGS_LIT;
	}

	lod = *((int**) ((char*) entity + 0x30));
	if (lod != NULL) {
		objbase_color_entity(lod, color, lit_flag_mask + 1);
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

static
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

#define _CScriptThread__getNumberParams 0x464080
#define _CScriptThread__setNumberParams 0x464370
#define _CObject_vtable_CEntity_render 0x534310
#define _CWorld__remove 0x563280

static
void remove_rope_registered_on_object(void *object)
{
	int i;

	TRACE("remove_rope_registered_on_object");
	for (i = 0; i < MAX_ROPES; i++) {
		if (ropes[i].ropeHolder == object) {
			game_RopeRemove(ropes + i);
			return;
		}
	}
}

static
void objbase_object_created(object, sa_object, sa_handle)
	struct OBJECT *object;
	void *sa_object;
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

static
__declspec(naked) void cworld_remove_detour()
{
	_asm {
		push [esp+0x8]
		call objbase_on_world_entity_removed
		call rb_on_entity_removed_from_world
		add esp, 0x4
		pop eax
		push esi
		mov esi, [esp+0x8]
		push eax
		ret
	}
}

static
__declspec(naked) void cworld_add_detour()
{
	_asm {
		push [esp+0x8]
		call rb_on_entity_added_to_world
		add esp, 0x4
		pop eax
		push esi
		mov esi, [esp+0x8]
		push eax
		ret
	}
}

/**
calls to _CScriptThread__setNumberParams at the near end of opcode 0107 handler
get redirected here
*/
static
__declspec(naked) void opcode_0107_detour()
{
	_asm {
		pushad
		push eax /*sa_handle*/
		push edi /*sa_object*/
		mov eax, _opcodeParameters+0x4 /*x (object)*/
		push [eax]
		call objbase_object_created
		add esp, 0xC
		popad
		mov eax, _CScriptThread__setNumberParams
		jmp eax
	}
}

/**
opcode 0108 destroy_object
Hooked on the call to CWorld__remove to remove attached ropes, for example for
model 1385, the game automagically attaches a rope to it (and reattaches one
as soon as the rope is deleted?). The client crashes when this object is
deleted, so in here the rope is deleted as well.
*/
static
__declspec(naked) void opcode_0108_detour()
{
	_asm {
		pushad
		push edi
		call remove_rope_registered_on_object
		add esp, 0x4
		popad
		mov eax, _CWorld__remove
		jmp eax
	}
}

/**
calls to _CScriptThread__getNumberParams at the beginning of opcode 0453 handler
get redirected here
*/
static
__declspec(naked) void opcode_0453_detour()
{
	_asm {
		pushad
		mov eax, _opcodeParameters
		push [eax] /*handle*/
		call objbase_object_rotation_changed
		add esp, 0x4
		popad
		mov eax, _CScriptThread__getNumberParams
		jmp eax
	}
}

struct DETOUR {
	int *target;
	int old_target;
	int new_target;
};

/*0107=5,%5d% = create_object %1o% at %2d% %3d% %4d%*/
static struct DETOUR detour_0107;
/*0108=1,destroy_object %1d%*/
static struct DETOUR detour_0108;
/*0453=4,set_object %1d% XYZ_rotation %2d% %3d% %4d%*/
static struct DETOUR detour_0453;
static struct DETOUR detour_render_object;
static struct DETOUR detour_render_centity;
static struct DETOUR detour_render_water;
static struct DETOUR detour_cworld_remove;
static struct DETOUR detour_cworld_add;

void objbase_install_detour(struct DETOUR *detour)
{
	DWORD oldvp;

	VirtualProtect(detour->target, 4, PAGE_EXECUTE_READWRITE, &oldvp);
	detour->old_target = *detour->target;
	*detour->target = (int) detour->new_target - ((int) detour->target + 4);
}

void objbase_uninstall_detour(struct DETOUR *detour)
{
	*detour->target = detour->old_target;
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
	DWORD oldvp;

	RpGeometryForAllMaterials = (int*) 0x74C790; /*laptop exe*/
	if (*RpGeometryForAllMaterials == 0x24448B10) {
		RpGeometryForAllMaterials = (int*) 0x74C7E0; /*desktop exe*/
	}

	memset(&selected_entity, 0, sizeof(selected_entity));

	detour_0107.target = (int*) 0x469896;
	detour_0107.new_target = (int) opcode_0107_detour;
	detour_0108.target = (int*) 0x4698E5;
	detour_0108.new_target = (int) opcode_0108_detour;
	detour_0453.target = (int*) 0x48A355;
	detour_0453.new_target = (int) opcode_0453_detour;
	detour_render_object.target = (int*) 0x59F1EE;
	//detour_render_object.new_target = (int) render_object_detour;
	detour_render_centity.target = (int*) 0x534313;
	detour_render_centity.new_target = (int) render_centity_detour;
	detour_render_water.target = (int*) 0x6EF658;
	detour_render_water.new_target = (int) render_water_detour;
	detour_cworld_remove.target = (int*) 0x563281;
	detour_cworld_remove.new_target = (int) cworld_remove_detour;
	detour_cworld_add.target = (int*) 0x563221;
	detour_cworld_add.new_target = (int) cworld_add_detour;
	objbase_install_detour(&detour_0107);
	objbase_install_detour(&detour_0108);
	objbase_install_detour(&detour_0453);
	//objbase_install_detour(&detour_render_object);
	objbase_install_detour(&detour_render_centity);
	VirtualProtect((void*) 0x534312, 1, PAGE_EXECUTE_READWRITE, &oldvp);
	*((char*) 0x534312) = 0xE8;
	objbase_install_detour(&detour_render_water);
	objbase_install_detour(&detour_cworld_remove);
	VirtualProtect((void*) 0x563280, 1, PAGE_EXECUTE_READWRITE, &oldvp);
	*((char*) 0x563280) = 0xE8;
	objbase_install_detour(&detour_cworld_add);
	VirtualProtect((void*) 0x563220, 1, PAGE_EXECUTE_READWRITE, &oldvp);
	*((char*) 0x563220) = 0xE8;

}

void objbase_dispose()
{
	TRACE("objbase_dispose");
	objbase_uninstall_detour(&detour_0107);
	objbase_uninstall_detour(&detour_0108);
	objbase_uninstall_detour(&detour_0453);
	//objbase_uninstall_detour(&detour_render_object);
	objbase_uninstall_detour(&detour_render_centity);
	*((char*) 0x534312) = 0x51;
	*((int*) 0x534313) = 0x8BF18B56;
	objbase_uninstall_detour(&detour_render_water);
	objbase_uninstall_detour(&detour_cworld_remove);
	*((char*) 0x563280) = 0x56;
	objbase_uninstall_detour(&detour_cworld_add);
	*((char*) 0x563220) = 0x56;

	objbase_color_new_entity(&selected_entity, NULL, 0);
	objbase_color_new_entity(&hovered_entity, NULL, 0);
}
