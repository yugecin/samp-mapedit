/* vim: set filetype=c ts=8 noexpandtab: */

#define _CRT_SECURE_NO_DEPRECATE

#include "CLEO_SDK/CLEO.h"
#include "../shared/sizecheck.h"
#include "../shared/clientlink.h"
#include <stdio.h>

static HMODULE client = NULL;

static char *clientcompilepath = NULL;
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
	if (clientcompilepath != NULL) {
		CopyFileA(clientcompilepath, "CLEO\\mapedit-client.dll", FALSE);
	}
	client = LoadLibrary("CLEO\\mapedit-client.dll");
	((MapEditMain_t*) GetProcAddress(client, "MapEditMain"))(&data);
	data.proc_client_clientmain(data);
}

/**
reloads client, to be called from the client
*/
static
void __cdecl reload_client()
{
	unload_client();
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
	}
	load_client();
	return OR_CONTINUE;
}

static
void WINAPI expandEntryInfoNodePool(HMODULE hModule)
{
	int *size = (void*) 0x550FBA;
	// default is 500, samp bumps this to 5000, we want more
	while (*size != 5000) Sleep(20);
	*size = 10000;
	ExitThread(/*exit code*/ 0);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason_for_call, LPVOID lpResrvd)
{
	HANDLE hThread;
	FILE *f;
	char *c;

	if (reason_for_call == DLL_PROCESS_ATTACH) {
		f = fopen("CLEO\\mapedit-client.dll.txt", "r");
		if (f) {
			clientcompilepath = malloc(200);
			clientcompilepath[fread(clientcompilepath, 1, 200, f)] = 0;
			clientcompilepath[199] = 0;
			c = clientcompilepath;
			while (*c != 0 && *c != '\n') c++;
			*c = 0;
			fclose(f);
		}
		hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) expandEntryInfoNodePool, hModule, 0, NULL);
		if (hThread) {
			CloseHandle(hThread);
		}
		data.proc_loader_reload_client = reload_client;
		data.proc_loader_unload_client = unload_client;
		return CLEO_RegisterOpcode(0x0C62, &opcode_0C62);
	}
	return TRUE;
}

