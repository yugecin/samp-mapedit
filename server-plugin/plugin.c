
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

AMX_NATIVE n_CreateObject;
AMX_NATIVE n_DestroyObject;
AMX_NATIVE n_SetObjectMaterial;
AMX_NATIVE n_SetObjectMaterialText;
AMX_NATIVE n_SetObjectPos;
AMX_NATIVE n_SetObjectRot;
union NCDATA nc_params;

PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports()
{
	return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES | SUPPORTS_PROCESS_TICK;
}

static
void gen_gamemode_script()
{
#define cellsize 4
#define PUBLICS 2
#define NATIVES 1

	FILE *f;
	int tmp, size;
	char pubtbl[PUBLICS * 8], nattbl[NATIVES * 8];
	int pubidx;
	int cod, cip, dat;
	union {
		char i1;
		short i2;
		int i4;
		char buf[1000];
	} data;
	struct {
		int addr, sym;
	} *table_entry;

	/*error handling is.. missing*/
	f = fopen("mapedit.amx", "wb");

	/*header*/
	fwrite(data.buf, 4, 1, f); /*size, to be set when done*/
	data.i2 = 0xF1E0;
	fwrite(data.buf, 2, 1, f); /*magic*/
	data.i1 = 0x8;
	fwrite(data.buf, 1, 1, f); /*file version*/
	data.i1 = 0x8;
	fwrite(data.buf, 1, 1, f); /*amx version*/
	data.i2 = 0x14;
	fwrite(data.buf, 2, 1, f); /*flags*/
	data.i2 = 0x8;
	fwrite(data.buf, 2, 1, f); /*defsize*/
	data.i4 = 0; /*done later*/
	fwrite(data.buf, 4, 1, f); /*cod*/
	data.i4 = 0; /*done later*/
	fwrite(data.buf, 4, 1, f); /*dat*/
	data.i4 = 0; /*done later*/
	fwrite(data.buf, 4, 1, f); /*hea*/
	data.i4 = 0; /*done later*/
	fwrite(data.buf, 4, 1, f); /*stp*/
	data.i4 = 0; /*done later*/
	fwrite(data.buf, 4, 1, f); /*cip*/
	data.i4 = 0x38;
	fwrite(data.buf, 4, 1, f); /*publictable*/
	data.i4 += sizeof(pubtbl);
	fwrite(data.buf, 4, 1, f); /*nativetable*/
	data.i4 += sizeof(nattbl);
	fwrite(data.buf, 4, 1, f); /*libraries*/
	fwrite(data.buf, 4, 1, f); /*pubvars*/
	fwrite(data.buf, 4, 1, f); /*tags*/
	fwrite(data.buf, 4, 1, f); /*nametable*/
	size = 0x38; /*header*/

	/*pubtbl, dummy for now*/
	fwrite(pubtbl, sizeof(pubtbl), 1, f);
	size += sizeof(pubtbl);

	/*nattbl, dummy for now*/
	fwrite(nattbl, sizeof(nattbl), 1, f);
	size += sizeof(nattbl);

	/*namtbl*/
	data.i2 = 31;
	fwrite(data.buf, 2, 1, f); /*max name len (I suppose?)*/
	size += 2;
	pubidx = 0;
#define NATIVE(NAME) \
	tmp=strlen(NAME)+1;\
	fwrite(NAME,tmp,1,f);\
	table_entry->addr=0;\
	table_entry->sym=size;\
	table_entry++;\
	size+=tmp;
#define PUBLIC(NAME) \
	tmp=strlen(NAME)+1;\
	fwrite(NAME,tmp,1,f);\
	table_entry->addr=(2+pubidx*4)*cellsize;\
	table_entry->sym=size;\
	table_entry++;\
	pubidx++;\
	size+=tmp;
	/*first [PUBLICS] amount of natives should be the names of the
	native plugin functions to passthrough callbacks to*/
	table_entry = (void*) pubtbl;
	PUBLIC("MM");
	PUBLIC("dummies");
	table_entry = (void*) nattbl;
	NATIVE("AddPlayerClass");

	cod = size;

	/*code segment*/
	cip = 0;
	data.i1 = 0x78;
	fwrite(data.buf, 1, 1, f);
	data.i1 = 0x00;
	fwrite(data.buf, 1, 1, f);
	size += 2;
	/*publics*/
	for (tmp = 0; tmp < PUBLICS; tmp++) {
		data.buf[0] = 46; /*OP_PROC*/
		data.buf[1] = 123; /*OP_SYSREC_C*/
		data.buf[2] = tmp; /*name table entry*/
		data.buf[3] = 48; /*OP_RETN*/
		fwrite(data.buf, 4, 1, f);
	}
	size += 4 * PUBLICS;
	/*entrypoint*/
	cip = (size - cod) * cellsize;
	data.i1 = 46; /*OP_PROC*/
	fwrite(data.buf, 1, 1, f);
	data.i1 = 48; /*OP_RETN*/
	fwrite(data.buf, 1, 1, f);
	size += 2;
	dat = cod + (size - cod) * cellsize;

	fseek(f, 0, SEEK_SET);
	data.i4 = size;
	fwrite(data.buf, 4, 1, f); /*size*/
	fseek(f, 0xC, SEEK_SET);
	data.i4 = cod;
	fwrite(data.buf, 4, 1, f); /*cod*/
	data.i4 = dat;
	fwrite(data.buf, 4, 1, f); /*dat*/
	fwrite(data.buf, 4, 1, f); /*hea*/
	data.i4 += STACK_HEAP_SIZE * cellsize;
	fwrite(data.buf, 4, 1, f); /*stp*/
	data.i4 = cip;
	fwrite(data.buf, 4, 1, f); /*cip*/

	fseek(f, 0x38, SEEK_SET);
	fwrite(pubtbl, sizeof(pubtbl), 1, f); /*pubtbl*/
	fwrite(nattbl, sizeof(nattbl), 1, f); /*nattbl*/

	fclose(f);
}

PLUGIN_EXPORT int PLUGIN_CALL Load(void **ppData)
{
	pAMXFunctions = ppData[PLUGIN_DATA_AMX_EXPORTS];
	logprintf = (logprintf_t) ppData[PLUGIN_DATA_LOGPRINTF];
	gen_gamemode_script();
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

static
int findnatives()
{
#define N(X) {#X,(int*)&n_##X}
	struct NATIVE {
		char *name;
		int *var;
	};
	struct NATIVE natives[] = {
		N(CreateObject),
		N(DestroyObject),
		N(SetObjectMaterial),
		N(SetObjectMaterialText),
		N(SetObjectPos),
		N(SetObjectRot),
	};
	struct NATIVE *n = natives + sizeof(natives)/sizeof(struct NATIVE);
	AMX_HEADER *header;
	AMX_FUNCSTUB *func;
	unsigned char *nativetable;
	int nativesize;
	int idx;

	header = (AMX_HEADER*) amx->base;
	nativetable = (unsigned char*) header + header->natives;
	nativesize = (int) header->defsize;

	while (n-- != natives) {
		if (amx_FindNative(amx, n->name, &idx) == AMX_ERR_NOTFOUND) {
			logprintf("ERR: no %s native", n->name);
			return 0;
		}
		func = (AMX_FUNCSTUB*) (nativetable + idx * nativesize);
		*n->var = func->address;
	}
	return 1;
}

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

	if (!findnatives()) {
		return 0;
	}

	return amx_Register(a, PluginNatives, -1);
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX *a)
{
	return AMX_ERR_NONE;
}
