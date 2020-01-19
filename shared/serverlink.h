
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

#define MAPEDIT_MSG_RESETOBJECTS 0
#define MAPEDIT_MSG_NATIVECALL 1

struct MSG {
	int id;
};

struct MSG_NC {
	struct MSG parent;
	int nc;
	union {
		int asint[20];
		float asflt[20];
	} params;
};
