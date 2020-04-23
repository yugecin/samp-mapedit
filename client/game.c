/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ui.h"
#include <windows.h>

struct CRope *ropes = (struct CRope*) 0xB768B8;
struct CSector *worldSectors;
struct CRepeatSector *worldRepeatSectors;
unsigned int *fontColorABGR = (unsigned int*) 0xC71A97;
unsigned char *enableHudByOpcode = (unsigned char*) 0xA444A0;
struct CMouseState *activeMouseState = (struct CMouseState*) 0xB73404;
struct CMouseState *currentMouseState = (struct CMouseState*) 0xB73418;
struct CMouseState *prevMouseState = (struct CMouseState*) 0xB7342C;
struct CKeyState *activeKeyState = (struct CKeyState*) 0xB72CB0;
struct CKeyState *currentKeyState = (struct CKeyState*) 0xB72F20;
struct CKeyState *prevKeyState = (struct CKeyState*) 0xB73190;
float *gamespeed = (float*) 0xB7CB64;
struct CCamera *camera = (struct CCamera*) (0xB6F028);
float *hudScaleX = (float*) 0x859520;
float *hudScaleY = (float*) 0x859524;
struct CRaceCheckpoint *racecheckpoints = (struct CRaceCheckpoint*) 0xC7F158;
int *timeInGame = (int*) 0xB7CB84;
int *script_PLAYER_CHAR = (int*) 0xA49968; /* $2=PLAYER_CHAR */
int *script_PLAYER_ACTOR = (int*) 0xA4996C; /* $3=PLAYER_ACTOR */
struct CPed *player;
void *gameHwnd = (void*) 0xC97C1C;
void *_opcode0815ret = (void*) 0x473BEE;
unsigned char *logBuffer = (char*) 0xBAB278;

void unprotect_stuff()
{
	DWORD oldvp;

	VirtualProtect(_opcode0815ret, 1, PAGE_EXECUTE_READWRITE, &oldvp);
}

__declspec(naked) void game_init()
{
	_asm {
		call unprotect_stuff
		mov eax, script_PLAYER_ACTOR
		mov eax, [eax]
		push eax /*pedHandle*/
		mov ecx, 0xB74490 /*void *PedPool*/
		mov ecx, [ecx]
		mov eax, 0x404910 /*CPed *CPool_CPed::getStructByHandle*/
		call eax /*__thiscall*/
		mov player, eax

		/*on laptop executable: 0x408259
		on desktop executable: 0x408261*/
		mov eax, 0x408258
		cmp byte ptr [eax], 0xB9
		mov eax, 0x408259
		jz have_mov
		mov eax, 0x408261
have_mov:
		mov eax, [eax]
		mov worldSectors, eax

		mov eax, 0x5634A5
		mov eax, [eax]
		mov worldRepeatSectors, eax

		ret
	}
}

__declspec(naked) void game_CameraRestore()
{
	_asm mov ecx, camera
	_asm mov eax, 0x50B930
	_asm jmp eax
}

__declspec(naked) void game_CameraRestoreWithJumpCut()
{
	_asm mov ecx, camera
	_asm mov eax, 0x50BAB0
	_asm jmp eax
}

/**
@param unk when 2, seems to restore the camera?
*/
__declspec(naked) void __stdcall game_CameraSetOnPoint(point, cutmode, unk)
	struct RwV3D *point;
	enum eCameraCutMode cutmode;
	int unk;
{
	_asm mov ecx, camera
	_asm mov eax, 0x50C8B0
	_asm jmp eax
}

void game_DrawRect(float x, float y, float w, float h, int argb)
{
	struct IM2DVERTEX verts[] = {
		{x, y + h, 0, 0x40555556, argb, 1.0f, 0.0f},
		{x + w, y + h, 0, 0x40555556, argb, 1.0f, 0.0f},
		{x, y, 0, 0x40555556, argb, 1.0f, 0.0f},
		{x + w, y, 0, 0x40555556, argb, 1.0f, 0.0f},
	};
	game_RwIm2DPrepareRender();
	game_RwIm2DRenderPrimitive(4, verts, 4);
}

__declspec(naked)
struct CColModel *game_EntityGetColModel(void *entity)
{
	_asm {
		mov ecx, [esp+0x4]
		mov eax, 0x535300
		jmp eax
	}
}

__declspec(naked)
float game_EntityGetDistanceFromCentreOfMassToBaseOfModel(void *entity)
{
	_asm {
		mov ecx, [esp+0x4]
		mov eax, 0x536BE0
		jmp eax
	}
}

__declspec(naked) void game_EntitySetAlpha(void *entity, unsigned char alpha)
{
	_asm {
		mov ecx, [esp+0x4]
		push [esp+0x8]
		mov eax, 0x5332C0
		call eax
		ret
	}
}

void game_FreezePlayer(char flag)
{
	/*see CPlayer__makePlayerSafe 0x56E870
	used by opcode 01B4: set_player $PLAYER_CHAR can_move 1
	at 0x56E89D and 0x56E9C8*/
	if (flag) {
		*((unsigned char*) CPAD + 0x10E) |= 0x20;
	} else {
		*((unsigned char*) CPAD + 0x10E) &= 0xDF;
	}
}

__declspec(naked) int __stdcall game_InputWasKeyPressed(short keycode)
{
	_asm mov ecx, 0xB70198 /*inputEvents*/
	_asm mov eax, 0x52E450
	_asm jmp eax
}

__declspec(naked) int game_Intersect(
	struct RwV3D *origin,
	struct RwV3D *direction,
	struct CColPoint *collidedColpoint,
	void **collidedEntity,
	int buildings,
	int vehicles,
	int peds,
	int objects,
	int dummies,
	int doSeeThroughCheck,
	int doCameraIgnoreCheck,
	int doShootThroughCheck)
{
	_asm mov eax, 0x56BA00
	_asm jmp eax
}

int game_IntersectBuildingObject(
	struct RwV3D *origin,
	struct RwV3D *direction,
	struct CColPoint *colpoint,
	void **collidedEntity)
{
	return game_Intersect(
		origin, direction, colpoint, collidedEntity,
		1, 0, 0, 1, 0, 0, 0, 0);
}

/**
see opcode 01BB @0x47D567
*/
__declspec(naked) void game_ObjectGetPos(void *object, struct RwV3D *pos)
{
	_asm {
		push ecx
		push esi
		mov eax, [esp+0xC] /*object*/
		mov esi, [esp+0x10] /*pos*/
		mov ecx, [eax+0x14] /*CPlaceable.m_pCoords*/
		test ecx, ecx
		jz no_explicit_coords
		lea eax, [ecx+0x30-0x4] /*CMatrix.pos*/
no_explicit_coords:
		add eax, 4
		mov ecx, [eax]
		mov [esi], ecx
		mov ecx, [eax+0x4]
		mov [esi+0x4], ecx
		mov ecx, [eax+0x8]
		mov [esi+0x8], ecx
		pop esi
		pop ecx
		ret
	}
}

/**
using opcode 0815 @0x473A01
*/
__declspec(naked) void game_ObjectSetPos(void *object, struct RwV3D *pos)
{
	_asm {
		push esi
		mov eax, _opcode0815ret
		push [eax]
		mov byte ptr [eax], 0xC3

		mov eax, esp
		pushad

		/*esi: object*/
		/*ecx: x*/
		/*edx: y*/
		/*eax: z (_opcodeParameters+0xC)*/

		push [eax+0xC] /*object*/
		mov eax, [eax+0x10] /*pos*/
		mov ecx, [eax] /*x*/
		mov edx, [eax+0x4] /*y*/
		mov esi, [eax+0x8] /*z*/
		mov eax, _opcodeParameters
		mov [eax+0xC], esi /*z*/
		pop esi /*object*/
		mov eax, 0x473A29
		sub esp, 0x1A4 /*see 0x474386*/
		call eax
		add esp, 0x1A4

		popad

		pop eax
		mov esi, _opcode0815ret
		mov byte ptr [esi], al
		pop esi
		ret
	}
}

/**
see opcode 0453 @0x48A350
*/
__declspec(naked) void game_ObjectSetRotRad(void *object, struct RwV3D *rot)
{
	_asm {
		push [esp+0x4] /*object*/
		mov eax, 0x563280 /*__cdecl CWorld::Remove*/
		call eax
		pop eax

		mov ecx, [esp+0x4] /*this*/
		mov eax, [esp+0x8]
		push [eax+0x8] /*z*/
		push [eax+0x4] /*y*/
		push [eax] /*x*/
		mov eax, 0x439A80 /*__thiscall CPlaceable::SetOrientation*/
		call eax

		mov ecx, [esp+0x4] /*this*/
		mov eax, 0x446F90 /*__cdecl CMatrix::updateRW*/
		call eax

		mov ecx, [esp+0x4] /*this*/
		mov eax, 0x532B00 /*__thiscall CEntity::updateRwFrame()*/
		call eax

		push [esp+0x4] /*object*/
		mov eax, 0x563220 /*__cdecl CWorld::Add*/
		call eax
		pop eax

		ret
	}
}

/**
opcode 00A0 @0x4677E2

2 is subtracted from the z value, because setPos sets the player slightly above

TODO: test this through?
*/
__declspec(naked) void game_PedGetPos(ped, pos, rot)
	struct CPed *ped;
	struct RwV3D **pos;
	float *rot;
{
	_asm {
		push ecx

		mov eax, [esp+0x8] /*ped*/
		mov ecx, [eax+0x46C] /*CPed.PedFlags.Flag2 (invehicle)*/
		test ch, 1
		mov ecx, eax
		jnz gotplaceable
		mov ecx, [eax+0x58C] /*CPed.pVehicle*/
		test ecx, ecx
		jnz gotplaceable
		mov ecx, eax
gotplaceable:
		mov eax, 0x441DB0 /*CPlaceable__getRotation*/
		call eax
		mov eax, [esp+0x10] /*rot*/
		test eax, eax
		jz skiprotation
		fmul ds:0x859878 /*_radToDeg*/
		fstp [eax]
skiprotation:
		mov eax, ecx
		mov ecx, [eax+0x14] /*CPlaceable.m_pCoords*/
		test ecx, ecx
		jz no_explicit_coords
		lea eax, [ecx+0x30-0x4] /*CMatrix.pos*/
no_explicit_coords:
		lea eax, [eax+0x4] /*CPlaceable.placement*/
		mov ecx, [esp+0xC] /*pos*/
		test ecx, ecx
		jz skipposition
		push esi
		mov esi, [eax]
		mov [ecx], esi
		mov esi, [eax+0x4]
		mov [ecx+0x4], esi
		fld [eax+0x8]
		fld1
		fsubp ST(1), ST(0)
		fstp [ecx+0x8]
		pop esi
skipposition:
		pop ecx
		ret
	}
}

__declspec(naked) void game_PedSetPos(struct CPed *ped, struct RwV3D *pos)
{
	_asm {
		push ecx
		push 1 /*?*/
		push 1 /*?*/
		mov eax, [esp+0x14] /*pos*/
		push [eax+0x8] /*z*/
		push [eax+0x4] /*y*/
		push [eax] /*x*/
		push [esp+0x1C] /*ped*/
		/*it acts as a thiscall, but ecx is unused? (though changed)*/
		mov ecx, [esp]
		mov eax, 0x464DC0
		call eax /*CPed__putAtCoords*/
		pop ecx
		ret
	}
}

/*
opcode 0173 @0x47CB44
*/
__declspec(naked) void game_PedSetRot(struct CPed *ped, float rot)
{
	_asm {
		push ecx
		mov ecx, [esp+0x8] /*ped*/
		mov eax, [ecx+0x46C] /*CPed.PedFlags.Flag2 (invehicle)*/
		test ah, 1
		jz notinvehicle
		mov eax, [ecx+0x58C] /*CPed.pVehicle*/
		test eax, eax
		jnz invehicle
notinvehicle:
		fld [esp+0x10] /*rot*/
		fmul ds:0x8595EC /*_degToRad*/
		fst [ecx+0x558] /*CPed.currentHeading*/
		fstp [ecx+0x55C] /*CPed.targetHeading*/
invehicle:
		pop ecx
		ret

/*apparently it works without doing this?*/
#if 0
		push [esp+0xC]
		mov ecx, [esp+0xC]
		mov eax, 0x43E0C0 /*CPlaceable__setHeading*/
		call eax
		mov ecx, [esp+0x8] /*ped*/
		push edx /*used in call below*/
		mov eax, 0x446F90 /*CMatrix__updateRW*/
		call eax
		pop edx
		pop ecx
		ret
#endif
	}
}

__declspec(naked)
void game_RopeRemove(struct CRope *rope)
{
	_asm {
		push ecx
		mov ecx, [esp+0x8] /*rope*/
		mov eax, 0x556780
		call eax
		pop ecx
		ret
	}
}

__declspec(naked) int game_RwIm2DPrepareRender()
{
	_asm {
		//pushad
		mov eax, 0xC97B24
		mov eax, [eax] /*_RwEngineInstance*/
		mov eax, [eax+0x10+0x10] /*.dOpenDevice.fpRenderStateSet*/
		push eax
		push 0x0
		push 0x1
		call eax
		add esp, 0x8
		mov eax, [esp]
		push 0x1
		push 0x7
		call eax
		add esp, 0x8
		mov eax, [esp]
		push 0x1
		push 0xC
		call eax
		add esp, 0xC
		//popad
		ret
	}
}

__declspec(naked) int game_RwIm2DRenderPrimitive(type, verts, numverts)
	int type;
	float *verts;
	int numverts;
{
	_asm {
		mov eax, 0xC97B24
		mov eax, [eax] /*_RwEngineInstance*/
		mov eax, [eax+0x10+0x20] /*.dOpenDevice.fpIm2DRenderPrimitive*/
		jmp eax
	}
}

__declspec(naked) void game_RwMatrixInvert(out, in)
	struct CMatrix *out;
	struct CMatrix *in;
{
	_asm {
		mov eax, 0x4C6E1D /*call    _RwMatrixInvert*/
		mov eax, [eax]
		add eax, 0x4C6E21
		jmp eax
	}
}

void game_ScreenToWorld(struct RwV3D *out, float x, float y, float dist)
{
	struct RwV3D in;
	struct CMatrix invCVM;

	in.x = x * dist / fresx;
	in.y = y * dist / fresy;
	in.z = dist;
	memset(&invCVM, 0, sizeof(struct CMatrix));
	invCVM.pad3 = 1.0f;
	game_RwMatrixInvert(&invCVM, &camera->cameraViewMatrix);
	game_TransformPoint(out, &invCVM, &in);
}

__declspec(naked) float game_TextGetSizeX(char *text, char unk1, char unk2)
{
	_asm mov eax, 0x71A0E0
	_asm jmp eax
}

__declspec(naked) int game_TextGetSizeXY(size, sizex, sizey, text)
	struct Rect *size;
	float sizex;
	float sizey;
	char *text;
{
	_asm mov eax, 0x71A620
	_asm jmp eax
}

__declspec(naked) int game_TextPrintChar(float x, float y, char character)
{
	_asm mov eax, 0x718A10
	_asm jmp eax
}

__declspec(naked) int game_TextPrintString(float x, float y, char *text)
{
	_asm mov eax, 0x71A700
	_asm jmp eax
}

__declspec(naked) int game_TextPrintStringFromBottom(x, y, text)
	float x;
	float y;
	char *text;
{
	_asm mov eax, 0x71A820
	_asm jmp eax
}

__declspec(naked) int game_TextSetAlign(enum eTextdrawAlignment align)
{
	_asm mov eax, 0x719610
	_asm jmp eax
}

__declspec(naked) int game_TextSetAlpha(float alpha)
{
	_asm mov eax, 0x719500
	_asm jmp eax
}

__declspec(naked) int game_TextSetBackground(char flag, char includewrap)
{
	_asm mov eax, 0x7195C0
	_asm jmp eax
}

__declspec(naked) int game_TextSetBackgroundColor(int abgr)
{
	_asm mov eax, 0x7195E0
	_asm jmp eax
}

__declspec(naked) int game_TextSetColor(int abgr)
{
	_asm mov eax, 0x719430
	_asm jmp eax
}

__declspec(naked) int game_TextSetFont(char font)
{
	_asm mov eax, 0x719490
	_asm jmp eax
}

__declspec(naked) int game_TextSetJustify(char flag)
{
	_asm mov eax, 0x719600
	_asm jmp eax
}

__declspec(naked) int game_TextSetLetterSize(float x, float y)
{
	_asm mov eax, 0x719380
	_asm jmp eax
}

__declspec(naked) int game_TextSetMonospace(char monospace)
{
	_asm mov eax, [esp+0x4]
	_asm xor eax, 1
	_asm mov [esp+0x4], eax
	_asm mov eax, 0x7195B0
	_asm jmp eax
}

__declspec(naked) int game_TextSetOutline(char outlintsize)
{
	_asm mov eax, 0x719590
	_asm jmp eax
}

__declspec(naked) int game_TextSetProportional(char proportional)
{
	_asm mov eax, 0x7195B0
	_asm jmp eax
}

__declspec(naked) int game_TextSetShadow(char shadowsize)
{
	_asm mov eax, 0x719570
	_asm jmp eax
}

__declspec(naked) int game_TextSetShadowColor(int abgr)
{
	_asm mov eax, 0x719510
	_asm jmp eax
}

__declspec(naked) int game_TextSetWrapX(float value)
{
	_asm mov eax, 0x7194D0
	_asm jmp eax
}

__declspec(naked) int game_TextSetWrapY(float value)
{
	_asm mov eax, 0x7194E0
	_asm jmp eax
}

__declspec(naked) int game_TransformPoint(out, mat, in)
	struct RwV3D *out;
	struct CMatrix *mat;
	struct RwV3D *in;
{
	_asm mov eax, 0x59C890
	_asm jmp eax
}

__declspec(naked) void game_WorldFindObjectsInRange(
	struct RwV3D *origin,
	float radius,
	int only2d,
	short *numObjects,
	short maxObjects,
	void **entities,
	int buildings,
	int vehicles,
	int peds,
	int objects,
	int dummies)
{
	_asm {
		mov eax, 0x564A20
		jmp eax
	}
}

void game_WorldToScreen(struct RwV3D *out, struct RwV3D *in)
{
	game_TransformPoint(out, &camera->cameraViewMatrix, in);
	out->x *= fresx / out->z;
	out->y *= fresy / out->z;
}
