/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "game.h"

unsigned int *fontColorABGR = (unsigned int*) 0xC71A97;
unsigned char *enableHudByOpcode = (unsigned char*) 0xA444A0;
struct CMouseState *activeMouseState = (struct CMouseState*) 0xB73404;
struct CMouseState *currentMouseState = (struct CMouseState*) 0xB73418;
struct CMouseState *prevMouseState = (struct CMouseState*) 0xB7342C;

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

__declspec(naked) int game_TextSetAlign(char align)
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