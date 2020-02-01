/* vim: set filetype=c ts=8 noexpandtab: */

#include "../shared/sizecheck.h"
#include "../shared/clientlink.h"
#include "msgbox.h"
#include "objects.h"
#include "project.h"
#include "sockets.h"
#include "samp.h"
#include "settings.h"
#include "ui.h"
#include "windows.h"
#include <windows.h>

struct CLIENTLINK *linkdata;
int reload_requested = 0;
int in_samp;

static
__declspec(naked) void clientloop()
{
	sockets_recv();
	ui_render();
	_asm {
		pop eax
		push reload_requested
		jmp eax
	}
}

static
__declspec(naked) void detour_detour()
{
	_asm {
		pushad
		call clientloop
		pop eax
		test eax, eax
		popad
		mov eax, 0x99887766 /*dummy 5 bytes, to be original code*/
		jz noreload
		/*disguise next jmp to linkdata->proc_loader_reload_client
		as a call with as return address the address were this was
		hooked from*/
		push 0x99887766
		mov eax, 0x99887766 /*dummy 5 bytes, to be jmp to reload*/
noreload:
		mov eax, 0x99887766 /*dummy 5 bytes, to be return jmp*/
	}
}

#define DETOUR_OPCODE 0x58FBC4
#define DETOUR_PARAM 0x58FBC5
#define DETOUR_EIP 0x58FBC9
#define DETOUR_SIZE 0x20
#define DETOUR &detour_detour
#define DT(X) ((int) DETOUR + X)

static unsigned char detour_opcode;
static unsigned int detour_param;

static
void detour()
{
	DWORD oldvp;
	unsigned char *passthru_op = (unsigned char*) DT(0xA);
	unsigned int *passthru_pa = (unsigned int*) DT(0xB);
	int reload_addr = (int) linkdata->proc_loader_reload_client;

	/*fetch the original code*/
	detour_opcode = *((unsigned char*) DETOUR_OPCODE);
	detour_param = *((unsigned int*) DETOUR_PARAM);
	/*install the detour*/
	*((unsigned char*) DETOUR_OPCODE) = 0xE9;
	*((unsigned int*) DETOUR_PARAM) = (int) DETOUR - DETOUR_EIP;

	VirtualProtect(DETOUR, DETOUR_SIZE, PAGE_EXECUTE_READWRITE, &oldvp);
	/*write the thing that was overwritten to the detour jmp*/
	*passthru_op = detour_opcode;
	if (detour_opcode == 0xA0 && detour_param == 0xBA6769) {
		/*original code*/
		*passthru_pa = detour_param;
	} else if (detour_opcode == 0xE9) {
		/*already a jump, perhaps plhud? :)*/
		*passthru_pa = (detour_param + DETOUR_EIP) - DT(0xF);
		/*reloading won't work in this case,
		but production use should be ok*/
	}
	/*return address for fake call to reload_client*/
	*((unsigned int*) DT(0x12)) = DETOUR_EIP;
	/*call to reload_client (actually a jmp)*/
	*((unsigned char*) DT(0x16)) = 0xE9;
	*((unsigned int*) DT(0x17)) = reload_addr - DT(0x1B);
	/*jmp back to where the detour was set*/
	*((unsigned char*) DT(0x1B)) = 0xE9;
	*((unsigned int*) DT(0x1C)) = DETOUR_EIP - DT(DETOUR_SIZE);
	VirtualProtect(DETOUR, DETOUR_SIZE, oldvp, &oldvp);
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
	objects_dispose();
	settings_dispose();
	sockets_dispose();

	ui_dispose();
	wnd_dispose();
	prj_dispose();
	msg_dispose();

	undetour();
	samp_dispose();
}

/**
Separate init function, because you shouldn't do too much in the dll's entrypt.

See https://docs.microsoft.com/en-us/windows/win32/dlls/dllmain
*/
static
void client_init()
{
	CreateDirectoryA("samp-mapedit", NULL);
	samp_init();
	detour();

	ui_init();
	/*order of init affects layout of main menu*/
	settings_init();
	wnd_init();
	prj_init();
	msg_init();

	sockets_init();
	objects_init();
}

__declspec(dllexport) void __cdecl MapEditMain(struct CLIENTLINK *data)
{
	linkdata = data;
	data->proc_client_finalize = client_finalize;
	data->proc_client_clientmain = client_init;
}
