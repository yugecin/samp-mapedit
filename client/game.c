/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"
#include "ui.h"

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

__declspec(naked) void game_init()
{
	_asm {
		mov eax, script_PLAYER_ACTOR
		mov eax, [eax]
		push eax /*pedHandle*/
		mov ecx, 0xB74490 /*void *PedPool*/
		mov ecx, [ecx]
		mov eax, 0x404910 /*CPed *CPool_CPed::getStructByHandle*/
		call eax /*__thiscall*/
		mov player, eax
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

/**
opcode 00A0 @0x4677E2

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
		fmul ds:0x859878 /*_radToDeg*/
		fstp [eax]
		mov eax, ecx
		mov ecx, [eax+0x14] /*CPlaceable.m_pCoords*/
		test ecx, ecx
		jz no_explicit_coords
		lea eax, [ecx+0x30-0x4] /*what's at +30h?*/
no_explicit_coords:
		lea eax, [eax+0x4] /*CPlaceable.placement*/
		mov ecx, [esp+0xC] /*pos*/
		mov [ecx], eax
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
		/*push angle*/
		/*mov ecx, CPlaceable*/
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

void game_WorldToScreen(struct RwV3D *out, struct RwV3D *in)
{
	game_TransformPoint(out, &camera->cameraViewMatrix, in);
	out->x *= fresx / out->z;
	out->y *= fresy / out->z;
}
