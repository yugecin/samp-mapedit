/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "objbase.h"
#include "objects.h"
#include "objbrowser.h"
#include "player.h"
#include "sockets.h"
#include "ui.h"
#include "../shared/serverlink.h"
#include <string.h>
#include <windows.h>

struct ENTITYCOLORINFO {
	void *entity;
	int lit_flag_mask[2]; /*one for entity + one for LOD*/
	int lod_lit_flag_mask;
};

static struct ENTITYCOLORINFO selected_entity, hovered_entity;

static struct OBJECT exclusiveExtraObject;
static void *exclusiveExtraEntity = NULL;
static void *exclusiveEntity = NULL;

void objbase_set_entity_to_render_exclusively(void *entity)
{
	struct RwV3D pos;

	exclusiveEntity = entity;
	if (entity != NULL && exclusiveExtraEntity != NULL) {
		game_ObjectGetPos(entity, &pos);
		game_ObjectSetPos(exclusiveExtraEntity, &pos);
	}
}

/**
TODO: optimize this
*/
struct OBJECT *objects_find_by_sa_handle(int sa_handle)
{
	int i;
	struct OBJECT *objects;

	objects = objbrowser_object_by_handle(sa_handle);
	if (objects != NULL) {
		return objects;
	}

	if (active_layer != NULL) {
		objects = active_layer->objects;
		for (i = active_layer->numobjects - 1; i >= 0; i--) {
			if (objects[i].sa_handle == sa_handle) {
				return objects + i;
			}
		}
	}
	return NULL;
}

/**
TODO: optimize this
*/
struct OBJECT *objects_find_by_sa_object(void *sa_object)
{
	int i;
	struct OBJECT *objects;

	if (active_layer != NULL) {
		objects = active_layer->objects;
		for (i = active_layer->numobjects - 1; i >= 0; i--) {
			if (objects[i].sa_object == sa_object) {
				return objects + i;
			}
		}
	}
	return NULL;
}

/**
Since x-coord of creation is a pointer to the object handle, the position needs
to be reset.
*/
static
void objbase_set_position_after_creation(struct OBJECT *object)
{
	struct RwV3D pos;

	game_ObjectGetPos(object->sa_object, &pos);
	pos.x = object->temp_x;
	game_ObjectSetPos(object->sa_object, &pos);
}

static
void objbase_object_creation_confirmed(struct OBJECT *object)
{
	objbase_set_position_after_creation(object);

	if (object == &exclusiveExtraObject) {
		exclusiveExtraEntity = object->sa_object;
		*(float*)((char*) object->sa_object + 0x15C) = 0.1f; /*scale*/
	} else {
		objbrowser_object_created(object);
	}
}

void objbase_mkobject(struct OBJECT *object, struct RwV3D *pos)
{
	struct MSG_NC nc;

	object->temp_x = pos->x;
	object->justcreated = 1;
	object->samp_objectid = -1;

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
/*RpGeometry *RpGeometryForAllMaterials
	(RpGeometry* geometry, RpMaterialCallBack fpCallBack, void* pData);*/
#define RpGeometryForAllMaterials 0x74C790
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
		cmp ecx, exclusiveExtraEntity
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
#define CObject_flags_lit 0x10000000

#define __MaybeCBuilding_GetBoundingBox 0x534120
#define __MaybeCObject_GetBoundingBox 0x5449B0

static
void objbase_color_entity(void *entity, int color, int *lit_flag_mask)
{
	void *lod;
	int *flags;

	set_entity_color_agbr(entity, color);
	flags = (int*) ((char*) entity + 0x1C);
	if (color) {
		*lit_flag_mask = *flags & CObject_flags_lit;
		*flags |= CObject_flags_lit;
	} else {
		*flags &= *lit_flag_mask | ~CObject_flags_lit;
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
	/*remove hovered entity to revert its changes first*/
	objbase_color_new_entity(&hovered_entity, NULL, 0);
	objbase_color_new_entity(&selected_entity, entity, 0xFF0000FF);
}

static
void objbase_do_hover()
{
	struct CColPoint cp;
	void *entity;

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

#if 0
static
void objbase_draw_entity_bound_rect(void *entity, int color)
{
	void *proc;
	void *bb;
	void *colmodel;
	struct Rect boundrect;
	struct RwV3D world[4], *a, *b, *c, *d;
	struct RwV3D screen[4];
	struct IM2DVERTEX verts[] = {
		{0, 0, 0, 0x40555556, 0x660000FF, 1.0f, 0.0f},
		{0, 0, 0, 0x40555556, 0x660000FF, 1.0f, 0.0f},
		{0, 0, 0, 0x40555556, 0x660000FF, 1.0f, 0.0f},
		{0, 0, 0, 0x40555556, 0x660000FF, 1.0f, 0.0f},
	};
	struct RwV3D *min, *max;

	a = world;
	b = a + 1;
	c = b + 1;
	d = c + 1;
	if (entity != NULL) {
		/*

		use this: (getcolmodel) with CPlaceable::GetRightDirection etc
		_asm mov ecx, entity
		_asm mov eax, 0x535300
		_asm call eax
		_asm mov colmodel, eax
		if (colmodel) {
			min = (struct RwV3D*) colmodel;
			max = (struct RwV3D*) ((char*) colmodel + 0xC);
		}
		game_WorldToScreen(screen + 0, min);
		game_WorldToScreen(screen + 1, max);
		game_TextPrintString(screen[0].x, screen[0].y, "hi");
		game_TextPrintString(screen[1].x, screen[1].y, "hi");


		*/
		/*proc = (*((void***) entity))[9];
		bb = &boundrect;
		_asm {
			mov ecx, entity
			push bb
			call proc
		}
		sprintf(debugstring, "%f %f %f %f",
			boundrect.top, boundrect.bottom, boundrect.left,
			boundrect.right);
		ui_push_debug_string();
		sprintf(debugstring, "OKE",
			boundrect.top, boundrect.bottom, boundrect.left,
			boundrect.right);
		ui_push_debug_string();*/
		/*
		_asm {
			mov ecx, entity
			push a
			push b
			push c
			push d
			mov eax, 0x535340
			call eax
		}
		game_WorldToScreen(screen + 0, world + 0);
		game_WorldToScreen(screen + 1, world + 1);
		game_WorldToScreen(screen + 2, world + 2);
		game_WorldToScreen(screen + 3, world + 3);
		game_TextPrintString(screen[0].x, screen[0].y, "hi");
		game_TextPrintString(screen[1].x, screen[1].y, "hi");
		game_TextPrintString(screen[2].x, screen[2].y, "hi");
		game_TextPrintString(screen[3].x, screen[3].y, "hi");
		*/
		/*
		game_RwIm2DPrepareRender();
		verts[0].x = screen[0].x;
		verts[0].y = screen[0].y;
		verts[1].x = screen[1].x;
		verts[1].y = screen[1].y;
		verts[2].x = screen[3].x;
		verts[2].y = screen[3].y;
		verts[3].x = screen[2].x;
		verts[3].y = screen[2].y;
		game_RwIm2DRenderPrimitive(4, verts, 4);
		game_RwIm2DRenderPrimitive(4, verts, 4);
		game_RwIm2DRenderPrimitive(4, verts, 4);
		game_RwIm2DRenderPrimitive(4, verts, 4);
		game_RwIm2DRenderPrimitive(4, verts, 4);
		game_RwIm2DRenderPrimitive(4, verts, 4);*/
	}
}
#endif

void objbase_frame_update()
{
	objbase_do_hover();
#if 0
	objbase_draw_entity_bound_rect(selected_entity.entity, 0xFFFF00FF);
	objbase_draw_entity_bound_rect(hovered_entity.entity, 0xFFFF00FF);
#endif
}

#define _CScriptThread__getNumberParams 0x464080
#define _CScriptThread__setNumberParams 0x464370
#define _CObject_vtable_CEntity_render 0x534310

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
		if (exclusiveExtraObject.sa_handle == sa_handle) {
			object = &exclusiveExtraObject;
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
/*0453=4,set_object %1d% XYZ_rotation %2d% %3d% %4d%*/
static struct DETOUR detour_0453;
static struct DETOUR detour_render_object;
static struct DETOUR detour_render_centity;
static struct DETOUR detour_render_water;

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
	exclusiveExtraObject.model = 1212;
	objbase_mkobject(&exclusiveExtraObject, &pos);
}

void objbase_init()
{
	DWORD oldvp;

	memset(&selected_entity, 0, sizeof(selected_entity));

	detour_0107.target = (int*) 0x469896;
	detour_0107.new_target = (int) opcode_0107_detour;
	detour_0453.target = (int*) 0x48A355;
	detour_0453.new_target = (int) opcode_0453_detour;
	detour_render_object.target = (int*) 0x59F1EE;
	//detour_render_object.new_target = (int) render_object_detour;
	detour_render_centity.target = (int*) 0x534313;
	detour_render_centity.new_target = (int) render_centity_detour;
	detour_render_water.target = (int*) 0x6EF658;
	detour_render_water.new_target = (int) render_water_detour;
	objbase_install_detour(&detour_0107);
	objbase_install_detour(&detour_0453);
	//objbase_install_detour(&detour_render_object);
	objbase_install_detour(&detour_render_centity);
	VirtualProtect((void*) 0x534312, 1, PAGE_EXECUTE_READWRITE, &oldvp);
	*((char*) 0x534312) = 0xE8;
	objbase_install_detour(&detour_render_water);

}

void objbase_dispose()
{
	objbase_uninstall_detour(&detour_0107);
	objbase_uninstall_detour(&detour_0453);
	//objbase_uninstall_detour(&detour_render_object);
	objbase_uninstall_detour(&detour_render_centity);
	*((char*) 0x534312) = 0x51;
	*((int*) 0x534313) = 0x8BF18B56;
	objbase_uninstall_detour(&detour_render_water);

	objbase_color_new_entity(&selected_entity, NULL, 0);
	objbase_color_new_entity(&hovered_entity, NULL, 0);
}
