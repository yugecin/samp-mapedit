
/* vim: set filetype=c ts=8 noexpandtab: */

#define _CRT_SECURE_NO_DEPRECATE

#include <stdlib.h>
#include <stdio.h>

#include "vendor/SDK/amx/amx.h"
#include "vendor/SDK/plugincommon.h"

#define STATIC_ASSERT(E) typedef char __static_assert_[(E)?1:-1]
#define EXPECT_SIZE(S,SIZE) STATIC_ASSERT(sizeof(S)==(SIZE))

typedef void (*cb_t)(void* data);

typedef void (*logprintf_t)(char* format, ...);

extern logprintf_t logprintf;

/*in cells*/
#define STACK_HEAP_SIZE 1024

struct FAKEAMX_DATA {
	union {
		cell ascell[144];
		char aschr[144 * sizeof(cell)];
		float asflt[144];
	} a144;
	cell _stackheap[STACK_HEAP_SIZE];
};
#define basea ((int) &fakeamx_data)
#define buf144a ((int) &fakeamx_data.a144 - basea)
#define buf144 (fakeamx_data.a144.ascell)
#define cbuf144 fakeamx_data.a144.aschr
#define fbuf144 fakeamx_data.a144.asflt

extern AMX *amx;
extern struct FAKEAMX_DATA fakeamx_data;

extern AMX_NATIVE n_CreateObject;
extern AMX_NATIVE n_DestroyObject;
extern AMX_NATIVE n_SetObjectMaterial;
extern AMX_NATIVE n_SetObjectMaterialText;
extern AMX_NATIVE n_SetObjectPos;
extern AMX_NATIVE n_SetObjectRot;

union NCDATA {
	cell asint[20];
	float asflt[20];
};
extern union NCDATA nc_params;
