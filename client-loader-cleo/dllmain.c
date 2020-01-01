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

/**
Loads the editor client.

Example:
{$O 0C62=0,amazingshit}
0C62:
*/
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

/**
Moves a file.

Unloads client if it has been loaded and load it again after done moving.

Example:
{$O 0C63=2,move file %1d% to %2d%}
0AC6: 0@ = label @FROM offset
0AC6: 1@ = label @TO offset
0C63: move file 0@ to 1@
0A93: end_custom_thread
:FROM
hex
	"C:\\file.txt" 00
end
:TO
hex
	"C:\\file2.txt" 00
end
*/
OpcodeResult WINAPI opcode_0C63(CScriptThread *thread)
{
	char *from = (char*) CLEO_GetIntOpcodeParam(thread);
	char *to = (char*) CLEO_GetIntOpcodeParam(thread);

	if (client != NULL) {
		unload_client();
	}
	MoveFileExA(
		from,
		to,
		MOVEFILE_COPY_ALLOWED |
		MOVEFILE_REPLACE_EXISTING |
		MOVEFILE_WRITE_THROUGH);
	return opcode_0C62(thread);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason_for_call, LPVOID lpResrvd)
{
	if (reason_for_call == DLL_PROCESS_ATTACH) {
		return CLEO_RegisterOpcode(0x0C62, &opcode_0C62) &&
			CLEO_RegisterOpcode(0x0C63, &opcode_0C63);
	}
	return TRUE;
}

