
/* vim: set filetype=c ts=8 noexpandtab: */

#define _CRT_SECURE_NO_DEPRECATE

#include "windows.h"
#include "io.h"
#include "../shared/sizecheck.h"
#include "../shared/serverlink.h"
#include "vendor/SDK/amx/amx.h"
#include "vendor/SDK/plugincommon.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_OBJECTS 1000
#define MAX_VEHICLES 2000

typedef void (*cb_t)(void* data);
typedef void (*logprintf_t)(char* format, ...);

logprintf_t logprintf;
extern void *pAMXFunctions;

EXPECT_SIZE(char, 1);
EXPECT_SIZE(short, 2);
EXPECT_SIZE(int, 4);
EXPECT_SIZE(cell, 4);
EXPECT_SIZE(float, 4);

/*in cells*/
#define STACK_HEAP_SIZE 1024

AMX *amx;
struct FAKEAMX_DATA {
	union {
		cell ascell[144];
		char aschr[144 * sizeof(cell)];
		float asflt[144];
	} a144;
	cell _stackheap[STACK_HEAP_SIZE];
} fakeamx_data;
#define basea ((int) &fakeamx_data)
#define buf144a ((int) &fakeamx_data.a144 - basea)
#define buf144 (fakeamx_data.a144.ascell)
#define cbuf144 fakeamx_data.a144.aschr
#define fbuf144 fakeamx_data.a144.asflt

union NCDATA {
	cell asint[20];
	float asflt[20];
} nc_params;

struct NATIVE {
	char *name;
	AMX_NATIVE fp;
};
struct NATIVE natives[] = {
	/*this needs to be synced with NC_ definitions*/
	{ "CreateObject", 0 },
	{ "DestroyObject", 0 },
	{ "SetObjectMaterial", 0 },
	{ "SetObjectMaterialText", 0 },
	{ "SetObjectPos", 0 },
	{ "SetObjectRot", 0 },
	{ "AddPlayerClass", 0 },
	{ "EditObject", 0 },
	{ "CreateVehicle", 0 },
	{ "DestroyVehicle", 0 },
	{ "SetWorldTime", 0 },
	{ "SetWeather", 0 },
	{ "DestroyVehicle", 0 },
	{ "GangZoneCreate", 0 },
	{ "GangZoneShowForAll", 0 },
	{ "GangZoneDestroy", 0 },
	{ "SelectTextDraw", 0 },
};
#define NUMNATIVES (sizeof(natives)/sizeof(struct NATIVE))

static char objectidused[MAX_OBJECTS];

static int socketsend, socketrecv;

PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports()
{
	return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES | SUPPORTS_PROCESS_TICK;
}

static
void gen_gamemode_script()
{
#define cellsize 4
#define PUBLICS 0

	FILE *f;
	int tmp, size;
#if PUBLICS > 0
	char pubtbl[PUBLICS * 8];
#endif
	char nattbl[NUMNATIVES * 8];
	int pubidx, i;
	int cod, cip, dat;
	union {
		char i1;
		short i2;
		int i4;
		char buf[4];
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
#if PUBLICS > 0
	data.i4 += sizeof(pubtbl);
#endif
	fwrite(data.buf, 4, 1, f); /*nativetable*/
	data.i4 += sizeof(nattbl);
	fwrite(data.buf, 4, 1, f); /*libraries*/
	fwrite(data.buf, 4, 1, f); /*pubvars*/
	fwrite(data.buf, 4, 1, f); /*tags*/
	fwrite(data.buf, 4, 1, f); /*nametable*/
	size = 0x38; /*header*/

	/*pubtbl, dummy at this point*/
#if PUBLICS > 0
	fwrite(pubtbl, sizeof(pubtbl), 1, f);
	size += sizeof(pubtbl);
#endif

	/*nattbl, dummy at this point*/
	fwrite(nattbl, sizeof(nattbl), 1, f);
	size += sizeof(nattbl);

	/*namtbl*/
	data.i2 = 31;
	fwrite(data.buf, 2, 1, f); /*max name len (I suppose?)*/
	size += 2;
	pubidx = 0;
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
#if PUBLICS > 0
	table_entry = (void*) pubtbl;
	PUBLIC("MM");
	PUBLIC("dummies");
#endif
	table_entry = (void*) nattbl;
	for (i = 0; i < NUMNATIVES; i++) {
		tmp = strlen(natives[i].name) + 1;
		fwrite(natives[i].name, tmp, 1, f);
		table_entry->addr = 0;
		table_entry->sym = size;
		table_entry++;
		size += tmp;
	}
#undef PUBLIC

	cod = size;

	/*code segment*/
	cip = 0;
	dat = 0;
	data.i1 = 0x80; /*???*/
	fwrite(data.buf, 1, 1, f);
	data.i1 = 0x78; /*OP_HALT*/
	fwrite(data.buf, 1, 1, f);
	data.i1 = 0x00; /*param*/
	fwrite(data.buf, 1, 1, f);
	size += 3;
	dat += 2;
	/*publics*/
#if PUBLICS > 0
	/*TODO: something makes the amx not accept this...*/
	for (tmp = 0; tmp < PUBLICS; tmp++) {
		data.buf[0] = 46; /*OP_PROC*/
		data.buf[1] = 123; /*OP_SYSREC_C*/
		data.buf[2] = tmp; /*name table entry*/
		data.buf[3] = 48; /*OP_RETN*/
		fwrite(data.buf, 4, 1, f);
		size += 4;
		dat += 4;
	}
#endif
	/*entrypoint*/
	cip = (size - cod) * cellsize;
	data.i1 = 46; /*OP_PROC*/
	fwrite(data.buf, 1, 1, f);
	data.i1 = 48; /*OP_RETN*/
	fwrite(data.buf, 1, 1, f);
	size += 2;
	dat += 2;
	dat = cod + dat * cellsize;

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
#if PUBLICS > 0
	fwrite(pubtbl, sizeof(pubtbl), 1, f); /*pubtbl*/
#endif
	fwrite(nattbl, sizeof(nattbl), 1, f); /*nattbl*/

	fclose(f);
}

PLUGIN_EXPORT int PLUGIN_CALL Load(void **ppData)
{
	struct sockaddr_in addr;
	struct sockaddr *saddr = (struct sockaddr*) &addr;
	int flags;

	pAMXFunctions = ppData[PLUGIN_DATA_AMX_EXPORTS];
	logprintf = (logprintf_t) ppData[PLUGIN_DATA_LOGPRINTF];
	gen_gamemode_script();

	socketrecv = socket(AF_INET, SOCK_DGRAM, 0);
	if (socketrecv == -1) {
		printf("failed to create recv socket\n");
		return 0;
	}

	socketsend = socket(AF_INET, SOCK_DGRAM, 0);
	if (socketsend == -1) {
		closesocket(socketrecv);
		socketrecv = -1;
		printf("failed to create send socket\n");
		return 0;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SOCKET_PORT_SERVER);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(socketrecv, saddr, sizeof(addr)) == -1) {
		closesocket(socketrecv);
		closesocket(socketsend);
		socketrecv = -1;
		logprintf("socket cannot listen (port %d)", SOCKET_PORT_SERVER);
		return 0;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(SOCKET_PORT_CLIENT);
	if (connect(socketsend, saddr, sizeof(struct sockaddr)) == -1) {
		closesocket(socketrecv);
		closesocket(socketsend);
		socketrecv = -1;
		logprintf("send socket cannot connect");
		return 0;
	}

	/*set sockets as non-blocking*/
	flags = 1;
	ioctlsocket(socketsend, FIONBIO, (DWORD*) &flags);
	ioctlsocket(socketrecv, FIONBIO, (DWORD*) &flags);

	return 1;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload()
{
	if (socketrecv != -1) {
		closesocket(socketrecv);
		closesocket(socketsend);
	}
}

#define REGISTERNATIVE(X) {#X, X}
AMX_NATIVE_INFO PluginNatives[] =
{
	{0, 0}
};

static
void collect_natives()
{
	struct NATIVE *n = natives;
	AMX_HEADER *header;
	unsigned char *nattabl;
	int i;

	header = (AMX_HEADER*) amx->base;
	nattabl = (unsigned char*) header + header->natives;

	for (i = 0; i < NUMNATIVES; i++) {
		/*skipping some steps here...*/
		natives[i].fp = (AMX_NATIVE) *((int*) (nattabl + i * 8));
	}
}

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX *a)
{
	AMX_HEADER *hdr;
	int tmp;

	amx = a;
	collect_natives();

	nc_params.asint[1] = 0;
	nc_params.asflt[2] = 0.0f;
	nc_params.asflt[3] = 0.0f;
	nc_params.asflt[4] = 4.5f;
	nc_params.asflt[5] = 0.0f;
	nc_params.asint[6] = nc_params.asint[8] = nc_params.asint[10] = 0;
	nc_params.asint[7] = nc_params.asint[9] = nc_params.asint[11] = 0;
	natives[NC_AddPlayerClass].fp(amx, nc_params.asint);

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

	memset(objectidused, 0, sizeof(objectidused));

	return amx_Register(a, PluginNatives, -1);
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX *a)
{
	return AMX_ERR_NONE;
}

static
void socket_send(char *data, int len)
{
	send(socketsend, data, len, 0);
}

static
void msg_resetobjects()
{
	int idx;
	AMX_NATIVE nc_destroy;

	nc_destroy = natives[NC_DestroyObject].fp;
	for (idx = 0; idx < MAX_OBJECTS; idx++) {
		if (objectidused[idx]) {
			nc_params.asint[1] = idx;
			nc_destroy(amx, nc_params.asint);
			objectidused[idx] = 0;
		}
	}

	/*
	nc_params.asint[1] = 0;
	natives[NC_GangZoneDestroy].fp(amx, nc_params.asint);
	nc_params.asint[1] = 0x44556677;
	nc_params.asint[2] = 0x88996677;
	nc_params.asint[3] = 0x11223344;
	nc_params.asint[4] = 0xF3388FF8;
	natives[NC_GangZoneCreate].fp(amx, nc_params.asint);
	nc_params.asint[1] = 0;
	nc_params.asint[2] = 0xFF00FFFF;
	natives[NC_GangZoneShowForAll].fp(amx, nc_params.asint);
	*/
}

static
void msg_resetvehicles()
{
	int idx;

	for (idx = 0; idx < MAX_VEHICLES; idx++) {
		nc_params.asint[1] = idx;
		natives[NC_DestroyVehicle].fp(amx, nc_params.asint);
	}
}

static
void msg_nativecall(struct MSG_NC *msg)
{
	struct MSG_OBJECT_CREATED createdmsg;
	int idx, res;

	idx = msg->nc;
	if (0 <= idx && idx < NUMNATIVES) {
		res = natives[idx].fp(amx, msg->params.asint);
		if (idx == NC_CreateObject) {
			objectidused[res] = 1;
			createdmsg._parent.id = MAPEDIT_MSG_OBJECT_CREATED;
			createdmsg.object = (void*) msg->params.asint[2]; /*x*/
			createdmsg.samp_objectid = res;
			socket_send((char*) &createdmsg, sizeof(createdmsg));
		}
	} else {
		logprintf("invalid nc idx in rpc_nc: %d", idx);
	}
}

static
void msg_bulkdelete(struct MSG_BULKDELETE *msg)
{
	int i;

	for (i = 0; i < msg->num_deletions; i++) {
		nc_params.asint[1] = msg->objectids[i];
		natives[NC_DestroyObject].fp(amx, nc_params.asint);
	}
}

PLUGIN_EXPORT void PLUGIN_CALL ProcessTick()
{
	static struct sockaddr_in remote_client;
	static int client_len = sizeof(remote_client);
	static struct sockaddr* rc = (struct sockaddr*) &remote_client;
	static char buf[8096];

	int recvsize, msgid;

	recvsize = recvfrom(socketrecv, buf, sizeof(buf), 0, rc, &client_len);
	if (recvsize > 3) {
		msgid = ((struct MSG*) buf)->id;
		switch (msgid) {
		case MAPEDIT_MSG_RESETOBJECTS:
			msg_resetobjects();
			break;
		case MAPEDIT_MSG_NATIVECALL:
			if (recvsize == sizeof(struct MSG_NC)) {
				msg_nativecall((struct MSG_NC*) buf);
			}
			break;
		case MAPEDIT_MSG_RESETVEHICLES:
			msg_resetvehicles();
			break;
		case MAPEDIT_MSG_BULKDELETE:
			if (recvsize >= sizeof(struct MSG_BULKDELETE)) {
				msg_bulkdelete((struct MSG_BULKDELETE*) buf);
			}
			break;
		default:
			logprintf("unknown MSG: %d", msgid);
			break;
		}
	}
}
