/* vim: set filetype=c ts=8 noexpandtab: */

#include "samp.h"
#include <windows.h>

#define BASE 0x3BA0000
#define PCMDWINDOW 0x3DBA0E8
#define TOREL(X) (X - BASE)

static int pCmdWindow = TOREL(PCMDWINDOW);
/**
read-only
*/
static int *chat_bar_visible;
static unsigned char *chat_bar_enable_op;
static unsigned char isR2;

int samp_handle;

void samp_init()
{
	DWORD oldvp;

	samp_handle = (int) GetModuleHandle("samp.dll");
	pCmdWindow += samp_handle;
	chat_bar_visible = (int*) (*((int*) pCmdWindow) + 0x14E0);
	chat_bar_enable_op = (unsigned char*) (TOREL(0x3C057E0) + samp_handle);

	isR2 = 0;
	/*check for "-R2 " string in startup message*/
	if (*((int*) (samp_handle + 0xD3988)) == 0x7B203252) {
		isR2 = 1;
		chat_bar_enable_op = (unsigned char*) (samp_handle + 0x658B0);
		/*TODO: char_bar_visible*/
	}

	VirtualProtect(chat_bar_enable_op, 1, PAGE_EXECUTE_READWRITE, &oldvp);
}

void samp_dispose()
{
	samp_restore_chat_bar();
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

/*untested (also needs samp_handle check)
__declspec(naked) int _samp_show_chat_bar()
{
	_asm {
		mov ecx, pCmdWindow
		mov eax, TOREL(0x3C057E0)
		add eax, samp_handle
		jmp eax
	}
}

__declspec(naked) int _samp_hide_chat_bar()
{
	_asm {
		mov ecx, pCmdWindow
		mov eax, TOREL(0x3C058E0)
		add eax, samp_handle
		jmp eax
	}
}
*/
