/* vim: set filetype=c ts=8 noexpandtab: */

#include "client.h"
#include <windows.h>

static unload_client_t proc_unload;

void __cdecl renderloop()
{
	float xratio = (float)(*(int*)(0xC17044)) / 640.0f;
	float yratio = (float)(*(int*)(0xC17048)) / 480.0f;

	char *text = "hey text";
	float x = 80.0f * xratio;
	float y = 180.0f * yratio;
	int font = 1;
	char proportional = 1;
	int align = 0;
	int r = 255;
	int g = 20;
	int b = 20;
	int a = 255;
	float xsize = 1.0f * xratio;
	float ysize = 1.0f * yratio;

	/**((int*) 0xBAB230) = *((int*) (0xB73418 + 0xC));*/

	((void (__cdecl *)(int,int))0x7195C0)(0, 0);
	((void (__cdecl *)(int))0x7194F0)(0);
	((void (__cdecl *)(char align))0x719610)(align);
	((void (__cdecl *)(float xsize, float ysize))0x719380)(xsize, ysize);
	((void (__cdecl *)(char proportional))0x7195B0)(proportional);
	((void (__cdecl *)(char font))0x719490)(font);
	((void (__cdecl *)(int outline))0x719590)(1);
	((void (__cdecl *)(int abgr))0x719510)(0xFF000000);
	((void (__cdecl *)(int abgr))0x719430)(0xFF0050CC);
	((void (__cdecl *)(float, float, const char*))0x71A700)(x, y, text);
}

__declspec(naked) void detour_detour()
{
	_asm {
		pushad
		call renderloop
		popad
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
	}
}

#define DETOUR_OPCODE 0x58FBC4
#define DETOUR_PARAM 0x58FBC5
#define DETOUR_EIP 0x58FBC9
#define DT(X) ((int) &detour_detour + X)

static unsigned char detour_opcode;
static unsigned int detour_param;

static void detour()
{
	DWORD oldvp;
	unsigned char *passthru_op = (unsigned char*) DT(7);
	unsigned int *passthru_pa = (unsigned int*) DT(8);

	detour_opcode = *((unsigned char*) DETOUR_OPCODE);
	detour_param = *((unsigned int*) DETOUR_PARAM);

	VirtualProtect(&detour_detour, 17, PAGE_EXECUTE_READWRITE, &oldvp);
	*passthru_op = detour_opcode;
	if (detour_opcode == 0xA0 && detour_param == 0xBA6769) {
		/*original code*/
		*passthru_pa = detour_param;
	} else if (detour_opcode == 0xE9) {
		/*already a jump, perhaps plhud? :)*/
		*passthru_pa =
			(detour_param + 0x58FBC9) -
			(unsigned int) passthru_pa + 4;
	}
	*((unsigned char*) DT(12)) = 0xE9;
	*((unsigned int*) DT(13)) = DETOUR_EIP - (int) &detour_detour - 17;
	VirtualProtect(&detour_detour, 17, oldvp, &oldvp);

	*((unsigned char*) DETOUR_OPCODE) = 0xE9;
	*((unsigned int*) DETOUR_PARAM) = (int) &detour_detour - DETOUR_EIP;
}

static void undetour()
{
	*((unsigned char*) DETOUR_OPCODE) = detour_opcode;
	*((unsigned int*) DETOUR_PARAM) = detour_param;
}

/**
Called from the loader, do not call directly (use proc_unload)
*/
void client_finalize()
{
	undetour();
}

__declspec(dllexport)
client_finalize_t __cdecl MapEditMain(unload_client_t unloadfun)
{
	proc_unload = unloadfun;
	detour();
	return &client_finalize;
}
