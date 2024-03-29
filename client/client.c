/* vim: set filetype=c ts=8 noexpandtab: */

#include "bulkeditui.h"
#include "common.h"
#include "detours.h"
#include "entity.h"
#include "game.h"
#include "gangzone.h"
#include "textdraws.h"
#include "ide.h"
#include "massmove.h"
#include "msgbox.h"
#include "objects.h"
#include "objectsui.h"
#include "objbrowser.h"
#include "objectlistui.h"
#include "objectseditor.h"
#include "objecttextures.h"
#include "persistence.h"
#include "player.h"
#include "project.h"
#include "racecp.h"
#include "racecpeditor.h"
#include "racecpui.h"
#include "removedbuildings.h"
#include "removebuildingeditor.h"
#include "removedbuildingsui.h"
#include "sockets.h"
#include "samp.h"
#include "settings.h"
#include "timeweather.h"
#include "ui.h"
#include "vehicles.h"
#include "vehicleseditor.h"
#include "vehicleselector.h"
#include "vehiclesui.h"
#include "../shared/serverlink.h"
#include "../shared/clientlink.h"
#include <windows.h>

struct CLIENTLINK *linkdata;
int reload_requested = 0;
int in_samp;

static
__declspec(naked) void clientloop()
{
	sockets_recv();
	objects_update();
	ui_render();
	objects_show_creation_progress();
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
		sprintf(debugstring, "mods detected (plhud?), reloading might not work");
		ui_push_debug_string();
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
	TRACE("undetour");
	*((unsigned char*) DETOUR_OPCODE) = detour_opcode;
	*((unsigned int*) DETOUR_PARAM) = detour_param;
}

static WNDPROC hOldProc;

static
LRESULT APIENTRY NewWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_MOUSEWHEEL) {
		ui_on_mousewheel(GET_WHEEL_DELTA_WPARAM(wParam) / 120);
	}
	if (uMsg == WM_KEYDOWN && ui_handle_keydown((int) wParam)) {
		return 0;
	}
	/*somewhere WM_KEYDOWN is already translated to WM_CHAR?*/
	if (uMsg == WM_CHAR && ui_handle_char((char) wParam)) {
		return 0;
	}
	return CallWindowProc(hOldProc, hWnd, uMsg, wParam, lParam);
}

/**
Called from the loader, do not call directly (use proc_unload)
*/
static
void client_finalize()
{
	TRACE("client_finalize");
	SetWindowLong(*((HWND*) gameHwnd), GWL_WNDPROC, (LONG) hOldProc);

	massmove_dispose();
	racecpeditor_dispose();
	racecpui_dispose();
	vehiclesui_dispose();
	objbrowser_dispose();
	objedit_dispose();
	objlistui_dispose();
	objui_dispose();
	objects_dispose();
	rbui_dispose();
	rbe_dispose();
	settings_dispose();
	sockets_dispose();
	vehsel_dispose();
	vehedit_dispose();
	bulkeditui_dispose();
	objecttextures_dispose();

	ui_dispose();
	timeweather_dispose();
	prj_dispose();
	msg_dispose();
	rb_undo_all();

	vehicles_destroy();

	undetour();
	gangzone_dispose();
	textdraws_dispose();
	samp_dispose();
	detours_uninstall();
	common_dispose();
}

/**
Separate init function, because you shouldn't do too much in the dll's entrypt.

See https://docs.microsoft.com/en-us/windows/win32/dlls/dllmain
*/
static
void client_init()
{
	struct MSG_NC nc;
	HWND ghwnd = *((HWND*) gameHwnd);

	common_init();
	TRACE("client_init");
	CreateDirectoryA("samp-mapedit", NULL);

	game_init();
	samp_init();
	entity_init();
	detours_install();
	detour();

	ui_init();
	ide_load();
	racecp_resetall();

	/*order of init affects layout of main menu and context menu*/
	settings_init();
	player_init();
	prj_init();
	timeweather_init();
	msg_init();

	vehedit_init();
	vehsel_init();
	sockets_init();
	objects_init();
	objui_init();
	objlistui_init();
	objbrowser_init();
	objedit_init();
	bulkeditui_init();
	gangzone_init();
	rbe_init();
	rbui_init();
	vehiclesui_init();
	racecpui_init();
	racecpeditor_init();
	textdraws_init();
	massmove_init();
	objecttextures_init();

	prj_do_show_window();

	nc._parent.id = MAPEDIT_MSG_NATIVECALL;
	nc.nc = NC_DestroyVehicle;
	nc.params.asint[1] = 1;
	sockets_send(&nc, sizeof(nc));
	nc._parent.id = MAPEDIT_MSG_NATIVECALL;
	nc.nc = NC_SetWorldTime;
	nc.params.asint[1] = 13;
	sockets_send(&nc, sizeof(nc));

	persistence_init();
	prj_open_persistent_state();
	objects_open_persistent_state();
	ui_open_persistent_state();

	hOldProc = (WNDPROC) GetWindowLong(ghwnd, GWL_WNDPROC);
	SetWindowLong(ghwnd, GWL_WNDPROC, (LONG) NewWndProc);
}

__declspec(dllexport) void __cdecl MapEditMain(struct CLIENTLINK *data)
{
	linkdata = data;
	data->proc_client_finalize = client_finalize;
	data->proc_client_clientmain = client_init;
}
