/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "objbase.h"
#include "objects.h"
#include "sockets.h"
#include "../shared/serverlink.h"
#include <string.h>
#include <windows.h>

static struct {
	void *entity;
	int lit_flag_mask[2]; /*one for entity + one for LOD*/
	int lod_lit_flag_mask;
} selected_entity;

/**
TODO: optimize this
*/
struct OBJECT *objects_find_by_sa_handle(int sa_handle)
{
	int i;
	struct OBJECT *objects;

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
Since x-coord of creation is a pointer to the object handle, the position needs
to be reset.
*/
static
void objects_set_position_after_creation(struct OBJECT *object)
{
	struct MSG_NC nc;
	struct RwV3D pos;

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
}

struct OBJECT *objbase_mkobject(
	struct OBJECTLAYER *layer,
	int model,
	struct RwV3D *pos)
{
	struct OBJECT *object;
	struct MSG_NC nc;

	if (layer->numobjects >= MAX_OBJECTS) {
		return NULL;
	}

	object = layer->objects + layer->numobjects++;
	layer->needupdate = 1;

	object->model = model;
	object->temp_x = pos->x;
	object->justcreated = 1;
	object->samp_objectid = -1;

	nc._parent.id = MAPEDIT_MSG_NATIVECALL;
	nc._parent.data = 0; /*TODO*/
	nc.nc = NC_CreateObject;
	nc.params.asint[1] = 3279;
	nc.params.asint[2] = (int) object;
	nc.params.asflt[3] = pos->y;
	nc.params.asflt[4] = pos->z;
	nc.params.asflt[5] = 0.0f;
	nc.params.asflt[6] = 0.0f;
	nc.params.asflt[7] = 0.0f;
	nc.params.asflt[8] = 500.0f;
	sockets_send(&nc, sizeof(nc));
	return object;
}

void objbase_server_object_created(struct MSG_OBJECT_CREATED *msg)
{
	struct OBJECT *object;

	object = msg->object;
	object->samp_objectid = msg->samp_objectid;
	if (!object->justcreated) {
		/*race with objects_object_rotation_changed*/
		objects_set_position_after_creation(object);
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
__declspec(naked) void render_centity_detour_restore_material_color()
{
	_asm {
		push 0 /*color*/
		push ecx /*entity*/
		call set_entity_color_agbr
		add esp, 0x8
		ret
	}
}

static
__declspec(naked) void render_centity_detour()
{
	_asm {
		cmp ecx, selected_entity.entity
		jne notthisone
		push 0xFFFF00FF /*color*/
		push ecx /*entity*/
		call set_entity_color_agbr
		add esp, 0x8
		push render_centity_detour_restore_material_color
notthisone:
		mov esi, ecx
		mov eax, [esi+0x18]
		ret
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

void objbase_select_entity(void *entity)
{
	if (selected_entity.entity != NULL) {
		objbase_color_entity(selected_entity.entity, 0,
			selected_entity.lit_flag_mask);
	}

	selected_entity.entity = entity;
	if (entity != NULL) {
		objbase_color_entity(entity, 0xFF0000FF,
			selected_entity.lit_flag_mask);
	}
}

#define _CScriptThread__getNumberParams 0x464080
#define _CScriptThread__setNumberParams 0x464370
#define _opcodeParameters 0xA43C78
#define _CObject_vtable_CEntity_render 0x534310

static
void objbase_object_created(object, sa_object, sa_handle)
	struct OBJECT *object;
	void *sa_object;
	int sa_handle;
{
	/*TODO: this can cause crashes when reconnecting to the server*/
	object->sa_object = sa_object;
	object->sa_handle = sa_handle;
}

void objbase_object_rotation_changed(int sa_handle)
{
	struct OBJECT *object;

	object = objects_find_by_sa_handle(sa_handle);
	if (object != NULL) {
		if (object->justcreated) {
			object->justcreated = 0;
			/*race with objects_server_object_created*/
			if (object->samp_objectid != -1) {
				objects_set_position_after_creation(object);
			}
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
	objbase_install_detour(&detour_0107);
	objbase_install_detour(&detour_0453);
	//objbase_install_detour(&detour_render_object);
	//objbase_install_detour(&detour_render_centity);
	//VirtualProtect((void*) 0x534310, 1, PAGE_EXECUTE_READWRITE, &oldvp);
	//*((char*) 0x534312) = 0xE8;

}

void objbase_dispose()
{
	objbase_uninstall_detour(&detour_0107);
	objbase_uninstall_detour(&detour_0453);
	//objbase_uninstall_detour(&detour_render_object);
	//objbase_uninstall_detour(&detour_render_centity);
	//*((char*) 0x534312) = 0x51;
	//*((int*) 0x534313) = 0x8BF18B56;

	objbase_select_entity(NULL);
}
