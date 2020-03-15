/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "objbase.h"
#include "objects.h"
#include "sockets.h"
#include "../shared/serverlink.h"
#include <string.h>
#include <windows.h>

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

#define _CScriptThread__getNumberParams 0x464080
#define _CScriptThread__setNumberParams 0x464370
#define _opcodeParameters 0xA43C78

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
	detour_0107.target = (int*) 0x469896;
	detour_0107.new_target = (int) opcode_0107_detour;
	detour_0453.target = (int*) 0x48A355;
	detour_0453.new_target = (int) opcode_0453_detour;
	objbase_install_detour(&detour_0107);
	objbase_install_detour(&detour_0453);
}

void objbase_dispose()
{
	objbase_uninstall_detour(&detour_0107);
	objbase_uninstall_detour(&detour_0453);
}
