/* vim: set filetype=c ts=8 noexpandtab: */

#include "client.h"

static unload_client_t proc_unload;
static int original_money_color;

/**
Called from the loader, do not call directly (use proc_unload)
*/
void client_finalize()
{
	*((int*) 0xBAB230) = original_money_color;
}

__declspec(dllexport)
client_finalize_t __cdecl MapEditMain(unload_client_t unloadfun)
{
	proc_unload = unloadfun;
	/*set money color just to test if it works*/
	original_money_color = *((int*) 0xBAB230);
	*((int*) 0xBAB230) = 0xFFFF4336;
	return &client_finalize;
}
