/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"
#include "samp.h"
#include <windows.h>

static int pCmdWindow;
/**
read-only
*/
static int *chat_bar_visible;
static unsigned char *chat_bar_enable_op;
static unsigned char isR2;
static int *ui_mode;
static int old_ui_mode_value;

int samp_handle;

void samp_init()
{
	struct RPCParameters rpcparams;
	struct RPCDATA_SetWorldBounds rpcsetworldbounds;
	DWORD oldvp;

	samp_handle = (int) GetModuleHandle("samp.dll");
	old_ui_mode_value = -1;

	if (*((int*) (samp_handle + 0xE5B91)) == 0x0034522D/* "-R4\0" */) {
		ui_mode = *((int**) (samp_handle + 0x26E9F8/*chatWindow*/)) + 2;
		pCmdWindow = samp_handle + 0x26E9FC;
		chat_bar_enable_op = (unsigned char*) (samp_handle + 0x69440);
	} else if (*((int*) (samp_handle + 0xE5B8C)) == 0x7B203252/* "R2 {" */) {
		ui_mode = *((int**) (samp_handle + 0x21A0EC/*chatWindow*/)) + 2;
		pCmdWindow = samp_handle + 0x21A0F0;
		chat_bar_enable_op = (unsigned char*) (samp_handle + 0x658B0);
	} else {
		ui_mode = *((int**) (samp_handle + 0x21A0E4/*chatWindow*/)) + 2;
		pCmdWindow = samp_handle + 0x21A0E8;
		chat_bar_enable_op = (unsigned char*) (samp_handle + 0x657E0);
	}

	chat_bar_visible = (int*) (*((int*) pCmdWindow) + 0x14E0);
	VirtualProtect(chat_bar_enable_op, 1, PAGE_EXECUTE_READWRITE, &oldvp);

	rpcparams.dataBitLength = sizeof(struct RPCDATA_SetWorldBounds) * 8;
	rpcparams.data = &rpcsetworldbounds;
	rpcsetworldbounds.max_x = 100000;
	rpcsetworldbounds.min_x = -100000;
	rpcsetworldbounds.max_y = 100000;
	rpcsetworldbounds.min_y = -100000;
	samp_SetWorldBounds(&rpcparams);
}

void samp_dispose()
{
	TRACE("samp_dispose");
	samp_restore_chat_bar();
	samp_restore_ui_f7();
}

static unsigned char chat_bar_overwritten_op = 0;

void samp_break_chat_bar()
{
	if (samp_handle && !chat_bar_overwritten_op) {
		chat_bar_overwritten_op = *chat_bar_enable_op;
		*chat_bar_enable_op = 0xC3; /*ret*/
	}
}

void samp_restore_chat_bar()
{
	if (samp_handle && chat_bar_overwritten_op) {
		*chat_bar_enable_op = chat_bar_overwritten_op;
		chat_bar_overwritten_op = 0;
	}
}

void samp_hide_ui_f7()
{
	old_ui_mode_value = *ui_mode;
	*ui_mode = 0;
}

void samp_restore_ui_f7()
{
	if (old_ui_mode_value != -1) {
		*ui_mode = old_ui_mode_value;
		old_ui_mode_value = -1;
	}
}

/*untested (also needs samp_handle check)
__declspec(naked) int _samp_show_chat_bar()
{
	_asm {
		mov ecx, pCmdWindow
		mov eax, 0x657E0
		add eax, samp_handle
		jmp eax
	}
}

__declspec(naked) int _samp_hide_chat_bar()
{
	_asm {
		mov ecx, pCmdWindow
		mov eax, 0x658E0
		add eax, samp_handle
		jmp eax
	}
}
*/

__declspec(naked)
void __cdecl samp_SendWarningChatMessageF(void *null, char *format, ...)
{
	_asm {
		mov eax, samp_handle
		mov eax, [eax+0x26E9F8] // console
		mov [esp+4], eax
		mov eax, samp_handle
		add eax, 0x680B0
		jmp eax
	}
}

__declspec(naked) void samp_SetPlayerObjectMaterial(struct RPCParameters *rpc_parameters)
{
	_asm {
		mov eax, [samp_handle]
		add eax, 0x1B650
		jmp eax
	}
}

__declspec(naked) void samp_SetWorldBounds(struct RPCParameters *rpc_parameters)
{
	_asm {
		mov eax, [samp_handle]
		add eax, 0x1A3C0
		jmp eax
	}
}

static char materialText[2048];
static char *materialTextPtr = materialText;
static int materialTextLen;
static char materialTextPatch1;
static int materialTextPatch4;
static int materialPatchRet;

static
void mstrcpy(char *dest, char *src)
{
	while (*dest++ = *src++);
}

__declspec(naked) void samp_RPC_SetPlayerObjectMaterial_Text_ReadTextPatch()
{
	_asm {
		lea ecx, [esp+0C80h-0x818]
		mov eax, [materialTextPtr]
		push edx
		mov edx, eax
next:
		mov al, [edx]
		mov [ecx], al
		add ecx, 1
		add edx, 1
		test al, al
		jne next
		pop edx
		mov eax, [materialPatchRet]
		jmp eax
	}
}

void samp_patchObjectMaterialReadText(char *text)
{
	int addr;
	DWORD oldvp;

	materialTextLen = strlen(text) + 1;
	memcpy(materialText, text, materialTextLen);

	addr = samp_handle + 0x1B900;
	materialPatchRet = samp_handle + 0x1B920;
	VirtualProtect((void*) addr, 5, PAGE_EXECUTE_READWRITE, &oldvp);
	materialTextPatch1 = *(char*) addr;
	materialTextPatch4 = *(int*) (addr + 1);
	*(char*) addr = 0xE9;
	*(int*) (addr + 1) = (int) &samp_RPC_SetPlayerObjectMaterial_Text_ReadTextPatch - (addr + 5);
}

void samp_unpatchObjectMaterialReadText()
{
	int addr;
	addr = samp_handle + 0x1B900;
	*(char*) addr = materialTextPatch1;
	*(int*) (addr + 1) = materialTextPatch4;
}
