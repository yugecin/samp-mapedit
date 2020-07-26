
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
#define NC_EditObject 7
#define NC_CreateVehicle 8
#define NC_DestroyVehicle 9
#define NC_SetWorldTime 10
#define NC_SetWeather 11
#define NC_DESTROYVEHICLE 12

#define MAPEDIT_MSG_RESETOBJECTS 0
#define MAPEDIT_MSG_NATIVECALL 1
#define MAPEDIT_MSG_OBJECT_CREATED 2
#define MAPEDIT_MSG_RESETVEHICLES 3
#define MAPEDIT_MSG_BULKDELETE 4

#pragma pack(push,1)
struct MSG {
	int id;
};

struct MSG_NC {
	struct MSG _parent;
	int nc;
	union {
		int asint[20];
		float asflt[20];
	} params;
};

struct MSG_OBJECT_CREATED {
	struct MSG _parent;
	int samp_objectid;
	struct OBJECT *object;
};

struct MSG_BULKDELETE {
	struct MSG _parent;
	int num_deletions;
	int objectids[1];
};
#pragma pack(pop)
