/* vim: set filetype=c ts=8 noexpandtab: */

#include "client.h"
#include "ui.h"
#include <windows.h>

static struct CLIENTLINK *linkdata;

static
void __cdecl renderloop()
{
	ui_render();
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

static
void detour()
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
		*passthru_pa = (detour_param + DETOUR_EIP) - DT(12);
	}
	*((unsigned char*) DT(12)) = 0xE9;
	*((unsigned int*) DT(13)) = DETOUR_EIP - DT(17);
	VirtualProtect(&detour_detour, 17, oldvp, &oldvp);

	*((unsigned char*) DETOUR_OPCODE) = 0xE9;
	*((unsigned int*) DETOUR_PARAM) = (int) &detour_detour - DETOUR_EIP;
}

static
void undetour()
{
	*((unsigned char*) DETOUR_OPCODE) = detour_opcode;
	*((unsigned int*) DETOUR_PARAM) = detour_param;
}

/**
Called from the loader, do not call directly (use proc_unload)
*/
static
void client_finalize()
{
	ui_dispose();
	undetour();
}

/**
Separate init function, because you shouldn't do too much in the dll's entrypt.

See https://docs.microsoft.com/en-us/windows/win32/dlls/dllmain
*/
static
void client_init()
{
	detour();
	ui_init();
}

__declspec(dllexport) void __cdecl MapEditMain(struct CLIENTLINK *data)
{
	linkdata = data;
	data->proc_client_finalize = client_finalize;
	data->proc_client_clientmain = client_init;
}
