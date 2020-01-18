
/* vim: set filetype=c ts=8 noexpandtab: */

#define SOCKET_PORT_CLIENT 8887
#define SOCKET_PORT_SERVER 8889

#define NC_CreateObject 0
#define NC_DestroyObject 1
#define NC_SetObjectMaterial 2
#define NC_SetObjectMaterialText 3
#define NC_SetObjectPos 4
#define NC_SetObjectRot 5
#define NC_AddPlayerClass 6

#define MAPEDIT_RPC_RESETOBJECTS 0
#define MAPEDIT_RPC_NATIVECALL 1

struct RPC {
	int id;
};

struct RPC_NC {
	struct RPC;
	int nc;
	int params[20];
};
