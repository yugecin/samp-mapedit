/* vim: set filetype=c ts=8 noexpandtab: */

#include "CLEO_SDK/CLEO.h"
#include "../client/client.h"

static HMODULE client = NULL;
static client_finalize_t proc_finalize;

void unload_client()
{
	if (client != NULL) {
		proc_finalize();
		FreeLibrary(client);
		client = NULL;
	}
}

OpcodeResult WINAPI opcode_0C62(CScriptThread *thread)
{
	MapEditMain_t clientmain;

	if (client != NULL) {
		unload_client();
		/*when reloading MPACK, this sleep allows the copy script
		to copy the file while it's unused (copy script sleeps for 2s)*/
		Sleep(2750);
	}
	client = LoadLibrary("CLEO\\mapedit-client.dll");
	clientmain = (MapEditMain_t) GetProcAddress(client, "MapEditMain");
	proc_finalize = clientmain(&unload_client);
	return OR_CONTINUE;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason_for_call, LPVOID lpResrvd)
{
	if (reason_for_call == DLL_PROCESS_ATTACH) {
		return CLEO_RegisterOpcode(0x0C62, &opcode_0C62);
	}
	return TRUE;
}

