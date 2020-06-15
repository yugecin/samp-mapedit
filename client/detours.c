/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "detours.h"
#include "entity.h"
#include "game.h"
#include "objects.h"
#include "objectsui.h"
#include "objbrowser.h"
#include "removedbuildings.h"
#include "removebuildingeditor.h"
#include "vehicles.h"
#include <windows.h>

struct CEntity *lastCarSpawned;
int showWater;

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
		mov eax, showWater
		test eax, eax
		jz norender
		mov eax, exclusiveEntity
		test eax, eax
		jz continuerender
norender:
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

#define _CScriptThread__getNumberParams 0x464080
#define _CScriptThread__setNumberParams 0x464370
#define _CWorld__remove 0x563280

static
__declspec(naked) void cworld_remove_detour()
{
	_asm {
		push [esp+0x8]
		call objui_on_world_entity_removed
		call rb_on_entity_removed_from_world
		call vehicles_on_entity_removed_from_world
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
		call objects_client_object_created
		add esp, 0xC
		popad
		mov eax, _CScriptThread__setNumberParams
		jmp eax
	}
}

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
		call objects_object_rotation_changed
		add esp, 0x4
		popad
		mov eax, _CScriptThread__getNumberParams
		jmp eax
	}
}

static
__declspec(naked) void spawn_car_detour()
{
	_asm {
		mov lastCarSpawned, esi
		mov eax, 0x56E210 /*_getPlayerPed*/
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
static struct DETOUR detour_spawn_car;

void install_detour(struct DETOUR *detour)
{
	DWORD oldvp;

	VirtualProtect(detour->target, 4, PAGE_EXECUTE_READWRITE, &oldvp);
	detour->old_target = *detour->target;
	*detour->target = (int) detour->new_target - ((int) detour->target + 4);
}

void uninstall_detour(struct DETOUR *detour)
{
	*detour->target = detour->old_target;
}

static char CTheScripts__ClearSpaceForMissionEntity_op;

void detours_install()
{
	DWORD oldvp;

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
	detour_spawn_car.target = (int*) 0x43A359;
	detour_spawn_car.new_target = (int) spawn_car_detour;
	install_detour(&detour_0107);
	install_detour(&detour_0108);
	install_detour(&detour_0453);
	//install_detour(&detour_render_object);
	install_detour(&detour_render_centity);
	VirtualProtect((void*) 0x534312, 1, PAGE_EXECUTE_READWRITE, &oldvp);
	*((char*) 0x534312) = 0xE8;
	install_detour(&detour_render_water);
	install_detour(&detour_cworld_remove);
	VirtualProtect((void*) 0x563280, 1, PAGE_EXECUTE_READWRITE, &oldvp);
	*((char*) 0x563280) = 0xE8;
	install_detour(&detour_cworld_add);
	VirtualProtect((void*) 0x563220, 1, PAGE_EXECUTE_READWRITE, &oldvp);
	*((char*) 0x563220) = 0xE8;
	install_detour(&detour_spawn_car);
	VirtualProtect((void*) 0x486B00, 1, PAGE_EXECUTE_READWRITE, &oldvp);
	CTheScripts__ClearSpaceForMissionEntity_op = *((char*) 0x486B00);
	*((char*) 0x486B00) = 0xC3;

	showWater = 1;
}

void detours_uninstall()
{
	uninstall_detour(&detour_0107);
	uninstall_detour(&detour_0108);
	uninstall_detour(&detour_0453);
	//uninstall_detour(&detour_render_object);
	uninstall_detour(&detour_render_centity);
	*((char*) 0x534312) = 0x51;
	*((int*) 0x534313) = 0x8BF18B56;
	uninstall_detour(&detour_render_water);
	uninstall_detour(&detour_cworld_remove);
	*((char*) 0x563280) = 0x56;
	uninstall_detour(&detour_cworld_add);
	*((char*) 0x563220) = 0x56;
	uninstall_detour(&detour_spawn_car);
	*((char*) 0x486B00) = CTheScripts__ClearSpaceForMissionEntity_op;
}
