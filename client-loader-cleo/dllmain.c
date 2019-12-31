/* vim: set filetype=c ts=8 noexpandtab: */

#include "CLEO_SDK/CLEO.h"
#include "../client/client.h"

static HMODULE client = NULL;

void unload_client()
{
	if (client != NULL) {
		FreeLibrary(client);
		client = NULL;
	}
}

OpcodeResult WINAPI opcode_0C62(CScriptThread *thread)
{
	if (client != NULL) {
		unload_client();
		/*when reloading MPACK, this sleep allows the copy script
		to copy the file while it's unused (copy script sleeps for 2s)*/
		Sleep(2750);
	}
	*((int*) 0xBAB230) = 0xFF0000FF;
	client = LoadLibrary("CLEO\\mapedit-client.dll");
	((MapEditMain_t) GetProcAddress(client, "MapEditMain"))(&unload_client);
	return OR_CONTINUE;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason_for_call, LPVOID lpResrvd)
{
	if (reason_for_call == DLL_PROCESS_ATTACH) {
		return CLEO_RegisterOpcode(0x0C62, &opcode_0C62);
	}
	return TRUE;
}

