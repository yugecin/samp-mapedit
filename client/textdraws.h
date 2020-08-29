/* vim: set filetype=c ts=8 noexpandtab: */

#define MAX_TEXTDRAWS 2048
#define MAX_PLAYER_TEXTDRAWS 256

#pragma pack(push,1)
struct TEXTDRAW
{
	char		szText[800 + 1];
	char		szString[1600 + 2]; /*copied into from szText by samp's draw func*/
	float		fLetterWidth; // 0.24
	float		fLetterHeight; // 1.0
	int		dwLetterColor; // -1
	char		byte_unk; // 0	//(justified?)
	char		byteCenter; // 0
	char		byteBox; // 0
	float		fBoxSizeX; // 1280.0
	float		fBoxSizeY; // 1280.0
	int		dwBoxColor; // 0x80808080
	char		byteProportional; // 1
	int		dwShadowColor; // 0xFF000000
	char		byteShadowSize; // 1
	char		byteOutline; // 0
	char		byteLeft; // 0
	char		byteRight; // 0
	int		iStyle;	// 1	// font style/texture/model
	float		fX; // 546.0
	float		fY; // 48.0
	char		__unk1[8]; // 0, 0, 0, 0, 0, 0, 0, 0
	int		__unk2;	// -1 // -1
	int		__unk3;	// -1 // -1
	int		probablyTextureIdForPreview; // -1 // -1
	char		selectable; // 0
	short		sModel;
	float		fRot[3];
	float		fZoom; // 1.0
	short		sColor[2]; //-1 -1
	char		f9BE;
	char		byte9BF;
	char		byte9C0;
	int		dword9C1;
	int		dword9C5;
	int		dword9C9;
	int		dword9CD;
	char		byte9D1;
	int		dword9D2;
};
//EXPECT_SIZE(struct TEXTDRAW, 0x9D6);
#pragma pack(pop)

#define TEXTDRAW_NAME_LEN 100
#define TEXTDRAW_TEXT_LEN 800

extern struct TEXTDRAW textdraws[MAX_TEXTDRAWS];
extern char textdraw_text[MAX_TEXTDRAWS][TEXTDRAW_TEXT_LEN];
extern char textdraw_name[MAX_TEXTDRAWS][TEXTDRAW_NAME_LEN];
extern int *textdraw_enabled;
extern int numtextdraws;

void textdraws_init();
void textdraws_dispose();
void textdraws_frame_update();
void textdraws_ui_activated();
void textdraws_ui_deactivated();
void textdraws_on_active_window_changed(struct UI_WINDOW *_wnd);
