
/* vim: set filetype=c ts=8 noexpandtab: */

#include "common.h"

logprintf_t logprintf;
extern void *pAMXFunctions;

EXPECT_SIZE(char, 1);
EXPECT_SIZE(short, 2);
EXPECT_SIZE(int, 4);
EXPECT_SIZE(cell, 4);
EXPECT_SIZE(float, 4);

AMX *amx;
struct FAKEAMX_DATA fakeamx_data;

PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports()
{
	return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES | SUPPORTS_PROCESS_TICK;
}

PLUGIN_EXPORT int PLUGIN_CALL Load(void **ppData)
{
	pAMXFunctions = ppData[PLUGIN_DATA_AMX_EXPORTS];
	logprintf = (logprintf_t) ppData[PLUGIN_DATA_LOGPRINTF];
	return 1;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload()
{
}

PLUGIN_EXPORT void PLUGIN_CALL ProcessTick()
{
}

#define REGISTERNATIVE(X) {#X, X}
AMX_NATIVE_INFO PluginNatives[] =
{
	{0, 0}
};

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX *a)
{
	AMX_HEADER *hdr;
	int tmp;

	amx = a;

	/*relocate the data segment*/
	hdr = (AMX_HEADER*) amx->base;
	tmp = hdr->hea - hdr->dat;
	if (tmp) {
		/*data section will be cleared so whoever references it
		will probably overwrite our precious data*/
		logprintf("WARN: data section is not empty! (%d)", tmp);
	}
	tmp = hdr->stp - hdr->hea - STACK_HEAP_SIZE * sizeof(cell);
	if (tmp) {
		logprintf("ERR: stack/heap size mismatch! "
			"(excess %d cells)",
			tmp / sizeof(cell));
		return 0;
	}
	/*copy content of stack/heap to our data segment (this is most
	likely empty, but better safe than sorry)*/
	memcpy(fakeamx_data._stackheap,
		amx->base + hdr->hea,
		STACK_HEAP_SIZE * sizeof(cell));
	/*finally do the relocation*/
	amx->data = (unsigned char*) &fakeamx_data;
	/*adjust pointers since the data segment grew*/
	tmp = (char*) &fakeamx_data._stackheap - (char*) &fakeamx_data;
	amx->frm += tmp;
	amx->hea += tmp;
	amx->hlw += tmp;
	amx->stk += tmp;
	amx->stp += tmp;
	/*samp core doesn't seem to use amx_SetString or amx_GetString
	so the data offset needs to be adjusted*/
	hdr->dat = (int) &fakeamx_data - (int) hdr;
	/*this won't work on linux because linux builds have assertions
	enabled, but this is only targeted for windows anyways*/

	/*if (!natives_find()) {
		return 0;
	}*/

	return amx_Register(a, PluginNatives, -1);
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX *a)
{
	return AMX_ERR_NONE;
}
