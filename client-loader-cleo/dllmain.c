/* vim: set filetype=c ts=8 noexpandtab: */

#define _CRT_SECURE_NO_DEPRECATE

#include "CLEO_SDK/CLEO.h"
#include "../client/client.h"

static HMODULE client = NULL;

static char *clientcompilepath = NULL, *clienttargetpath;
static struct CLIENTLINK data;

static
void unload_client()
{
	if (client != NULL) {
		data.proc_client_finalize();
		FreeLibrary(client);
		client = NULL;
	}
}

static
void load_client()
{
	client = LoadLibrary("CLEO\\mapedit-client.dll");
	((MapEditMain_t*) GetProcAddress(client, "MapEditMain"))(&data);
	data.proc_client_clientmain(data);
}

static
void copy_client()
{
	CopyFileA(clientcompilepath, clienttargetpath, FALSE);
}

static
void __cdecl reload_client()
{
	unload_client();
	copy_client();
	load_client();
}

/**
Loads the editor client.

Example:
{$O 0C62=0,amazingshit}
0C62:
*/
static
OpcodeResult WINAPI opcode_0C62(CScriptThread *thread)
{
	if (client != NULL) {
		unload_client();
		/*when reloading MPACK, this sleep allows the copy script
		to copy the file while it's unused (copy script sleeps for 2s)*/
		Sleep(2750);
	}
	load_client();
	return OR_CONTINUE;
}

/**
Moves a file.

Unloads client if it has been loaded and load it again after done moving.

Example:
{$O 0C63=2,copy client %1d% to %2d% and load}
0AC6: 0@ = label @FROM offset
0AC6: 1@ = label @TO offset
0C63: copy client 0@ to 1@ and load
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
static
OpcodeResult WINAPI opcode_0C63(CScriptThread *thread)
{
	char *from = (char*) CLEO_GetIntOpcodeParam(thread);
	char *to = (char*) CLEO_GetIntOpcodeParam(thread);

	if (clientcompilepath == NULL) {
		clientcompilepath = malloc(sizeof(char) * strlen(from));
		clienttargetpath = malloc(sizeof(char) * strlen(to));
		strcpy(clientcompilepath, from);
		strcpy(clienttargetpath, to);
	}
	if (client != NULL) {
		unload_client();
	}
	copy_client();
	load_client();
	return opcode_0C62(thread);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason_for_call, LPVOID lpResrvd)
{
	if (reason_for_call == DLL_PROCESS_ATTACH) {
		data.proc_loader_reload_client = reload_client;
		data.proc_loader_unload_client = unload_client;
		return CLEO_RegisterOpcode(0x0C62, &opcode_0C62) &&
			CLEO_RegisterOpcode(0x0C63, &opcode_0C63);
	}
	return TRUE;
}

