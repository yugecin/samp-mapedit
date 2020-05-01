/* vim: set filetype=c ts=8 noexpandtab: */

/*
- thanks to the ones that helped make the idb that I have (I can't remember
  where I found it)
- thanks to Seemann, DjDeji69 and others involved with
  SannyBuilder, CLEO & gtagmodding opcode database
- thanks to plugin-sdk maintainers https://github.com/DK22Pac/plugin-sdk/
*/

#define GAME_RESOLUTION_X *(int*)(0xC17044)
#define GAME_RESOLUTION_Y *(int*)(0xC17048)
#define CPAD 0xB73458
#define _opcodeParameters 0xA43C78

/**
See https://github.com/DK22Pac/plugin-sdk/blob/
8d4d2ff5502ffcb3a741cbcac238d49664689808/plugin_sa/game_sa/rw/rwplcore.h#L3936
*/
enum RwPrimitiveType {
    rwPRIMTYPETRILIST = 3,
    rwPRIMTYPETRISTRIP = 4,
    rwPRIMTYPETRIFAN = 5,
};

enum eTextdrawAlignment {
	CENTER = 0,
	LEFT = 1,
	RIGHT = 2,
};

enum eCameraCutMode {
	SMOOTH = 1,
	CUT = 2,
};

#pragma pack(push,1)
struct IM2DVERTEX {
	float x, y;
	int near;
	int far;
	int col;
	float u;
	float v;
};
EXPECT_SIZE(struct IM2DVERTEX, 0x1C);

struct RwV3D {
	float x;
	float y;
	float z;
};
EXPECT_SIZE(struct RwV3D, 0xC);

struct CColModel
{
	struct RwV3D min;
	struct RwV3D max;
	/*incomplete*/
};

struct CSimpleTransform
{
	struct RwV3D pos;
	float heading;
};
EXPECT_SIZE(struct CSimpleTransform, 0x10);

struct CMatrix {
	struct RwV3D right;
	float pad0;
	struct RwV3D up;
	float pad1;
	struct RwV3D at;
	float pad2;
	struct RwV3D pos;
	float pad3;
	int matrixPtr;
	int haveRwMatrix;
};
EXPECT_SIZE(struct CMatrix, 0x48);

struct CPlaceable
{
	void *__vmt;
	struct CSimpleTransform placement;
	struct CMatrix *matrix;
};
EXPECT_SIZE(struct CPlaceable, 0x18);

/* https://code.google.com/p/
mtasa-blue/source/browse/trunk/MTA10/sdk/game/CEntity.h */
enum eEntityType
{
	ENTITY_TYPE_NOTHING,
	ENTITY_TYPE_BUILDING,
	ENTITY_TYPE_VEHICLE,
	ENTITY_TYPE_PED,
	ENTITY_TYPE_OBJECT,
	ENTITY_TYPE_DUMMY,
	ENTITY_TYPE_NOTINPOOLS
};

struct CEntity
{
	struct CPlaceable _parent;
	void *rwObject;
	unsigned int flags;
	short randomSeed;
	short model;
	void *references;
	void *streamingLink;
	short scanCode;
	char iplIndex;
	unsigned char interior;
	struct CEntity *lod; /*-1 when none*/
	unsigned char numLodChildren;
	unsigned char numLodChildrenRedered;
	unsigned char type_status; /*3 bits type (lo), 5 bits status (hi)*/
	/*incomplete*/
};

#define ENTITY_IS_TYPE(ENTITY,TYPE) \
	((ENTITY->type_status&7)==TYPE)

struct CDoubleLinkListNode {
	void *item;
	struct CDoubleLinkListNode *next;
	struct CDoubleLinkListNode *prev;
};
EXPECT_SIZE(struct CDoubleLinkListNode, 0xC);

struct CSector {
	struct CDoubleLinkListNode *buildings;
	struct CDoubleLinkListNode *dummies;
};
EXPECT_SIZE(struct CSector, 0x8);

struct CRepeatSector {
	struct CDoubleLinkListNode *peds;
	struct CDoubleLinkListNode *vehicles;
	struct CDoubleLinkListNode *objects;
};
EXPECT_SIZE(struct CRepeatSector, 0xC);

struct CCam {
	char __pad0[0x190];
	struct RwV3D lookVector; /*+190*/
	struct RwV3D pos; /*+19C*/
	char __pad1[0x90];
};
EXPECT_SIZE(struct CCam, 0x238);

struct CCamera {
	char __pad0[0x4E];
	char field4E; /*+4E*/
	char __pad4D[0x59 - 0x4F];
	char activeCam; /*+59*/
	char __pad5A[0x174 - 0x5A];
	struct CCam cams[3]; /*+174*/
	char __pad81C[0x860 - 0x81C];
	struct RwV3D lookAt; /*+860*/
	struct RwV3D position; /*+86C*/
	struct RwV3D lookVector; /*+878*/
	char __pad884[0x958 - 0x884];
	int *targetEntity; /*+958*/
	char __pad95C[0xA04 - 0x95C];
	struct CMatrix cameraViewMatrix; /*+A04*/
	char __padA48[0xCEE - 0xA4C];
	char positionLocked; /*+CEE*/
	char directionLocked; /*+CEF*/
	char __padA4C[0xD78 - 0xCF0];
};
EXPECT_SIZE(struct CCamera, 0xD78);

struct Rect {
	float left;
	float bottom;
	float right;
	float top;
};
EXPECT_SIZE(struct Rect, 0x10);

struct CMouseState {
	char lmb;
	char rmb;
	char mmb;
	char wheelUp;
	char wheelDown;
	char xmb1;
	char xmb2;
	char __pad0;
	float z;
	float x;
	float y;
};
EXPECT_SIZE(struct CMouseState, 0x14);

struct CKeyState {
	short function[12];
	short standards[256];
	short esc;
	short insert;
	short del;
	short home;
	short end;
	short pgup;
	short pgdn;
	short up;
	short down;
	short left;
	short right;
	short scroll;
	short pause;
	short numlock;
	short div;
	short mul;
	short sub;
	short add;
	short enter;
	short decimal;
	short num1;
	short num2;
	short num3;
	short num4;
	short num5;
	short num6;
	short num7;
	short num8;
	short num9;
	short num0;
	short back;
	short tab;
	short capslock;
	short extenter;
	short lshift;
	short rshift;
	short shift;
	short lctrl;
	short rctrl;
	short lmenu;
	short rmenu;
	short lwin;
	short rwin;
	short apps;
};
EXPECT_SIZE(struct CKeyState, 0x270);

struct CColPoint
{
	struct RwV3D pos;
	float field_C;
	struct RwV3D normal;
	float field_1C;
	unsigned char surfaceTypeA;
	unsigned char pieceTypeA;
	char field_1F;
	char field_20;
	unsigned char surfaceTypeB;
	unsigned char pieceTypeB;
	char lightingB;
	char field_24;
	float depth;
};
EXPECT_SIZE(struct CColPoint, 0x2C);

#define RACECP_TYPE_ARROW 0
#define RACECP_TYPE_FINISH 1
#define RACECP_TYPE_NORMAL 2
#define RACECP_TYPE_AIRNORM 3
#define RACECP_TYPE_AIRFINISH 4
#define RACECP_TYPE_AIRROT 5
#define RACECP_TYPE_AIRUPDOWNNOTHING 6
#define RACECP_TYPE_AIRUPDOWN1 7
#define RACECP_TYPE_AIRUPDOWN2 8

struct CRaceCheckpoint
{
	char type;
	char free;
	char used;
	char isgoingup; /*for types: air up down*/
	char __pad4[4];
	int colABGR;
	char __padC[4];
	struct RwV3D pos;
	struct RwV3D arrowDirection;
	char __pad28[4];
	float fRadius;
	char __pad30[4];
	float verticaldisplacement; /*for types: air up down*/
};
EXPECT_SIZE(struct CRaceCheckpoint, 0x38);
#define MAXRACECHECKPOINT 32

struct CPedFlags
{
	char flag0;
	char flag1;
	char isInCar;
	char flag3;
	char moreflags[12];
};
EXPECT_SIZE(struct CPedFlags, 0x10);

struct CPed
{
	char __parent_CPhysical[0x138];
	char _pad138[0x46C - 0x138];
	struct CPedFlags flags;
	char _pad47C[0x548 - 0x47C];
	float armor;
	char _pad54C[0x58C - 0x548];
	void *vehicle;
	/*more stuff here*/
};

struct CRope {
	struct RwV3D segments[32];
	struct RwV3D segmentsReleased[32];
	int id;
	float field_304;
	float mass;
	float ropeTotalLength;
	void *ropeHolder;
	void *attachedObject;
	void *attachedEntity;
	float ropeSegmentLength;
	int time;
	char numsegments;
	char type;
	unsigned short flags;
};
EXPECT_SIZE(struct CRope, 0x328);
#pragma pack(pop)

#define MAX_ROPES 8
#define MAX_SECTORS_X 120
#define MAX_SECTORS_Y 120
#define MAX_SECTORS 14400
#define MAX_REPEATSECTORS 256

extern struct CRope *ropes;
extern struct CSector *worldSectors;
extern struct CRepeatSector *worldRepeatSectors;
extern unsigned int *fontColorABGR;
extern unsigned char *enableHudByOpcode;
extern struct CMouseState *activeMouseState;
extern struct CMouseState *currentMouseState;
extern struct CMouseState *prevMouseState;
extern struct CKeyState *activeKeyState;
extern struct CKeyState *currentKeyState;
extern struct CKeyState *prevKeyState;
extern float *gamespeed;
extern char *activecam;
extern struct CCamera *camera;
extern float *hudScaleX, *hudScaleY;
extern struct CRaceCheckpoint *racecheckpoints;
/**
Resets when player starts a new game.
*/
extern int *timeInGame;
extern int *script_PLAYER_CHAR;
extern int *script_PLAYER_ACTOR;
extern struct CPed *player;
extern void *gameHwnd;
/**
There are two 50 char log buffers, only pointing to the first one.
*/
extern unsigned char *logBuffer;
extern unsigned char pedPosBeingUpdated;
extern int (*randMax65535)();

void game_CameraRestore();
void game_CameraRestoreWithJumpCut();
void __stdcall game_CameraSetOnPoint(
	struct RwV3D *point, enum eCameraCutMode cutmode, int unk);
void game_DestroyVehicle(struct CEntity *vehicle);
void game_DrawRect(float x, float y, float w, float h, int argb);
struct CColModel *game_EntityGetColModel(void *entity);
float game_EntityGetDistanceFromCentreOfMassToBaseOfModel(void *entity);
void game_EntitySetAlpha(void *entity, unsigned char alpha);
void game_FreezePlayer(char flag);
void game_init();
/**
Seems to not work with standard keys, check enum RsKeyCode for supported ones.

idb CInputEvents__isKeyJustPressed
sdk CControllerConfigManager::GetIsKeyboardKeyJustDown
*/
int __stdcall game_InputWasKeyPressed(short keycode);
void game_ObjectGetHeadingRad(struct CEntity *object, float *heading);
void game_ObjectGetPos(void *object, struct RwV3D *pos);
void game_ObjectGetRot(struct CEntity *object, struct RwV3D *rot);
void game_ObjectSetHeadingRad(void *object, float heading);
void game_ObjectSetPos(void *object, struct RwV3D *pos);
void game_ObjectSetRotRad(void *object, struct RwV3D *rot);
/**
@param pos can be {@code NULL} when not needed
@param rot can be {@code NULL} when not needed
*/
void game_PedGetPos(struct CPed *ped, struct RwV3D *pos, float *rot);
void game_PedSetPos(struct CPed *ped, struct RwV3D *pos);
void game_PedSetRot(struct CPed *ped, float rot);
void game_RopeRemove(struct CRope *rope);
int game_RwIm2DPrepareRender();
int game_RwIm2DRenderPrimitive(int type, void *verts, int numverts);
void game_RwMatrixInvert(struct CMatrix *out, struct CMatrix *in);
void game_ScreenToWorld(struct RwV3D *out, float x, float y, float dist);
struct CEntity *game_SpawnVehicle(int model);
/**
sdk CFont::GetStringWidth
*/
float game_TextGetSizeX(char *text, char unk1, char unk2);
/**
sdk CFont::GetTextRect
*/
int game_TextGetSizeXY(struct Rect *size, float sizex, float sizey, char *text);
/**
sdk CFont::PrintChar
*/
int game_TextPrintChar(float x, float y, char character);
/**
idb _drawText
sdk CFont::PrintString
*/
int game_TextPrintString(float x, float y, char *text);
/**
sdk CFont::PrintStringFromBottom
*/
int game_TextPrintStringFromBottom(float x, float y, char *text);
/**
smp TextDrawAlignment
idb CText__SetTextAlignment
sdk CFont::SetOrientation
scm ? 03E4: set_text_draw_align_right 0
*/
int game_TextSetAlign(enum eTextdrawAlignment align);
/**
idb dummy_719500
sdk CFont::SetAlphaFade
*/
int game_TextSetAlpha(float alpha);
/**
smp TextDrawUseBox
idb CText__SetTextBackground
sdk CFont::SetBackground
scm 0345: enable_text_draw_background 0
*/
int game_TextSetBackground(char flag, char includewrap);
/**
smp TextDrawBoxColor
idb dummy_7195E0
sdk CFont::SetBackgroundColor
*/
int game_TextSetBackgroundColor(int abgr);
/**
smp TextDrawColor
idb CText__SetTextColour
sdk CFont::SetColor
scm 0340: set_text_draw_RGBA 180 180 180 255
*/
int game_TextSetColor(int abgr);
/**
smp TextDrawFont
idb CText__SetFont
sdk CFont::SetFontStyle
scm 0349: set_text_draw_font 3
*/
int game_TextSetFont(char font);
/**
smp TextDrawFont
sdk CFont::SetJustify
scm 0341: set_text_draw_align_justify 0
*/
int game_TextSetJustify(char flag);
/**
smp TextDrawLetterSize
idb CText__SetTextLetterSize
sdk CFont::SetScale
scm 033F: set_text_draw_letter_size 1.3 3.36
*/
int game_TextSetLetterSize(float x, float y);
/**
See game_TextSetProportional
*/
int game_TextSetMonospace(char monospace);
/**
smp TextDrawSetOutline
idb CText__SetTextOutline
sdk CFont::SetEdge
scm 081C: draw_text_outline 2 RGBA 0 0 0 255
*/
int game_TextSetOutline(char outlinesize);
/**
Inverse monospace

smp TextDrawSetProportional
idb CText__SetTextUseProportionalValues
sdk CFont::SetProportional
scm 0348: enable_text_draw_proportional 1
*/
int game_TextSetProportional(char proportional);
/**
smp TextDrawSetShadow
idb CText__SetTextShadow
sdk CFont::SetDropShadowPosition
scm 060D: draw_text_shadow 0 rgba 0 0 0 255
*/
int game_TextSetShadow(char shadowsize);
/**
smp TextDrawBackgroundColor
idb CText__SetBorderEffectRGBA
sdk CFont::SetDropColor
scm 060D: draw_text_shadow 0 rgba 0 0 0 255
*/
int game_TextSetShadowColor(int abgr);
/**
idk dummy_7194D0
sdk CFont::SetWrapx
*/
int game_TextSetWrapX(float value);
/**
?
*/
int game_TextSetWrapY(float value);
/**
idb _transformPoint
sdk CMatrix::operator*
*/
int game_TransformPoint(
	struct RwV3D *out, struct CMatrix *mat, struct RwV3D *in);
void game_VehicleSetPos(void *vehicle, struct RwV3D *pos);
void game_WorldFindObjectsInRange(
	struct RwV3D *origin,
	float radius,
	int only2d,
	short *numObjects,
	short maxObjects,
	void **entities,
	int buildings,
	int vehicles,
	int peds,
	int objects,
	int dummies);
/**
@return 1 on collision
*/
int game_WorldIntersect(
	struct RwV3D *origin,
	struct RwV3D *direction,
	struct CColPoint *collidedColpoint,
	void **collidedEntity,
	int buildings,
	int vehicles,
	int peds,
	int objects,
	int dummies,
	int doSeeThroughCheck,
	int doCameraIgnoreCheck,
	int doShootThroughCheck);
/**
@return 1 on collision
*/
int game_WorldIntersectBuildingObject(
	struct RwV3D *origin,
	struct RwV3D *direction,
	struct CColPoint *colpoint,
	void **collidedEntity);
/**
@return 1 on collision
*/
int game_WorldIntersectEntity(
	struct RwV3D *origin,
	struct RwV3D *direction,
	struct CColPoint *colpoint,
	void **collidedEntity);
void game_WorldToScreen(struct RwV3D *out, struct RwV3D *in);
