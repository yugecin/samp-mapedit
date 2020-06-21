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
