/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"

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

__declspec(naked) void game_CameraRestore()
{
	_asm mov ecx, camera
	_asm mov eax, 0x50B930
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